#!/bin/bash
# Test 1: normalny dzień od Tp do Tk.
# 300 turystów, dzień trwa 5 s. Symulacja ma się sama zakończyć o Tk,
# każdy turysta ma przyjść i wyjść, a po końcu nic nie może zostać.

source "$(dirname "$0")/lib.sh"
naglowek "Test 1: normalny dzień (300 turystów, Tk po 5 s)"
przygotuj_katalog

timeout 60 "$BIN/main" -n 300 -p 100 -k 120 -q -s 1 >/dev/null 2>wyjscie_bledow.txt
kod=$?
if [ "$kod" -eq 0 ]; then ok "proces główny zakończył się kodem 0"; else blad "kod wyjścia $kod"; fi

sprawdz_log "godzina Tk: koniec dnia" "symulacja zakończyła się o Tk"
sprawdz_log "turystów przyszło 300, zakończyło wizytę 300" "wszyscy turyści przyszli i wyszli"
sprawdz_sprzatanie
zakoncz_test
