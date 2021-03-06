#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "search.h"
#include "marcov.h"

#define ORDER 2

int main(int argc, char **argv) {
	int order = argc > 1 ? atoi(argv[1]) : ORDER;
	char buf[1024];
	int i, start, offset = 0;
	int addnewline = 0;

	void *strings = NULL;
	wordlist_t wl = { .num = order + 1, .w = calloc((order + 1), sizeof(char *)) };

	marcov_t m = { .key = NULL, .order = order, .total = 0, .tree = NULL };

	for(i = 0; i < order + 1; i++) {
		wl.w[i] = NULL;
	}
	
	while(fgets(buf + offset, 1024 - offset, stdin)) {
		i = 0;
		do {
			start = i;
			for(start = i;
			    buf[i] && buf[i] != ' ' && buf[i] != '\n';
			    i++);
			if(i == start)
				break;
			if(buf[i] == '\0') {
				memmove(buf, buf+start, i-start);
				offset = i - start;
				break;
			}
			offset = 0;

			if(buf[i] == '\n')
				addnewline = 1;
			else
				addnewline = 0;
			
			buf[i] = '\0';

			char *word = stringidx(&strings, buf + start);
			for(int j = 0; j < order; j++)
				wl.w[j] = wl.w[j+1];
			wl.w[order] = word;
			if(wl.w[0]) {
				marcov_add(&m, &wl);
			}

			if(addnewline) {
				char *word = stringidx(&strings, "\n");
				for(int j = 0; j < order; j++)
					wl.w[j] = wl.w[j+1];
				wl.w[order] = word;
				if(wl.w[0])
					marcov_add(&m, &wl);
				break;
			}
			while(buf[++i] == ' ');
		} while(1);
	}
	marcov_dump(strings, &m, 1);
	return 0;
}
