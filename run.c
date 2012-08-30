#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "search.h"
#include "markov.h"
#define ORDER 2

int main() {
	void *strings = NULL;

	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	//srand(t.tv_nsec);
	srand(0);
	
	markov_t *m = NULL;
	markov_load(&strings, &m, stdin);

	wordlist_t *wl = markov_randomstart(m, NULL);
	for(int i = 0; (i < 128 || wl->w[wl->num-1][0] != '\n') && wl->w[wl->num-1]; i++) {
		char *nx = markov_next(m, wl);
		printf("%s ", nx);
		for(int j = 1; j < m->order; j++) {
			wl->w[j-1] = wl->w[j];
		}
		wl->w[m->order-1] = nx;
	}
	printf("\n");
	return 0;
}
