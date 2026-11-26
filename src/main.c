/*
 * main.c - proces główny symulacji kolei linowej.
 *
 * Zadania procesu głównego:
 *   1. wczytuje i sprawdza parametry,
 *   2. tworzy plik logu i struktury IPC (jest ich jedynym właścicielem),
 *   3. uruchamia procesy obsługi przez fork() + exec(): kasjera, pracownika1,
 *      pracownika2 i generator turystów,
 *   4. pilnuje godzin pracy kolei (od Tp do Tk),
 *   5. na koniec dnia albo po Ctrl+C kończy wszystkie procesy, czeka na każdy
 *      z nich i usuwa struktury IPC.
 *
 * Proces główny nie czeka aktywnie. Śpi w sigsuspend(), a budzą go sygnały:
 *   SIGALRM          - minął czas (koniec dnia albo limit czasu na zakończenie),
 *   SIGCHLD          - zakończył się któryś proces potomny,
 *   SIGINT, SIGTERM  - przerwanie symulacji (np. Ctrl+C).
 *
 * Pseudokod:
 *   zablokuj sygnały, ustaw ich obsługę
 *   uruchom procesy obsługi (wszystkie w jednej, nowej grupie procesów)
 *   nastaw minutnik na długość dnia
 *   dopóki nie koniec dnia i nie przerwano:
 *       sigsuspend()                    -- tu proces śpi
 *       odbierz zakończone dzieci       -- brak zombie
 *   zakończ symulację:
 *       ustaw flagę końca, wyślij kasjerowi KOM_KONIEC, wyślij SIGTERM do grupy
 *       czekaj, aż nie będzie żadnego dziecka; po 5 s wyślij SIGKILL do grupy
 *   atexit: usuń IPC, plik klucza, zamknij log
 */
#include "common/blad.h"
#include "common/config.h"
#include "common/ipc.h"
#include "common/log.h"
#include "common/proces.h"
#include "common/zegar.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/msg.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

/* Ile sekund procesy mają na zakończenie po SIGTERM, zanim dostaną SIGKILL. */
#define CZAS_NA_ZAKONCZENIE 5.0

typedef struct {
    const char *nazwa;   /* nazwa w logu */
    char *argv[3];       /* argumenty programu, argv[0] = nazwa pliku w bin/ */
    int moze_wczesniej;  /* 1 = poprawne zakończenie przed Tk nie jest błędem */
    pid_t pid;           /* 0 = proces nie działa */
} ProcesObslugi;

static ProcesObslugi obsluga[] = {
    { "kasjer",     { "kasjer", NULL },         0, 0 },
    { "pracownik1", { "pracownik", "1", NULL }, 0, 0 },
    { "pracownik2", { "pracownik", "2", NULL }, 0, 0 },
    /* Generator kończy pracę, gdy wpuści wszystkich turystów i ich odbierze. */
    { "generator",  { "generator", NULL },      1, 0 },
};
#define LICZBA_OBSLUGI ((int)(sizeof obsluga / sizeof obsluga[0]))

static ZasobyIpc zasoby;
static int ipc_utworzone = 0;
static int plik_klucza = -1;
static pid_t pid_glowny;
static pid_t grupa = 0;   /* grupa procesów symulacji (PID kasjera) */

/* Flagi ustawiane przez obsługę sygnałów. */
static volatile sig_atomic_t sygnal_alarm = 0;
static volatile sig_atomic_t sygnal_dziecko = 0;
static volatile sig_atomic_t sygnal_przerwij = 0;

static void obsluz_sygnal(int sygnal)
{
    switch (sygnal) {
    case SIGALRM: sygnal_alarm = 1; break;
    case SIGCHLD: sygnal_dziecko = 1; break;
    default:      sygnal_przerwij = 1; break;   /* SIGINT, SIGTERM */
    }
}

/*
 * Tworzy plik klucza i zakłada na nim blokadę. Druga symulacja uruchomiona
 * w tym samym katalogu dostanie odmowę, zamiast usunąć struktury IPC pierwszej.
 */
static void zablokuj_katalog(void)
{
    if (mkdir(KATALOG_LOGOW, 0700) == -1 && errno != EEXIST)
        KRYTYCZNY("mkdir(" KATALOG_LOGOW ")");

    plik_klucza = (int)SPRAWDZ(creat(PLIK_KLUCZA, 0600));
    if (flock(plik_klucza, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK) {
            blad_danych("w tym katalogu działa już inna symulacja");
            exit(EXIT_FAILURE);
        }
        KRYTYCZNY("flock(" PLIK_KLUCZA ")");
    }
}

