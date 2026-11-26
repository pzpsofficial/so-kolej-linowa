#!/bin/bash
# Test 4: zabicie procesu głównego (SIGKILL).
# Proces główny nie ma szansy posprzątać. Sprawdzamy, że:
#   - pozostałe procesy same się kończą (PR_SET_PDEATHSIG), więc nie ma sierot,
#   - następne uruchomienie w tym samym katalogu usuwa pozostawione struktury IPC.

source "$(dirname "$0")/lib.sh"
naglowek "Test 4: zabicie procesu głównego (SIGKILL) i ponowne uruchomienie"
przygotuj_katalog

"$BIN/main" -n 100000 -p 300 -k 1 -q -s 4 >/dev/null 2>wyjscie_bledow.txt &
pid_main=$!
sleep 3
procesy_w_trakcie=$(liczba_procesow)
kill -KILL "$pid_main"
wait "$pid_main" 2>/dev/null

if czekaj_na_brak_procesow 10; then
    ok "po zabiciu procesu głównego zakończyło się $procesy_w_trakcie procesów (brak sierot)"
else
    blad "po zabiciu procesu głównego działa nadal $(liczba_procesow) procesów"
fi

zostalo=$(pozostale_ipc)
if [ "$zostalo" -eq 3 ]; then
    ok "struktury IPC zostały w systemie (oczekiwane: proces główny nie posprzątał)"
else
    blad "oczekiwano 3 pozostawionych struktur IPC, jest $zostalo"
fi

# Drugie uruchomienie w tym samym katalogu: ten sam plik klucza, te same klucze.
timeout 60 "$BIN/main" -n 50 -k 600 -q -s 4 >/dev/null 2>drugie_uruchomienie.txt
kod=$?
if [ "$kod" -eq 0 ]; then ok "drugie uruchomienie zakończyło się kodem 0"; else blad "drugie uruchomienie: kod $kod"; fi
usuniete=$(grep -c "Usuwam pozostałość" drugie_uruchomienie.txt)
if [ "$usuniete" -eq 3 ]; then
    ok "drugie uruchomienie usunęło 3 pozostałości po przerwanym przebiegu"
else
    blad "drugie uruchomienie usunęło $usuniete pozostałości (oczekiwano 3)"
fi

sprawdz_sprzatanie
zakoncz_test
