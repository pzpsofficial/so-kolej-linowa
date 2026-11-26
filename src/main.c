/*
 * main.c - proces główny symulacji kolei linowej.
 */
#include "common/config.h"

#include <stdlib.h>

int main(int argc, char **argv)
{
    Konfiguracja konf;
    int wynik = konfiguracja_wczytaj(&konf, argc, argv);
    if (wynik != 0)
        return wynik > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    return EXIT_SUCCESS;
}