/* Funkcja z atexit(): sprzątanie przy każdym wyjściu procesu głównego. */
static void sprzatanie(void)
{
    if (getpid() != pid_glowny)
        return;   /* zabezpieczenie: dzieci nie sprzątają za proces główny */

    /* Wyjście po błędzie krytycznym: najpierw kończymy wszystkie procesy. */
    if (grupa > 0 && kill(-grupa, 0) == 0) {
        kill(-grupa, SIGKILL);
        while (waitpid(-1, NULL, 0) > 0 || errno == EINTR)
            ;
    }
    if (ipc_utworzone) {
        log_ustaw_stan(NULL);   /* pamięć dzielona zaraz zniknie */
        ipc_usun(&zasoby);
        ipc_utworzone = 0;
        if (unlink(PLIK_KLUCZA) == -1)
            ZGLOS("unlink(" PLIK_KLUCZA ")");
        log_zapisz(LOG_INFO, "struktury IPC usunięte");
    }
    if (plik_klucza != -1 && close(plik_klucza) == -1)
        ZGLOS("close(" PLIK_KLUCZA ")");
    log_koniec();
}

static void ustaw_minutnik(double sekundy)
{
    struct itimerval czas;
    memset(&czas, 0, sizeof czas);
    czas.it_value.tv_sec = (time_t)sekundy;
    czas.it_value.tv_usec = (suseconds_t)((sekundy - (double)(time_t)sekundy) * 1e6);
    SPRAWDZ(setitimer(ITIMER_REAL, &czas, NULL));
}

static void opisz_status(int status, char *bufor, size_t rozmiar)
{
    if (WIFEXITED(status))
        snprintf(bufor, rozmiar, "kod wyjścia %d", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        snprintf(bufor, rozmiar, "zabity sygnałem %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
    else
        snprintf(bufor, rozmiar, "status %d", status);
}

/*
 * Odbiera wszystkie zakończone procesy potomne (bez blokowania).
 * Proces główny jest „subreaperem”, więc trafiają tu też osieroceni
 * turyści, jeśli ich rodzic (generator) zginął.
 * Zwraca 1, gdy jakieś dziecko jeszcze działa, 0 gdy nie ma już żadnego.
 */
static int odbierz_dzieci(void)
{
    for (;;) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0)
            return 1;
        if (pid == -1) {
            if (errno == ECHILD)
                return 0;
            if (errno == EINTR)
                continue;
            KRYTYCZNY("waitpid");
        }
        for (int i = 0; i < LICZBA_OBSLUGI; i++) {
            if (obsluga[i].pid != pid)
                continue;
            char opis[96];
            opisz_status(status, opis, sizeof opis);
            obsluga[i].pid = 0;
            int poprawnie = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            if (zasoby.stan->koniec || (obsluga[i].moze_wczesniej && poprawnie)) {
                log_zapisz(LOG_INFO, "%s zakończył pracę (%s)", obsluga[i].nazwa, opis);
            } else {
                log_zapisz(LOG_BLAD, "%s zakończył się przed końcem dnia (%s), kończę symulację",
                           obsluga[i].nazwa, opis);
                sygnal_przerwij = 1;
            }
        }
    }
}

static void uruchom_obsluge(const sigset_t *maska)
{
    for (int i = 0; i < LICZBA_OBSLUGI; i++) {
        /* Pierwszy proces zakłada nową grupę, kolejne do niej dołączają. */
        pid_t pid = proces_uruchom(obsluga[i].argv, grupa, maska);
        if (pid == -1) {
            ZGLOS("fork");
            log_zapisz(LOG_BLAD, "nie udało się uruchomić: %s", obsluga[i].nazwa);
            exit(EXIT_FAILURE);   /* sprzatanie() zakończy już uruchomione */
        }
        obsluga[i].pid = pid;
        if (grupa == 0) {
            grupa = pid;
            zasoby.stan->grupa = grupa;
        }
        log_zapisz(LOG_INFO, "uruchomiono %s (pid %d)", obsluga[i].nazwa, (int)pid);
    }
}

