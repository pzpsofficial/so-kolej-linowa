#!/bin/bash
# Test 2: duży wolumen.
# 5000 turystów, najwyżej 500 naraz. Generator nie może przekroczyć limitu
# procesów naraz, wszyscy turyści muszą przejść przez symulację, a po końcu
# nie może zostać żaden proces, zombie ani struktura IPC.

source "$(dirname "$0")/lib.sh"
naglowek "Test 2: duży wolumen (5000 turystów, najwyżej 500 naraz)"
przygotuj_katalog

timeout 120 "$BIN/main" -n 5000 -p 500 -k 60 -q -s 2 >/dev/null 2>wyjscie_bledow.txt
kod=$?
if [ "$kod" -eq 0 ]; then ok "proces główny zakończył się kodem 0"; else blad "kod wyjścia $kod"; fi

sprawdz_log "turystów przyszło 5000, zakończyło wizytę 5000" "5000 turystów przyszło i wyszło"

naraz=$(grep -o "najwięcej naraz [0-9]*" logs/symulacja.log | grep -o "[0-9]*$")
if [ -n "$naraz" ] && [ "$naraz" -le 500 ]; then
    ok "najwięcej turystów naraz: $naraz (limit 500)"
else
    blad "przekroczony limit turystów naraz: ${naraz:-brak danych}"
fi

sprawdz_sprzatanie
zakoncz_test
