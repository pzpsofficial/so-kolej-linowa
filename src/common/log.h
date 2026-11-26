/*
 * log.h - log przebiegu symulacji.
 *
 * Wszystkie procesy piszą do jednego pliku PLIK_LOGU otwartego z O_APPEND.
 * Każdy wiersz trafia do pliku jednym wywołaniem write(), więc wiersze
 * z różnych procesów się nie przeplatają. Ten sam wiersz, pokolorowany według
 * roli procesu, pojawia się na terminalu (chyba że wybrano tryb cichy).
 *
 * Format wiersza:  [08:15] kasjer      #0     pid 1234 | sprzedany bilet ...
 */
#ifndef LOG_H
#define LOG_H

#include "typy.h"

typedef enum {
    LOG_INFO,
    LOG_UWAGA,
    LOG_BLAD
} PoziomLogu;

/*
 * Otwiera plik logu. rola i id opisują proces w każdym wierszu.
 * obetnij = 1 czyści plik (robi to tylko proces główny na starcie).
 */
void log_start(const char *rola, int id, int obetnij);

/* Od tej chwili wiersze mają godzinę stacji i respektują tryb cichy. */
void log_ustaw_stan(const StanWspolny *stan);

/* Zapisuje jeden wiersz logu. */
void log_zapisz(PoziomLogu poziom, const char *format, ...) __attribute__((format(printf, 2, 3)));

/* Zamyka plik logu. */
void log_koniec(void);

#endif
