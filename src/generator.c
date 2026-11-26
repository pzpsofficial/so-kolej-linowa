/*
 * generator.c - generator turystów.
 *
 * Tworzy procesy turystów (fork + exec) w losowych odstępach czasu.
 * Przyjścia rozkładają się na pierwszą połowę dnia: średni odstęp to
 * (Tk - Tp) / 2 / liczba_turystów minut czasu stacji.
 *
 * Generator pilnuje, żeby naraz działało najwyżej konf.max_procesow turystów.
 * Gdy limit jest osiągnięty, śpi w waitpid() do zakończenia któregoś z nich.
 * Gdy fork() zwraca EAGAIN (system odmówił utworzenia procesu, np. przez
 * ulimit -u), generator zapisuje to w logu, czeka na zakończenie któregoś
 * turysty i próbuje ponownie, zamiast przerywać symulację.
 * Wszystkich zakończonych turystów odbiera przez waitpid(), więc nie zostają
 * procesy zombie.
 *
 * Pseudokod:
 *   dopóki są turyści do utworzenia i nie ma końca symulacji:
 *       odbierz zakończonych turystów (bez czekania)
 *       jeśli działa max_procesow turystów: czekaj na jednego, od nowa
 *       fork() + exec("turysta")
 *           EAGAIN -> zapisz w logu, czekaj na jednego turystę, od nowa
 *       śpij losowy czas do następnego przyjścia
 *   czekaj na wszystkich pozostałych turystów
 */
#include "common/blad.h"
#include "common/ipc.h"
#include "common/log.h"
#include "common/proces.h"
#include "common/zegar.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

static volatile sig_atomic_t koniec = 0;
static int brak_dzieci = 0;   /* waitpid() zwrócił ECHILD */

static void obsluz_koniec(int sygnal)
{
    (void)sygnal;
    koniec = 1;
}

/*
 * Odbiera zakończonych turystów i zwraca ich liczbę.
 *   blokujaco = 0: odbiera tylko tych, którzy już się zakończyli,
 *   blokujaco = 1: jeśli nikt się jeszcze nie zakończył, czeka na jednego
 *                  (sygnał przerywa czekanie i funkcja zwraca 0).
 */
static int odbierz_turystow(int blokujaco)
{
    int odebrani = 0;
    for (;;) {
        int flagi = (blokujaco && odebrani == 0) ? 0 : WNOHANG;
        pid_t pid = waitpid(-1, NULL, flagi);
        if (pid > 0) {
            odebrani++;
            continue;
        }
        if (pid == 0)
            return odebrani;
        if (errno == ECHILD) {
            brak_dzieci = 1;
            return odebrani;
        }
        if (errno == EINTR)
            return odebrani;
        KRYTYCZNY("waitpid(turysta)");
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    proces_ustaw_katalog(argv[0]);

    ZasobyIpc zasoby;
    ipc_dolacz(&zasoby);
    log_start("generator", 0, 0);
    log_ustaw_stan(zasoby.stan);
    proces_obsluga_sygnalu(SIGTERM, obsluz_koniec);
    proces_obsluga_sygnalu(SIGINT, obsluz_koniec);

    const Konfiguracja *konf = &zasoby.stan->konf;
    srand(konf->ziarno);
    double sredni_odstep = (double)(konf->tk - konf->tp) / 2.0 / konf->liczba_turystow;

    int aktywni = 0, utworzeni = 0, max_naraz = 0, odmowy_fork = 0;

    while (utworzeni < konf->liczba_turystow && !koniec && !zasoby.stan->koniec) {
        aktywni -= odbierz_turystow(0);

        if (aktywni >= konf->max_procesow) {
            aktywni -= odbierz_turystow(1);
            continue;
        }

        char id[16];
        snprintf(id, sizeof id, "%d", utworzeni + 1);
        char *argumenty[] = { "turysta", id, NULL };

        pid_t pid = proces_uruchom(argumenty, -1, NULL);
        if (pid == -1) {
            if (errno != EAGAIN)
                KRYTYCZNY("fork(turysta)");
            ZGLOS("fork(turysta)");
            odmowy_fork++;
            log_zapisz(LOG_UWAGA, "system odmówił utworzenia procesu (EAGAIN), turystów w drodze: %d, czekam",
                       aktywni);
            if (aktywni > 0)
                aktywni -= odbierz_turystow(1);
            else
                zegar_spij(konf, 1.0);
            continue;
        }

        aktywni++;
        utworzeni++;
        if (aktywni > max_naraz)
            max_naraz = aktywni;
        stan_zablokuj(&zasoby);
        zasoby.stan->turysci_utworzeni++;
        stan_odblokuj(&zasoby);

        /* Losowy odstęp do następnego przyjścia, równomiernie z [0, 2 * średni]. */
        double odstep = sredni_odstep * 2.0 * rand() / RAND_MAX;
        zegar_spij(konf, odstep);   /* przerwanie sygnałem sprawdzi warunek pętli */
    }

    log_zapisz(LOG_INFO, "koniec wpuszczania: utworzono %d turystów, najwięcej naraz %d (limit %d), odmów fork(): %d",
               utworzeni, max_naraz, konf->max_procesow, odmowy_fork);

    while (aktywni > 0 && !brak_dzieci)
        aktywni -= odbierz_turystow(1);

    log_zapisz(LOG_INFO, "wszyscy turyści odebrani");
    ipc_odlacz(&zasoby);
    log_koniec();
    return EXIT_SUCCESS;
}
