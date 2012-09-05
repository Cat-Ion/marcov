#ifndef _MARKOV_H
#define _MARKOV_H
#include "search.h"

typedef struct {
	int num;
	char **w;
} wordlist_t;
typedef struct marcov_s {
	char *key;
	int order;
	int total;
	void *tree;
} marcov_t;

char *
stringidx(void **l, char *s);
void
marcov_add(marcov_t *m, wordlist_t *w);
void
marcov_dec(marcov_t *m, wordlist_t *w);
void
marcov_dump(void *strings, marcov_t *m, int fd);
marcov_t *
marcov_find_prefix(marcov_t *m, wordlist_t *w, int len);
char *
marcov_getline(marcov_t *m);
void
marcov_load(void **strings, marcov_t **m, int fd);
char *
marcov_next(marcov_t *m, wordlist_t *w);
wordlist_t *
marcov_randomstart(marcov_t *m, wordlist_t *start);
void
marcov_walk(marcov_t *m, int (*function)(const void *, VISIT, int, void *), void *data);
#endif
