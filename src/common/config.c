/*
 * config.c - parametry symulacji (opis w config.h).
 */
#include "config.h"
#include "blad.h"
#include "log.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

/* Zapas procesów na kasjera, pracowników, generator i proces główny. */
#define PROCESY_STALE 5

static void domyslne(Konfiguracja *k)
{
    k->liczba_turystow = 200;
    k->max_procesow = 300;
    k->limit_n = 50;
    k->tp = 8 * 60;
    k->tk = 18 * 60;
    k->skala = 10;            /* 10 godzin stacji = 60 s */
    k->tk1 = 60;
    k->tk2 = 120;
    k->tk3 = 240;
    k->t1 = 10;
    k->t2 = 20;
    k->t3 = 30;
    k->ziarno = (unsigned int)time(NULL);
    k->cichy = 0;
}

/* Liczba całkowita z zakresu [min, max]. */
static int liczba(const char *tekst, long min, long max, const char *nazwa, int *wynik)
{
    char *koniec;
    errno = 0;
    long w = strtol(tekst, &koniec, 10);
    if (errno != 0 || koniec == tekst || *koniec != '\0') {
        blad_danych("%s: \"%s\" nie jest liczbą całkowitą", nazwa, tekst);
        return -1;
    }
    if (w < min || w > max) {
        blad_danych("%s: %ld spoza zakresu %ld..%ld", nazwa, w, min, max);
        return -1;
    }
    *wynik = (int)w;
    return 0;
}

/* Godzina "HH:MM" zamieniona na minuty od północy. */
static int godzina(const char *tekst, const char *nazwa, int *wynik)
{
    int h, m;
    char reszta;
    if (sscanf(tekst, "%d:%d%c", &h, &m, &reszta) != 2 || h < 0 || h > 23 || m < 0 || m > 59) {
        blad_danych("%s: \"%s\" nie jest godziną w formacie HH:MM", nazwa, tekst);
        return -1;
    }
    *wynik = h * 60 + m;
    return 0;
}

/* Trzy rosnące liczby dodatnie, np. "10,20,30". */
static int trzy_rosnace(const char *tekst, const char *nazwa, int *a, int *b, int *c)
{
    char reszta;
    if (sscanf(tekst, "%d,%d,%d%c", a, b, c, &reszta) != 3) {
        blad_danych("%s: \"%s\" to nie trzy liczby oddzielone przecinkami", nazwa, tekst);
        return -1;
    }
    if (*a <= 0 || *a >= *b || *b >= *c) {
        blad_danych("%s: wymagane 0 < pierwsza < druga < trzecia, podano %d,%d,%d", nazwa, *a, *b, *c);
        return -1;
    }
    return 0;
}

/* Sprawdzenia wymagające kilku parametrów naraz. */
static int sprawdz_calosc(const Konfiguracja *k)
{
    if (k->tp >= k->tk) {
        blad_danych("godzina otwarcia Tp musi być wcześniejsza niż godzina zamknięcia Tk");
        return -1;
    }

    /*
     * Limit procesów użytkownika (ulimit -u). Jeśli turystów naraz miałoby być
     * więcej, niż system pozwala uruchomić, odmawiamy od razu, zamiast czekać,
     * aż fork() zacznie zwracać błędy.
     */
    struct rlimit limit;
    SPRAWDZ(getrlimit(RLIMIT_NPROC, &limit));
    if (limit.rlim_cur != RLIM_INFINITY
        && (rlim_t)(k->max_procesow + PROCESY_STALE) > limit.rlim_cur) {
        blad_danych("maksymalna liczba turystów naraz (%d) + %d procesów obsługi "
                    "przekracza limit procesów użytkownika (ulimit -u = %lu)",
                    k->max_procesow, PROCESY_STALE, (unsigned long)limit.rlim_cur);
        return -1;
    }
    return 0;
}

