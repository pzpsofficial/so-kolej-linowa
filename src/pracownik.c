/*
 * pracownik.c - proces pracownika stacji.
 *
 * Uruchamiany jako „pracownik 1” (stacja dolna) albo „pracownik 2” (stacja
 * górna). Czeka w sigsuspend() na sygnały, więc nie zużywa procesora.
 *
 * Etap szkieletu: pracownik tylko melduje gotowość i kończy pracę po
 * SIGTERM/SIGINT. Zatrzymywanie kolei (SIGUSR1/SIGUSR2) dojdzie później.
 */
#include "common/blad.h"
#include "common/ipc.h"
#include "common/log.h"
#include "common/proces.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t koniec = 0;

static void obsluz_koniec(int sygnal)
{
    (void)sygnal;
    koniec = 1;
}

int main(int argc, char **argv)
{
    if (argc != 2 || (strcmp(argv[1], "1") != 0 && strcmp(argv[1], "2") != 0)) {
        blad_danych("użycie: %s 1|2 (numer pracownika)", argv[0]);
        return EXIT_FAILURE;
    }
    int numer = argv[1][0] - '0';
    proces_ustaw_katalog(argv[0]);

    ZasobyIpc zasoby;
    ipc_dolacz(&zasoby);
    log_start("pracownik", numer, 0);
    log_ustaw_stan(zasoby.stan);

    /* Sygnały zablokowane poza sigsuspend(): sprawdzenie flagi i zaśnięcie są atomowe. */
    sigset_t obslugiwane, maska_czekania;
    SPRAWDZ(sigemptyset(&obslugiwane));
    SPRAWDZ(sigaddset(&obslugiwane, SIGTERM));
    SPRAWDZ(sigaddset(&obslugiwane, SIGINT));
    SPRAWDZ(sigprocmask(SIG_BLOCK, &obslugiwane, &maska_czekania));
    proces_obsluga_sygnalu(SIGTERM, obsluz_koniec);
    proces_obsluga_sygnalu(SIGINT, obsluz_koniec);

    log_zapisz(LOG_INFO, "stacja %s gotowa", numer == 1 ? "dolna" : "górna");
    while (!koniec)
        sigsuspend(&maska_czekania);

    log_zapisz(LOG_INFO, "koniec pracy");
    ipc_odlacz(&zasoby);
    log_koniec();
    return EXIT_SUCCESS;
}
