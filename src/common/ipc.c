/*
 * ipc.c - tworzenie, dołączanie i usuwanie struktur IPC (opis w ipc.h).
 */
#include "ipc.h"
#include "blad.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

/* Linux wymaga, żeby program sam zdefiniował unię dla semctl(). */
#ifndef __APPLE__
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
#endif

static key_t klucz(int id_projektu)
{
    return (key_t)SPRAWDZ(ftok(PLIK_KLUCZA, id_projektu));
}

/*
 * Wspólny schemat tworzenia: próba z IPC_EXCL, a gdy struktura już istnieje,
 * usunięcie pozostałości i druga próba. Funkcje pobierz/usun różnią się dla
 * semaforów, pamięci i kolejek, dlatego są przekazywane jako argumenty.
 */
typedef int (*Pobierz)(key_t klucz, int flagi);
typedef int (*Usun)(int id);

static int utworz_struktura(key_t k, Pobierz pobierz, Usun usun, const char *nazwa)
{
    int id = pobierz(k, IPC_CREAT | IPC_EXCL | PRAWA_IPC);
    if (id != -1)
        return id;
    if (errno != EEXIST)
        KRYTYCZNY(nazwa);

    /* Pozostałość po przerwanym przebiegu: usuwamy i tworzymy od nowa. */
    fprintf(stderr, "Usuwam pozostałość po poprzednim przebiegu: %s\n", nazwa);
    int stary = (int)SPRAWDZ(pobierz(k, PRAWA_IPC));
    SPRAWDZ(usun(stary));
    return (int)SPRAWDZ(pobierz(k, IPC_CREAT | IPC_EXCL | PRAWA_IPC));
}

/*
 * Przy dołączaniu do istniejącej struktury rozmiar wynosi 0: pozostałość po
 * starszej wersji programu może mieć inny rozmiar, a semget/shmget z większym
 * rozmiarem niż istniejący zwróciłoby EINVAL.
 */
static int pobierz_semafory(key_t k, int flagi)
{
    return semget(k, (flagi & IPC_CREAT) ? SEM_LICZBA : 0, flagi);
}

static int pobierz_pamiec(key_t k, int flagi)
{
    return shmget(k, (flagi & IPC_CREAT) ? sizeof(StanWspolny) : 0, flagi);
}

static int pobierz_kolejke(key_t k, int flagi)  { return msgget(k, flagi); }
static int usun_semafory(int id) { return semctl(id, 0, IPC_RMID); }
static int usun_pamiec(int id)   { return shmctl(id, IPC_RMID, NULL); }
static int usun_kolejke(int id)  { return msgctl(id, IPC_RMID, NULL); }

static StanWspolny *dolacz_pamiec(int id)
{
    void *adres = shmat(id, NULL, 0);
    if (adres == (void *)-1)
        KRYTYCZNY("shmat");
    return adres;
}

void ipc_utworz(ZasobyIpc *z)
{
    z->klucz_semafory = klucz(FTOK_SEMAFORY);
    z->klucz_pamiec = klucz(FTOK_PAMIEC);
    z->klucz_kasa = klucz(FTOK_KASA);
    z->semafory = utworz_struktura(z->klucz_semafory, pobierz_semafory, usun_semafory, "semafory");
    z->pamiec = utworz_struktura(z->klucz_pamiec, pobierz_pamiec, usun_pamiec, "pamięć dzielona");
    z->kasa = utworz_struktura(z->klucz_kasa, pobierz_kolejke, usun_kolejke, "kolejka kasy");

    /* Wartości początkowe semaforów: muteks otwarty. */
    unsigned short wartosci[SEM_LICZBA];
    wartosci[SEM_STAN] = 1;
    union semun arg = { .array = wartosci };
    SPRAWDZ(semctl(z->semafory, 0, SETALL, arg));

    z->stan = dolacz_pamiec(z->pamiec);
    memset(z->stan, 0, sizeof *z->stan);
}

void ipc_dolacz(ZasobyIpc *z)
{
    z->klucz_semafory = klucz(FTOK_SEMAFORY);
    z->klucz_pamiec = klucz(FTOK_PAMIEC);
    z->klucz_kasa = klucz(FTOK_KASA);
    z->semafory = (int)SPRAWDZ(semget(z->klucz_semafory, SEM_LICZBA, PRAWA_IPC));
    z->pamiec = (int)SPRAWDZ(shmget(z->klucz_pamiec, sizeof(StanWspolny), PRAWA_IPC));
    z->kasa = (int)SPRAWDZ(msgget(z->klucz_kasa, PRAWA_IPC));
    z->stan = dolacz_pamiec(z->pamiec);
}

void ipc_odlacz(ZasobyIpc *z)
{
    if (z->stan != NULL && shmdt(z->stan) == -1)
        ZGLOS("shmdt");
    z->stan = NULL;
}

void ipc_usun(ZasobyIpc *z)
{
    ipc_odlacz(z);
    /* Przy sprzątaniu nie przerywamy na pierwszym błędzie: usuwamy, co się da. */
    if (semctl(z->semafory, 0, IPC_RMID) == -1)
        ZGLOS("semctl(IPC_RMID)");
    if (shmctl(z->pamiec, IPC_RMID, NULL) == -1)
        ZGLOS("shmctl(IPC_RMID)");
    if (msgctl(z->kasa, IPC_RMID, NULL) == -1)
        ZGLOS("msgctl(IPC_RMID)");
}

static int sem_zmien(int semafory, int numer, short zmiana, short flagi)
{
    struct sembuf operacja = {
        .sem_num = (unsigned short)numer,
        .sem_op = zmiana,
        .sem_flg = flagi,
    };
    return semop(semafory, &operacja, 1);
}

int sem_p(int semafory, int numer, short flagi) { return sem_zmien(semafory, numer, -1, flagi); }
int sem_v(int semafory, int numer, short flagi) { return sem_zmien(semafory, numer, +1, flagi); }

void stan_zablokuj(const ZasobyIpc *z)
{
    /* Muteks czekamy do skutku: przerwanie sygnałem (EINTR) ponawia próbę. */
    while (sem_p(z->semafory, SEM_STAN, SEM_UNDO) == -1) {
        if (errno != EINTR)
            KRYTYCZNY("semop(SEM_STAN, -1)");
    }
}

void stan_odblokuj(const ZasobyIpc *z)
{
    SPRAWDZ(sem_v(z->semafory, SEM_STAN, SEM_UNDO));
}
