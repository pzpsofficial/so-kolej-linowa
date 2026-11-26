#!/bin/bash
# Test 3: przerwanie w trakcie dnia (Ctrl+C).
# Uruchamiamy długi dzień z wieloma turystami, po 3 s wysyłamy SIGINT do
# procesu głównego. Wszystkie procesy mają się zakończyć, a struktury IPC
# zostać usunięte.

source "$(dirname "$0")/lib.sh"
naglowek "Test 3: przerwanie w trakcie dnia (SIGINT, czyli Ctrl+C)"
przygotuj_katalog

"$BIN/main" -n 100000 -p 300 -k 1 -q -s 3 >/dev/null 2>wyjscie_bledow.txt &
pid_main=$!
sleep 3
procesy_w_trakcie=$(liczba_procesow)
kill -INT "$pid_main"

if czekaj_na_proces "$pid_main" 15; then
    wait "$pid_main"
    kod=$?
    if [ "$kod" -eq 0 ]; then ok "proces główny zakończył się kodem 0"; else blad "kod wyjścia $kod"; fi
else
    blad "proces główny nie zakończył się w 15 s po SIGINT"
    kill -KILL "$pid_main" 2>/dev/null
fi

ok "w chwili przerwania działało $procesy_w_trakcie procesów symulacji"
sprawdz_log "przerwanie symulacji" "proces główny zauważył przerwanie"
sprawdz_sprzatanie
zakoncz_test
