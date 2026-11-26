/*
 * config.h - parametry symulacji: wartości domyślne, wczytanie z linii
 * poleceń i sprawdzenie poprawności.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include "typy.h"

/*
 * Wypełnia konfigurację wartościami domyślnymi, a potem nadpisuje je
 * opcjami z linii poleceń. Zwraca:
 *    0  konfiguracja poprawna,
 *    1  wypisano pomoc (-h), program powinien się zakończyć bez błędu,
 *   -1  błędne dane; komunikat został już wypisany.
 */
int konfiguracja_wczytaj(Konfiguracja *konf, int argc, char **argv);

/* Wypisuje opis opcji. */
void konfiguracja_pomoc(const char *program);

/* Zapisuje parametry do logu (na początku przebiegu). */
void konfiguracja_do_logu(const Konfiguracja *konf);

#endif
