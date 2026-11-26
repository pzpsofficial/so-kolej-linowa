/*
 * typy.h - stałe i struktury wspólne dla wszystkich procesów symulacji.
 */
#ifndef TYPY_H
#define TYPY_H

#include <sys/types.h>
#include <time.h>

/*
 * Plik, z którego ftok() buduje klucze IPC. Tworzy go proces główny w katalogu
 * uruchomienia, a procesy potomne dziedziczą ten katalog, więc liczą te same
 * klucze. Plik służy też jako blokada: w jednym katalogu działa naraz tylko
 * jedna symulacja.
 */
#define PLIK_KLUCZA "kolej.klucz"

#define KATALOG_LOGOW "logs"
#define PLIK_LOGU     "logs/symulacja.log"

/* Identyfikatory projektu dla ftok(): jeden klucz na każdą strukturę IPC. */
#define FTOK_SEMAFORY 'S'
#define FTOK_PAMIEC   'M'
#define FTOK_KASA     'K'

/* Prawa dostępu do struktur IPC: tylko właściciel czyta i pisze. */
#define PRAWA_IPC 0600

/* Numery semaforów w zestawie. */
enum {
    SEM_STAN = 0,   /* muteks chroniący liczniki w pamięci dzielonej */
    SEM_LICZBA
};

/* Parametry symulacji podane przy uruchomieniu (opis w config.c). */
typedef struct {
    int liczba_turystow;   /* ilu turystów przyjdzie w ciągu dnia */
    int max_procesow;      /* ilu turystów może działać naraz */
    int limit_n;           /* N: limit osób na terenie dolnej stacji */
    int tp, tk;            /* godziny pracy kolei, w minutach od północy */
    int skala;             /* ile minut czasu stacji trwa 1 s rzeczywista */
    int tk1, tk2, tk3;     /* czas ważności karnetów czasowych, w minutach */
    int t1, t2, t3;        /* średni czas zjazdu trasami 1-3, w minutach */
    unsigned int ziarno;   /* ziarno generatora liczb losowych */
    int cichy;             /* 1 = nie wypisuj logu na terminal */
} Konfiguracja;

/* Stan symulacji w pamięci dzielonej. */
typedef struct {
    Konfiguracja konf;          /* kopia parametrów dla procesów potomnych */
    struct timespec start;      /* chwila Tp, zegar CLOCK_MONOTONIC */
    pid_t pid_main;             /* proces główny */
    pid_t grupa;                /* grupa procesów symulacji */
    _Atomic int koniec;         /* 1 = symulacja się kończy */

    /* Liczniki, zmieniane tylko pod semaforem SEM_STAN. */
    int turysci_utworzeni;
    int turysci_zakonczeni;
} StanWspolny;

/* Typy komunikatów w kolejce kasy. */
#define KOM_KONIEC 1L   /* proces główny -> kasjer: zakończ pracę */

typedef struct {
    long mtype;
    pid_t nadawca;
    int dane;
} KomunikatKasa;

#endif
