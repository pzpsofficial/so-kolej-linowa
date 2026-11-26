/*
 * ipc.h - tworzenie, dołączanie i usuwanie struktur IPC (System V).
 *
 * Właścicielem wszystkich struktur jest proces główny: tylko on je tworzy
 * (ipc_utworz) i tylko on je usuwa (ipc_usun). Pozostałe procesy dołączają
 * do istniejących struktur (ipc_dolacz) i przy wyjściu tylko się odłączają.
 */
#ifndef IPC_H
#define IPC_H

#include "typy.h"

typedef struct {
    int semafory;        /* identyfikator zestawu semaforów */
    int pamiec;          /* identyfikator segmentu pamięci dzielonej */
    int kasa;            /* identyfikator kolejki komunikatów kasy */
    StanWspolny *stan;   /* dołączony segment pamięci dzielonej */
} ZasobyIpc;

/*
 * Tworzy wszystkie struktury od zera. Jeśli struktura o tym kluczu już
 * istnieje, to jest to pozostałość po przerwanym przebiegu (proces główny
 * zginął od SIGKILL): zostaje usunięta i utworzona ponownie.
 * Wymaga istniejącego pliku PLIK_KLUCZA.
 */
void ipc_utworz(ZasobyIpc *z);

/* Dołącza do struktur utworzonych przez proces główny. */
void ipc_dolacz(ZasobyIpc *z);

/* Odłącza segment pamięci dzielonej (struktury zostają w systemie). */
void ipc_odlacz(ZasobyIpc *z);

/* Usuwa wszystkie struktury z systemu (tylko proces główny). */
void ipc_usun(ZasobyIpc *z);

/*
 * Operacje na pojedynczym semaforze. flagi: 0, IPC_NOWAIT, SEM_UNDO.
 * Zwracają 0 albo -1 z ustawionym errno (np. EINTR, gdy przerwał sygnał).
 */
int sem_p(int semafory, int numer, short flagi);   /* zajmij (-1) */
int sem_v(int semafory, int numer, short flagi);   /* zwolnij (+1) */

/*
 * Muteks na liczniki w pamięci dzielonej. Używa SEM_UNDO: gdy proces zginie
 * w sekcji krytycznej, jądro samo zwolni semafor.
 */
void stan_zablokuj(const ZasobyIpc *z);
void stan_odblokuj(const ZasobyIpc *z);

#endif
