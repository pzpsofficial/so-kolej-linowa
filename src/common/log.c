/*
 * log.c - log przebiegu symulacji (opis w log.h).
 */
#include "log.h"
#include "blad.h"
#include "zegar.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define KOLOR_RESET "\033[0m"

static int plik = -1;
static int terminal_kolorowy = 0;
static char opis_procesu[48];
static const char *kolor_roli = "";
static const StanWspolny *stan_symulacji = NULL;

/* Każda rola ma własny kolor na terminalu. */
static const char *kolor_dla(const char *rola)
{
    static const struct { const char *rola; const char *kolor; } kolory[] = {
        { "main",      "\033[1;37m" },   /* biały, pogrubiony */
        { "kasjer",    "\033[0;33m" },   /* żółty */
        { "pracownik", "\033[0;36m" },   /* cyjan */
        { "generator", "\033[0;35m" },   /* fioletowy */
        { "turysta",   "\033[0;32m" },   /* zielony */
    };
    for (size_t i = 0; i < sizeof kolory / sizeof kolory[0]; i++)
        if (strcmp(rola, kolory[i].rola) == 0)
            return kolory[i].kolor;
    return "";
}

void log_start(const char *rola, int id, int obetnij)
{
    int flagi = O_WRONLY | O_CREAT | O_APPEND | (obetnij ? O_TRUNC : 0);
    plik = (int)SPRAWDZ(open(PLIK_LOGU, flagi, 0600));

    snprintf(opis_procesu, sizeof opis_procesu, "%-10s #%-5d pid %-7d", rola, id, (int)getpid());
    kolor_roli = kolor_dla(rola);
    terminal_kolorowy = isatty(STDOUT_FILENO);
}

void log_ustaw_stan(const StanWspolny *stan)
{
    stan_symulacji = stan;
}

void log_zapisz(PoziomLogu poziom, const char *format, ...)
{
    char godzina[6] = "--:--";
    if (stan_symulacji != NULL)
        zegar_format(zegar_minuty(stan_symulacji), godzina);

    const char *znacznik = poziom == LOG_BLAD ? "BŁĄD: " : poziom == LOG_UWAGA ? "UWAGA: " : "";

    char tresc[512];
    va_list argumenty;
    va_start(argumenty, format);
    vsnprintf(tresc, sizeof tresc, format, argumenty);
    va_end(argumenty);

    char wiersz[640];
    int dlugosc = snprintf(wiersz, sizeof wiersz, "[%s] %s| %s%s\n", godzina, opis_procesu, znacznik, tresc);
    if (dlugosc < 0)
        return;
    if ((size_t)dlugosc >= sizeof wiersz)
        dlugosc = sizeof wiersz - 1;   /* obcięty wiersz nadal kończy się '\n' */

    /* Jeden write() na wiersz: dzięki O_APPEND wiersze procesów się nie mieszają. */
    if (plik != -1 && write(plik, wiersz, (size_t)dlugosc) == -1)
        ZGLOS("write(log)");

    if (stan_symulacji != NULL && stan_symulacji->konf.cichy && poziom == LOG_INFO)
        return;
    if (terminal_kolorowy) {
        const char *kolor = poziom == LOG_BLAD ? "\033[1;31m" : poziom == LOG_UWAGA ? "\033[1;33m" : kolor_roli;
        dprintf(STDOUT_FILENO, "%s%.*s%s\n", kolor, dlugosc - 1, wiersz, KOLOR_RESET);
    } else if (write(STDOUT_FILENO, wiersz, (size_t)dlugosc) == -1) {
        ZGLOS("write(stdout)");
    }
}

void log_koniec(void)
{
    if (plik != -1 && close(plik) == -1)
        ZGLOS("close(log)");
    plik = -1;
}
