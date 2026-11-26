/*
 * proces.c - uruchamianie procesów symulacji (opis w proces.h).
 */
#include "proces.h"
#include "blad.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

/* Najwięcej argumentów programu (razem z argv[0]). */
#define MAX_ARGUMENTOW 8

static char katalog[512] = ".";
static char sciezka[600];

void proces_ustaw_katalog(const char *argv0)
{
    const char *ukosnik = strrchr(argv0, '/');
    if (ukosnik == NULL) {
        snprintf(katalog, sizeof katalog, ".");
        return;
    }
    snprintf(katalog, sizeof katalog, "%.*s", (int)(ukosnik - argv0), argv0);
}

const char *proces_sciezka(const char *nazwa)
{
    snprintf(sciezka, sizeof sciezka, "%s/%s", katalog, nazwa);
    return sciezka;
}

pid_t proces_uruchom(char *const argv[], pid_t grupa, const sigset_t *maska)
{
    pid_t rodzic = getpid();
    const char *program = proces_sciezka(argv[0]);

    pid_t pid = fork();
    if (pid == -1)
        return -1;

    if (pid == 0) {
        /*
         * Proces potomny. Do exec() nie wolno wołać exit() ani SPRAWDZ():
         * exit() uruchomiłoby funkcje atexit() rodzica (np. usunięcie IPC).
         * Dlatego przy błędzie tylko _exit().
         */
        if (grupa >= 0 && setpgid(0, grupa) == -1) {
            ZGLOS("setpgid(dziecko)");
            _exit(127);
        }
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) {
            ZGLOS("prctl(PR_SET_PDEATHSIG)");
            _exit(127);
        }
        /* Rodzic mógł zginąć, zanim ustawiliśmy PDEATHSIG. */
        if (getppid() != rodzic)
            _exit(127);
        /* exec() zachowuje maskę sygnałów, więc przywracamy ją jawnie. */
        if (maska != NULL && sigprocmask(SIG_SETMASK, maska, NULL) == -1) {
            ZGLOS("sigprocmask(dziecko)");
            _exit(127);
        }
        /*
         * argv[0] dziecka to pełna ścieżka programu: na jej podstawie dziecko
         * znajduje pozostałe programy (np. generator znajduje "turysta").
         */
        char *argumenty[MAX_ARGUMENTOW + 1];
        int n = 0;
        argumenty[n++] = (char *)program;
        while (n < MAX_ARGUMENTOW && argv[n] != NULL) {
            argumenty[n] = argv[n];
            n++;
        }
        argumenty[n] = NULL;
        execv(program, argumenty);
        ZGLOS(program);   /* execv() wraca tylko przy błędzie */
        _exit(127);
    }

    /*
     * Rodzic też ustawia grupę dziecka: nie wiadomo, kto zdąży pierwszy,
     * a sygnał do grupy może pójść zaraz po fork(). EACCES oznacza, że dziecko
     * już wykonało exec() i samo ustawiło grupę, więc to nie jest błąd.
     */
    if (grupa >= 0 && setpgid(pid, grupa == 0 ? pid : grupa) == -1 && errno != EACCES)
        ZGLOS("setpgid(rodzic)");
    return pid;
}

void proces_obsluga_sygnalu(int sygnal, void (*funkcja)(int))
{
    struct sigaction akcja;
    memset(&akcja, 0, sizeof akcja);
    akcja.sa_handler = funkcja;
    SPRAWDZ(sigemptyset(&akcja.sa_mask));
    akcja.sa_flags = 0;
    SPRAWDZ(sigaction(sygnal, &akcja, NULL));
}
