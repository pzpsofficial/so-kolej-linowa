/*
 * blad.h - wspólna obsługa błędów.
 *
 * Każde wywołanie funkcji systemowej w projekcie przechodzi przez jedno z
 * makr poniżej. Dzięki temu komunikat o błędzie zawsze ma ten sam format:
 *
 *     plik.c:123: semget(klucz, 1, 0600): No such file or directory
 *
 * Opis błędu pochodzi z perror(), czyli z bieżącej wartości errno.
 *
 * Dwa rodzaje błędów:
 *   - krytyczny (SPRAWDZ): proces nie może dalej działać, więc kończy się
 *     przez exit(). Sprzątanie robią funkcje zarejestrowane w atexit().
 *   - do obsłużenia (ZGLOS): błąd zostaje wypisany, a wywołujący sam decyduje,
 *     co dalej, np. czeka i ponawia fork() po EAGAIN.
 */
#ifndef BLAD_H
#define BLAD_H

/*
 * Wypisuje błąd ostatniego wywołania systemowego przez perror().
 * Zachowuje errno, więc po powrocie można je dalej sprawdzać.
 * Zwraca wartość errno z chwili wywołania.
 */
int blad_zglos(const char *plik, int linia, const char *wywolanie);

/* Jak blad_zglos(), a potem kończy proces kodem EXIT_FAILURE. */
_Noreturn void blad_krytyczny(const char *plik, int linia, const char *wywolanie);

/*
 * Sprawdza wynik wywołania: -1 oznacza błąd krytyczny.
 * Zwraca wynik bez zmian, żeby dało się pisać:  int id = SPRAWDZ(semget(...));
 */
long blad_sprawdz(long wynik, const char *plik, int linia, const char *wywolanie);

/* Komunikat o niepoprawnych danych od użytkownika (bez errno). */
void blad_danych(const char *format, ...) __attribute__((format(printf, 1, 2)));

#define ZGLOS(wywolanie) blad_zglos(__FILE__, __LINE__, (wywolanie))
#define KRYTYCZNY(wywolanie) blad_krytyczny(__FILE__, __LINE__, (wywolanie))
#define SPRAWDZ(x) blad_sprawdz((long)(x), __FILE__, __LINE__, #x)

#endif
