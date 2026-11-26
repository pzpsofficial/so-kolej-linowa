/*
 * turysta.c - proces turysty.
 *
 * Etap szkieletu: turysta przychodzi na teren stacji, zostaje tam losowy
 * czas (5-60 minut czasu stacji) i odchodzi. Bilety, bramki, krzesełka
 * i zjazdy dojdą w kolejnych krokach.
 */
#include "common/blad.h"
#include "common/ipc.h"
#include "common/log.h"
#include "common/proces.h"
#include "common/zegar.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t koniec = 0;

static void obsluz_koniec(int sygnal)
{
    (void)sygnal;
    koniec = 1;
}

int main(int argc, char **argv)
{
    char *reszta = NULL;
    long id = argc == 2 ? strtol(argv[1], &reszta, 10) : 0;
    if (argc != 2 || *reszta != '\0' || id <= 0) {
        blad_danych("użycie: %s NUMER (numer turysty > 0)", argv[0]);
        return EXIT_FAILURE;
    }
    proces_ustaw_katalog(argv[0]);

    ZasobyIpc zasoby;
    ipc_dolacz(&zasoby);
    log_start("turysta", (int)id, 0);
    log_ustaw_stan(zasoby.stan);
    proces_obsluga_sygnalu(SIGTERM, obsluz_koniec);
    proces_obsluga_sygnalu(SIGINT, obsluz_koniec);

    const Konfiguracja *konf = &zasoby.stan->konf;
    srand(konf->ziarno ^ ((unsigned int)getpid() * 2654435761u));

    log_zapisz(LOG_INFO, "przychodzę na stację");
    int wizyta = 5 + rand() % 56;
    if (zegar_spij(konf, wizyta) == -1 && errno == EINTR && koniec)
        log_zapisz(LOG_INFO, "stacja się zamyka, wychodzę wcześniej");
    else
        log_zapisz(LOG_INFO, "wychodzę po %d min", wizyta);

    stan_zablokuj(&zasoby);
    zasoby.stan->turysci_zakonczeni++;
    stan_odblokuj(&zasoby);

    ipc_odlacz(&zasoby);
    log_koniec();
    return EXIT_SUCCESS;
}
