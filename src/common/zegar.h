/*
 * zegar.h - czas symulacji.
 *
 * Symulacja działa w czasie przeskalowanym: 1 sekunda rzeczywista to
 * konf.skala minut czasu stacji. Punktem odniesienia jest chwila startu
 * (godzina Tp) zapisana w pamięci dzielonej. Używamy CLOCK_MONOTONIC, który
 * jest wspólny dla wszystkich procesów i nie cofa się przy zmianie zegara.
 */
#ifndef ZEGAR_H
#define ZEGAR_H

#include "typy.h"

#include <time.h>

/* Zapisuje chwilę startu (godzina Tp) w pamięci dzielonej. */
void zegar_start(StanWspolny *stan);

/* Aktualna godzina stacji w minutach od północy. */
int zegar_minuty(const StanWspolny *stan);

/* Zapisuje godzinę w postaci "HH:MM" (bufor co najmniej 6 znaków). */
void zegar_format(int minuty, char *bufor);

/* Ile czasu rzeczywistego trwa podana liczba minut stacji. */
struct timespec zegar_rzeczywisty(const Konfiguracja *konf, double minuty);

/*
 * Śpi przez podaną liczbę minut stacji. Zwraca 0 po przespaniu całości
 * albo -1, gdy sen przerwał sygnał (errno = EINTR).
 */
int zegar_spij(const Konfiguracja *konf, double minuty);

#endif
