CFLAGS+=-std=c99 -Wall -pedantic -D_GNU_SOURCE -g
LDFLAGS+=-lrt -lz

BIN=gen run

all: $(BIN)

gen: gen.o markov.o tsearch_avl.o

run: run.o markov.o tsearch_avl.o

gen.o: gen.c

run.o: run.c

markov.o: markov.c

tsearch_avl.o: tsearch_avl.c

clean:
	rm -rf *.o $(BIN)

.PHONY: clean
