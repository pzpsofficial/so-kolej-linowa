/*
 * blad.c - wspólna obsługa błędów (opis w blad.h).
 */
#include "blad.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int blad_zglos(const char *plik, int linia, const char *wywolanie)
{
    int numer = errno;   /* fprintf/snprintf mogą zmienić errno */
    char kontekst[256];

    snprintf(kontekst, sizeof kontekst, "%s:%d: %s", plik, linia, wywolanie);
    errno = numer;
    perror(kontekst);

    errno = numer;
    return numer;
}

_Noreturn void blad_krytyczny(const char *plik, int linia, const char *wywolanie)
{
    blad_zglos(plik, linia, wywolanie);
    exit(EXIT_FAILURE);
}

long blad_sprawdz(long wynik, const char *plik, int linia, const char *wywolanie)
{
    if (wynik == -1)
        blad_krytyczny(plik, linia, wywolanie);
    return wynik;
}

void blad_danych(const char *format, ...)
{
    va_list argumenty;

    fputs("Błędne dane: ", stderr);
    va_start(argumenty, format);
    vfprintf(stderr, format, argumenty);
    va_end(argumenty);
    fputc('\n', stderr);
}
