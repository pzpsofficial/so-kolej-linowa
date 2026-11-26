/*
 * kasjer.c - proces kasjera.
 *
 * Kasjer czeka na komunikaty w kolejce kasy (msgrcv blokuje proces, więc
 * nie ma aktywnego czekania). Kończy pracę po komunikacie KOM_KONIEC od
 * procesu głównego albo po sygnale SIGTERM/SIGINT.
 *
 * Etap szkieletu: kasjer jeszcze nie sprzedaje biletów.
 */
#include "common/blad.h"
#include "common/ipc.h"
#include "common/log.h"
#include "common/proces.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>

static volatile sig_atomic_t koniec = 0;
static volatile sig_atomic_t czeka_w_kolejce = 0;

/*
 * Sygnał zakończenia. Jeśli kasjer śpi w msgrcv() (albo zaraz w nim zaśnie),
 * kończymy od razu przez _exit(): flaga mogłaby zostać sprawdzona tuż przed
 * zaśnięciem i proces spałby dalej. W kasjerze nie ma nic do sprzątania,
 * czego nie zrobiłoby jądro (odłączenie pamięci, zamknięcie plików).
 */
static void obsluz_koniec(int sygnal)
{
    (void)sygnal;
    koniec = 1;
    if (czeka_w_kolejce)
        _exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    (void)argc;
    proces_ustaw_katalog(argv[0]);

    ZasobyIpc zasoby;
    ipc_dolacz(&zasoby);
    log_start("kasjer", 0, 0);
    log_ustaw_stan(zasoby.stan);
    proces_obsluga_sygnalu(SIGTERM, obsluz_koniec);
    proces_obsluga_sygnalu(SIGINT, obsluz_koniec);

    log_zapisz(LOG_INFO, "kasa otwarta");
    for (;;) {
        KomunikatKasa komunikat;
        czeka_w_kolejce = 1;
        if (koniec)
            break;
        ssize_t odebrano = msgrcv(zasoby.kasa, &komunikat, sizeof komunikat - sizeof komunikat.mtype, 0, 0);
        czeka_w_kolejce = 0;
        if (odebrano == -1) {
            if (errno == EINTR)
                continue;
            KRYTYCZNY("msgrcv(kasa)");
        }
        if (komunikat.mtype == KOM_KONIEC)
            break;
    }
    czeka_w_kolejce = 0;

    log_zapisz(LOG_INFO, "kasa zamknięta");
    ipc_odlacz(&zasoby);
    log_koniec();
    return EXIT_SUCCESS;
}
