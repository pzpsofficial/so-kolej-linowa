#!/bin/bash
# lib.sh - wspólne funkcje testów (dołączane przez "source").
#
# Każdy test uruchamia symulację w osobnym, pustym katalogu tymczasowym,
# więc plik klucza i logi nie mieszają się między testami.
# Po każdym przebiegu sprawdzamy to samo:
#   - nie działa żaden proces symulacji,
#   - nie ma procesów zombie,
#   - w ipcs nie ma struktur o kluczach z logu tego przebiegu.

set -u

KATALOG_PROJEKTU=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BIN="$KATALOG_PROJEKTU/bin"
WZORZEC_PROCESOW="^$BIN/(main|kasjer|pracownik|generator|turysta)( |\$)"

ZIELONY=$'\033[0;32m'
CZERWONY=$'\033[0;31m'
POGRUBIONY=$'\033[1m'
RESET=$'\033[0m'
BLEDY=0

naglowek() { echo "${POGRUBIONY}$*${RESET}"; }
ok()       { echo "  ${ZIELONY}OK${RESET}    $*"; }
blad()     { echo "  ${CZERWONY}BŁĄD${RESET}  $*"; BLEDY=$((BLEDY + 1)); }

# Tworzy katalog roboczy testu i przechodzi do niego.
przygotuj_katalog() {
    KATALOG_TESTU=$(mktemp -d /tmp/kolej-test.XXXXXX)
    cd "$KATALOG_TESTU" || exit 1
}

usun_katalog() {
    cd / && rm -rf "$KATALOG_TESTU"
}

liczba_procesow() {
    pgrep -u "$USER" -f "$WZORZEC_PROCESOW" | wc -l
}

liczba_zombie() {
    ps -u "$USER" -o stat=,comm= | awk '$1 ~ /^Z/ && $2 ~ /^(main|kasjer|pracownik|generator|turysta)$/' | wc -l
}

# Klucze struktur IPC zapisane przez proces główny w logu (np. 0x532c7bef).
klucze_z_logu() {
    grep -m1 "struktury IPC:" logs/symulacja.log | grep -o "0x[0-9a-f]\{8\}"
}

# Ile struktur o kluczach z logu jest nadal w systemie.
pozostale_ipc() {
    local ile=0 klucz
    for klucz in $(klucze_z_logu); do
        ile=$((ile + $(ipcs | grep -ci "^$klucz ")))
    done
    echo "$ile"
}

# Czeka do N sekund, aż proces o podanym PID się zakończy. Zwraca 1 przy przekroczeniu czasu.
czekaj_na_proces() {
    local pid=$1 sekundy=$2 i
    for ((i = 0; i < sekundy * 10; i++)); do
        kill -0 "$pid" 2>/dev/null || return 0
        sleep 0.1
    done
    return 1
}

# Czeka do N sekund, aż nie będzie żadnego procesu symulacji.
czekaj_na_brak_procesow() {
    local sekundy=$1 i
    for ((i = 0; i < sekundy * 10; i++)); do
        [ "$(liczba_procesow)" -eq 0 ] && return 0
        sleep 0.1
    done
    return 1
}

# Wspólne sprawdzenie po zakończeniu symulacji.
sprawdz_sprzatanie() {
    if czekaj_na_brak_procesow 5; then
        ok "nie działa żaden proces symulacji"
    else
        blad "nadal działają procesy symulacji: $(liczba_procesow)"
    fi

    local zombie
    zombie=$(liczba_zombie)
    if [ "$zombie" -eq 0 ]; then ok "brak procesów zombie"; else blad "procesy zombie: $zombie"; fi

    local ipc
    ipc=$(pozostale_ipc)
    if [ "$ipc" -eq 0 ]; then
        ok "struktury IPC usunięte (klucze: $(klucze_z_logu | tr '\n' ' '))"
    else
        blad "w systemie zostało $ipc struktur IPC"
    fi

    if [ ! -e kolej.klucz ]; then ok "plik klucza usunięty"; else blad "został plik kolej.klucz"; fi
}

# Sprawdza, czy log zawiera podany tekst.
sprawdz_log() {
    local tekst=$1 opis=$2
    if grep -q -- "$tekst" logs/symulacja.log; then ok "$opis"; else blad "$opis (brak w logu: \"$tekst\")"; fi
}

zakoncz_test() {
    usun_katalog
    if [ "$BLEDY" -eq 0 ]; then
        echo "  ${ZIELONY}${POGRUBIONY}ZALICZONY${RESET}"
        exit 0
    fi
    echo "  ${CZERWONY}${POGRUBIONY}NIEZALICZONY${RESET} (błędów: $BLEDY)"
    exit 1
}
