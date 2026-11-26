/*
 * zegar.c - czas symulacji (opis w zegar.h).
 */
#include "zegar.h"
#include "blad.h"

#include <stdio.h>

void zegar_start(StanWspolny *stan)
{
    SPRAWDZ(clock_gettime(CLOCK_MONOTONIC, &stan->start));
}

int zegar_minuty(const StanWspolny *stan)
{
    struct timespec teraz;
    SPRAWDZ(clock_gettime(CLOCK_MONOTONIC, &teraz));

    double sekundy = (double)(teraz.tv_sec - stan->start.tv_sec)
                   + (double)(teraz.tv_nsec - stan->start.tv_nsec) / 1e9;
    return stan->konf.tp + (int)(sekundy * stan->konf.skala);
}

void zegar_format(int minuty, char *bufor)
{
    unsigned int m = minuty < 0 ? 0u : (unsigned int)minuty;
    snprintf(bufor, 6, "%02u:%02u", (m / 60u) % 100u, m % 60u);
}

struct timespec zegar_rzeczywisty(const Konfiguracja *konf, double minuty)
{
    double sekundy = minuty / konf->skala;
    struct timespec t = {
        .tv_sec = (time_t)sekundy,
        .tv_nsec = (long)((sekundy - (double)(time_t)sekundy) * 1e9),
    };
    return t;
}

int zegar_spij(const Konfiguracja *konf, double minuty)
{
    struct timespec t = zegar_rzeczywisty(konf, minuty);
    return nanosleep(&t, NULL);
}