static void zakoncz_symulacje(const sigset_t *maska)
{
    zasoby.stan->koniec = 1;

    /* Kasjer śpi w msgrcv(): budzimy go komunikatem. */
    KomunikatKasa koniec = { .mtype = KOM_KONIEC, .nadawca = pid_glowny, .dane = 0 };
    if (msgsnd(zasoby.kasa, &koniec, sizeof koniec - sizeof koniec.mtype, IPC_NOWAIT) == -1)
        ZGLOS("msgsnd(KOM_KONIEC)");

    if (kill(-grupa, SIGTERM) == -1 && errno != ESRCH)
        ZGLOS("kill(SIGTERM)");

    sygnal_alarm = 0;
    ustaw_minutnik(CZAS_NA_ZAKONCZENIE);
    int wyslano_sigkill = 0;

    while (odbierz_dzieci()) {
        if (sygnal_alarm && !wyslano_sigkill) {
            log_zapisz(LOG_UWAGA, "procesy nie zakończyły się w %.0f s, wysyłam SIGKILL", CZAS_NA_ZAKONCZENIE);
            if (kill(-grupa, SIGKILL) == -1 && errno != ESRCH)
                ZGLOS("kill(SIGKILL)");
            wyslano_sigkill = 1;
        }
        sigsuspend(maska);   /* zawsze zwraca -1 z EINTR: obudził nas sygnał */
    }
    ustaw_minutnik(0);
}

int main(int argc, char **argv)
{
    Konfiguracja konf;
    int wynik = konfiguracja_wczytaj(&konf, argc, argv);
    if (wynik != 0)
        return wynik > 0 ? EXIT_SUCCESS : EXIT_FAILURE;

    proces_ustaw_katalog(argv[0]);
    pid_glowny = getpid();
    zablokuj_katalog();
    log_start("main", 0, 1);
    if (atexit(sprzatanie) != 0) {
        fputs("atexit: nie udało się zarejestrować sprzątania\n", stderr);
        return EXIT_FAILURE;
    }

    ipc_utworz(&zasoby);
    ipc_utworzone = 1;
    zasoby.stan->konf = konf;
    zasoby.stan->pid_main = pid_glowny;
    zegar_start(zasoby.stan);
    log_ustaw_stan(zasoby.stan);
    konfiguracja_do_logu(&konf);

    /* Osieroceni potomkowie (np. turyści po śmierci generatora) trafią do nas. */
    SPRAWDZ(prctl(PR_SET_CHILD_SUBREAPER, 1));

    /*
     * Sygnały są zablokowane poza sigsuspend(). Dzięki temu sygnał nie może
     * przyjść między sprawdzeniem flagi a zaśnięciem (nie ma zgubionych pobudek).
     */
    sigset_t obslugiwane, maska_czekania;
    SPRAWDZ(sigemptyset(&obslugiwane));
    SPRAWDZ(sigaddset(&obslugiwane, SIGINT));
    SPRAWDZ(sigaddset(&obslugiwane, SIGTERM));
    SPRAWDZ(sigaddset(&obslugiwane, SIGALRM));
    SPRAWDZ(sigaddset(&obslugiwane, SIGCHLD));
    SPRAWDZ(sigprocmask(SIG_BLOCK, &obslugiwane, &maska_czekania));
    proces_obsluga_sygnalu(SIGINT, obsluz_sygnal);
    proces_obsluga_sygnalu(SIGTERM, obsluz_sygnal);
    proces_obsluga_sygnalu(SIGALRM, obsluz_sygnal);
    proces_obsluga_sygnalu(SIGCHLD, obsluz_sygnal);

    uruchom_obsluge(&maska_czekania);

    double dlugosc_dnia = (double)(konf.tk - konf.tp) / konf.skala;
    char tk[6];
    zegar_format(konf.tk, tk);
    log_zapisz(LOG_INFO, "kolej otwarta, zamknięcie o %s (za %.1f s)", tk, dlugosc_dnia);
    ustaw_minutnik(dlugosc_dnia);

    while (!sygnal_alarm && !sygnal_przerwij) {
        sigsuspend(&maska_czekania);
        if (sygnal_dziecko) {
            sygnal_dziecko = 0;
            odbierz_dzieci();
        }
    }

    if (sygnal_przerwij)
        log_zapisz(LOG_UWAGA, "przerwanie symulacji, kończę wszystkie procesy");
    else
        log_zapisz(LOG_INFO, "godzina Tk: koniec dnia");
    zakoncz_symulacje(&maska_czekania);

    log_zapisz(LOG_INFO, "podsumowanie: turystów przyszło %d, zakończyło wizytę %d",
               zasoby.stan->turysci_utworzeni, zasoby.stan->turysci_zakonczeni);
    log_zapisz(LOG_INFO, "symulacja zakończona");
    return EXIT_SUCCESS;   /* resztę sprząta sprzatanie() z atexit() */
}
