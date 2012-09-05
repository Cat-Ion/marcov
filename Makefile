CFLAGS+=-std=c99 -Wall -pedantic -D_GNU_SOURCE -O3
LDFLAGS+=-lrt -lz

BIN=gen run-lines run-words
LIB=libmarcov.a

all: $(BIN) $(LIB)

$(BIN):%:marcov.o tsearch_avl.o %.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

libmarcov.a: marcov.o tsearch_avl.o
	ar rcs $@ $?

examples: examples/bible.mrk run-lines
	./run-lines 256 < examples/bible.mrk

%.mrk: %.txt gen
	./gen < $< > $@

clean:
	rm -rf *.o $(BIN) examples/*.mrk $(LIB)

.PHONY: clean examples
