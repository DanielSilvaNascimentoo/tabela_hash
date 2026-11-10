CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -pedantic -O2
SRC_LIB  = src/hash_table.c src/palavras.c

all: indice test_hash bench

indice: src/main.c $(SRC_LIB)
	$(CC) $(CFLAGS) -o $@ $^

test_hash: src/test_hash.c $(SRC_LIB)
	$(CC) $(CFLAGS) -o $@ $^

bench: src/bench.c src/hash_table.c
	$(CC) $(CFLAGS) -o $@ $^

test: test_hash
	./test_hash

run: indice
	./indice -m 101 data/exemplo.txt

experimentos: bench
	./bench 5 resultados.csv

asan:
	$(CC) -std=c11 -g -fsanitize=address,undefined -o test_hash_asan src/test_hash.c $(SRC_LIB) && ./test_hash_asan

clean:
	rm -f indice test_hash bench test_hash_asan resultados.csv