void konfiguracja_pomoc(const char *program)
{
    printf("Użycie: %s [opcje]\n"
           "\n"
           "  -n LICZBA    liczba turystów w ciągu dnia (1..100000, domyślnie 200)\n"
           "  -p LICZBA    ilu turystów może działać naraz (1..20000, domyślnie 300)\n"
           "  -N LICZBA    limit N osób na terenie dolnej stacji (1..10000, domyślnie 50)\n"
           "  -o HH:MM     godzina otwarcia Tp (domyślnie 08:00)\n"
           "  -z HH:MM     godzina zamknięcia Tk (domyślnie 18:00)\n"
           "  -k LICZBA    skala: ile minut stacji trwa 1 s (1..600, domyślnie 10)\n"
           "  -a A,B,C     ważność karnetów Tk1,Tk2,Tk3 w minutach (domyślnie 60,120,240)\n"
           "  -t A,B,C     średni czas zjazdu trasami T1,T2,T3 w minutach (domyślnie 10,20,30)\n"
           "  -s LICZBA    ziarno liczb losowych (domyślnie z zegara)\n"
           "  -q           tryb cichy: na terminal tylko uwagi i błędy\n"
           "  -h           ta pomoc\n",
           program);
}

int konfiguracja_wczytaj(Konfiguracja *k, int argc, char **argv)
{
    domyslne(k);

    int opcja;
    int ziarno;
    opterr = 0;   /* komunikaty o złych opcjach wypisujemy sami */
    while ((opcja = getopt(argc, argv, "n:p:N:o:z:k:a:t:s:qh")) != -1) {
        int wynik = 0;
        switch (opcja) {
        case 'n': wynik = liczba(optarg, 1, 100000, "-n (liczba turystów)", &k->liczba_turystow); break;
        case 'p': wynik = liczba(optarg, 1, 20000, "-p (turystów naraz)", &k->max_procesow); break;
        case 'N': wynik = liczba(optarg, 1, 10000, "-N (limit N)", &k->limit_n); break;
        case 'o': wynik = godzina(optarg, "-o (Tp)", &k->tp); break;
        case 'z': wynik = godzina(optarg, "-z (Tk)", &k->tk); break;
        case 'k': wynik = liczba(optarg, 1, 600, "-k (skala)", &k->skala); break;
        case 'a': wynik = trzy_rosnace(optarg, "-a (Tk1,Tk2,Tk3)", &k->tk1, &k->tk2, &k->tk3); break;
        case 't': wynik = trzy_rosnace(optarg, "-t (T1,T2,T3)", &k->t1, &k->t2, &k->t3); break;
        case 's':
            wynik = liczba(optarg, 0, INT_MAX, "-s (ziarno)", &ziarno);
            k->ziarno = (unsigned int)ziarno;
            break;
        case 'q': k->cichy = 1; break;
        case 'h': konfiguracja_pomoc(argv[0]); return 1;
        default:
            blad_danych("nieznana opcja albo brak wartości: -%c (pomoc: %s -h)", optopt, argv[0]);
            return -1;
        }
        if (wynik != 0)
            return -1;
    }
    if (optind < argc) {
        blad_danych("nadmiarowy argument \"%s\" (pomoc: %s -h)", argv[optind], argv[0]);
        return -1;
    }
    return sprawdz_calosc(k);
}

void konfiguracja_do_logu(const Konfiguracja *k)
{
    log_zapisz(LOG_INFO, "parametry: turystów %d (naraz do %d), N = %d, Tp %02d:%02d, Tk %02d:%02d, "
               "skala %d min/s, Tk1-Tk3 %d/%d/%d min, T1-T3 %d/%d/%d min, ziarno %u",
               k->liczba_turystow, k->max_procesow, k->limit_n,
               k->tp / 60, k->tp % 60, k->tk / 60, k->tk % 60, k->skala,
               k->tk1, k->tk2, k->tk3, k->t1, k->t2, k->t3, k->ziarno);
}
