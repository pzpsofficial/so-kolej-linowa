#!/bin/bash
# Test 5: limit procesów użytkownika (ulimit -u).
# Ustawiamy limit tylko o 40 wyższy niż liczba zadań, które użytkownik już ma.
# Linux liczy do tego limitu wątki (każdy wątek to osobne zadanie), dlatego
# liczymy je przez "ps -L", a nie same procesy. Parametr -p przechodzi
# walidację (jest poniżej limitu), ale w praktyce system zacznie odmawiać
# fork() z EAGAIN. Symulacja ma to przetrwać: czekać na zakończenie turystów
# i tworzyć kolejnych, aż obsłuży wszystkich, bez zombie i bez wycieków.
# 500 turystów w dniu trwającym 30 s: chętnych naraz jest więcej niż limit,
# a mimo odmów wszyscy zdążą przed Tk.

source "$(dirname "$0")/lib.sh"
naglowek "Test 5: limit procesów użytkownika (fork() zwraca EAGAIN)"
przygotuj_katalog

obecne=$(ps -L -u "$USER" --no-headers | wc -l)
limit=$((obecne + 40))
naraz=$((limit - 10))
ok "wątków użytkownika: $obecne, ustawiony limit: $limit, parametr -p: $naraz"

timeout 120 bash -c "ulimit -u $limit; exec '$BIN/main' -n 500 -p $naraz -k 20 -q -s 5" >/dev/null 2>wyjscie_bledow.txt
kod=$?
if [ "$kod" -eq 0 ]; then ok "proces główny zakończył się kodem 0"; else blad "kod wyjścia $kod"; fi

odmowy=$(grep -c "odmówił utworzenia procesu (EAGAIN)" logs/symulacja.log)
if [ "$odmowy" -gt 0 ]; then
    ok "system odmówił fork() $odmowy razy, symulacja działała dalej"
else
    blad "nie wystąpił EAGAIN, test nie sprawdził limitu (za mało turystów naraz?)"
fi

sprawdz_log "turystów przyszło 500, zakończyło wizytę 500" "mimo odmów obsłużono wszystkich 500 turystów"
sprawdz_sprzatanie
zakoncz_test
