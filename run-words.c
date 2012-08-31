#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "search.h"
#include "markov.h"
#define ORDER 2

int main(int argc, char **argv) {
	int limit = 128;

	if(argc > 1) {
		limit = atoi(argv[1]);
	}

	void *strings = NULL;

	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	srand(t.tv_nsec);
	
	markov_t *m = NULL;
	markov_load(&strings, &m, 0);

	char *nl = stringidx(&strings, "\n");
	wordlist_t nlstart = (wordlist_t) {.num = 1, .w = &nl};
	wordlist_t *wl = NULL;
	wl = markov_randomstart(m, &nlstart);
	if(!wl) {
		wl = markov_randomstart(m, NULL);
	}
	for(int i = 0; i < limit; i++) {
		char *word = markov_next(m, wl);
		if(!word) {
			break;
		}
		printf( "%s ", word);
		for(int j = 1; j < wl->num; j++)
			wl->w[j-1] = wl->w[j];
		wl->w[wl->num-1] = word;
	}
	return 0;
}
