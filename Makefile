# Kolej linowa - kompilacja
#
#   make          kompiluje wszystkie programy do katalogu bin/
#   make test     kompiluje i uruchamia testy z katalogu tests/
#   make clean    usuwa pliki kompilacji
#
# Każdy plik src/*.c to osobny program (main, kasjer, pracownik, ...).
# Pliki z src/common/ to wspólne moduły, dołączane do każdego programu.

CC      := gcc
CFLAGS  := -std=c11 -D_GNU_SOURCE -Wall -Wextra -Werror -O2 -g -pthread
LDFLAGS := -pthread

PROGRAMY   := $(basename $(notdir $(wildcard src/*.c)))
BIN        := $(addprefix bin/,$(PROGRAMY))
SRC_COMMON := $(wildcard src/common/*.c)
OBJ_COMMON := $(patsubst src/%.c,build/%.o,$(SRC_COMMON))

all: $(BIN)

bin/%: build/%.o $(OBJ_COMMON) | bin
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c | build/common
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

bin build/common:
	mkdir -p $@

test: all
	./tests/run_all.sh

clean:
	rm -rf build bin

-include $(OBJ_COMMON:.o=.d) $(addprefix build/,$(addsuffix .d,$(PROGRAMY)))

.PHONY: all test clean
.SECONDARY:
