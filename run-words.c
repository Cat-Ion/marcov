#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "search.h"
#include "marcov.h"
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
	
	marcov_t *m = NULL;
	marcov_load(&strings, &m, 0);

	char *nl = stringidx(&strings, "\n");
	wordlist_t nlstart = (wordlist_t) {.num = 1, .w = &nl};
	wordlist_t *wl = NULL;
	wl = marcov_randomstart(m, &nlstart);
	if(!wl) {
		wl = marcov_randomstart(m, NULL);
	}
	for(int i = 0; i < limit; i++) {
		char *word = marcov_next(m, wl);
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
