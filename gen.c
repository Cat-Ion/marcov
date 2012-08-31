#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "search.h"
#include "markov.h"

#define ORDER 2

int main() {
	char buf[1024];
	int i, start, offset = 0;
	int addnewline = 0;

	void *strings = NULL;
	wordlist_t wl = { .num = ORDER + 1, .w = calloc((ORDER + 1), sizeof(char *)) };

	markov_t m = { .key = NULL, .order = ORDER, .total = 0, .tree = NULL };

	for(i = 0; i < ORDER + 1; i++) {
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
			for(int j = 0; j < ORDER; j++)
				wl.w[j] = wl.w[j+1];
			wl.w[ORDER] = word;
			if(wl.w[0]) {
				markov_add(&m, &wl);
			}

			if(addnewline) {
				char *word = stringidx(&strings, "\n");
				for(int j = 0; j < ORDER; j++)
					wl.w[j] = wl.w[j+1];
				wl.w[ORDER] = word;
				if(wl.w[0])
					markov_add(&m, &wl);
				break;
			}
			while(buf[++i] == ' ');
		} while(1);
	}
	markov_dump(strings, &m, 1);
	return 0;
}
