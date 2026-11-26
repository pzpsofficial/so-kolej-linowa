/*
 * proces.h - uruchamianie procesów symulacji przez fork() + exec().
 */
#ifndef PROCES_H
#define PROCES_H

#include <signal.h>
#include <sys/types.h>

/*
 * Zapamiętuje katalog z programami na podstawie argv[0] procesu.
 * Wszystkie programy symulacji leżą w tym samym katalogu (bin/).
 */
void proces_ustaw_katalog(const char *argv0);

/* Pełna ścieżka programu o podanej nazwie, np. "kasjer" -> "./bin/kasjer". */
const char *proces_sciezka(const char *nazwa);

/*
 * Uruchamia program w nowym procesie i zwraca jego PID (albo -1 z errno,
 * gdy fork() się nie udał, np. EAGAIN przy limicie procesów).
 *
 *   argv   - argumenty programu; argv[0] to nazwa programu,
 *   grupa  - grupa procesów dziecka: 0 = nowa grupa z PID dziecka,
 *            > 0 = dołącz do tej grupy, -1 = zostań w grupie rodzica,
 *   maska  - maska sygnałów przywracana w dziecku przed exec()
 *            (NULL = bez zmian).
 *
 * Dziecko ustawia PR_SET_PDEATHSIG: gdy rodzic zginie, jądro wyśle dziecku
 * SIGTERM. Dzięki temu po zabiciu procesu nadrzędnego nie zostają sieroty.
 */
pid_t proces_uruchom(char *const argv[], pid_t grupa, const sigset_t *maska);

/*
 * Ustawia obsługę sygnału przez sigaction(). Bez SA_RESTART: sygnał przerywa
 * blokujące wywołania (msgrcv, semop, nanosleep, waitpid) z errno = EINTR,
 * więc proces może od razu zareagować na polecenie zakończenia.
 */
void proces_obsluga_sygnalu(int sygnal, void (*funkcja)(int));

#endif
