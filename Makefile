CFLAGS+=-std=c99 -Wall -pedantic -D_GNU_SOURCE -O3
LDFLAGS+=-lrt -lz

BIN=gen run

all: $(BIN)

$(BIN):%:markov.o tsearch_avl.o %.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

examples: examples/bible.mrk run
	./run 256 < examples/bible.mrk

%.mrk: %.txt gen
	./gen < $< > $@

clean:
	rm -rf *.o $(BIN) examples/*.mrk

.PHONY: clean examples
