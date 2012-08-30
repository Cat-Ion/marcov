#ifndef _MARKOV_H
#define _MARKOV_H
typedef struct {
	int num;
	char **w;
} wordlist_t;
typedef struct markov_s {
	char *key;
	int order;
	int total;
	void *tree;
} markov_t;

char *
stringidx(void **l, char *s);
void
markov_add(markov_t *m, wordlist_t *w);
void
markov_dump(void *strings, markov_t *m, int fd);
markov_t *
markov_find(markov_t *m, char *key);
wordlist_t
markov_getline(markov_t *m);
markov_t *
markov_insert(markov_t *m, markov_t *new);
void
markov_load(void **strings, markov_t **m, int fd);
char *
markov_next(markov_t *m, wordlist_t *w);
wordlist_t *
markov_randomstart(markov_t *m, wordlist_t *start);
markov_t *
markov_search(markov_t *m, char *key);

#endif
