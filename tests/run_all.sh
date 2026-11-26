#!/bin/bash
# run_all.sh - uruchamia wszystkie testy po kolei i podsumowuje wyniki.

KATALOG=$(cd "$(dirname "$0")" && pwd)
zaliczone=0
niezaliczone=()

for test in "$KATALOG"/test*.sh; do
    if bash "$test"; then
        zaliczone=$((zaliczone + 1))
    else
        niezaliczone+=("$(basename "$test")")
    fi
    echo
done

echo "Zaliczone: $zaliczone, niezaliczone: ${#niezaliczone[@]}"
for nazwa in "${niezaliczone[@]}"; do
    echo "  - $nazwa"
done
[ "${#niezaliczone[@]}" -eq 0 ]
