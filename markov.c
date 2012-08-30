#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "search.h"
#include "markov.h"

/* Comparison function for tsearch/tfind */
int markovkeycomp(const void *a, const void *b) {
	return strcmp(((markov_t *)a)->key, ((markov_t *)b)->key);
}

/* Compares a char *a and markov_t *b->key */
int strptrcmp(const void *a, const void *b) {
	return strcmp((char*)a, *(char **)b);
}

/* Actions for twalk */
int twalk_choose(const void *node, VISIT v, int depth, void *data) {
	struct { int n; markov_t *end; } *s = data;
	
	if(v == postorder || v == leaf) {
		if(s->n < (*(markov_t **)node)->total) {
			s->end = *(markov_t **)node;
			return 0;
		} else {
			s->n -= (*(markov_t **)node)->total;
			return 1;
		}
	} else {
		return 1;
	}
}
int twalk_createstringarray(const void *node, VISIT v, int depth, void *data) {
	char *str = *(char **)node;
	struct {
		int num, maxnum;
		char **ptrs;
	} *ptrs = data;
	if(v == postorder || v == leaf) {
		if(ptrs->num == ptrs->maxnum) {
			char **tmp;
			ptrs->maxnum *= 2;
			tmp = realloc(ptrs->ptrs, ptrs->maxnum * sizeof(char *));
			if(!tmp)
				exit(EXIT_FAILURE);
			ptrs->ptrs = tmp;
		}
		ptrs->ptrs[ptrs->num++] = str;
	}
	return 1;
}
int twalk_nodedumper(const void *node, VISIT v, int depth, void *data) {
	int sep = -2;
	markov_t *m = *(markov_t **)node;
	struct {
		int num, maxnum;
		char **ptrs;
		gzFile f;
	} *ptrs = data;

	if(v == postorder || v == leaf) {
		char **str = bsearch(m->key, ptrs->ptrs, ptrs->num, sizeof(char *), strptrcmp);
		int stridx = str - ptrs->ptrs;
		
		gzwrite(ptrs->f, &stridx, sizeof(int));
		gzwrite(ptrs->f, &(m->order), sizeof(int));
		gzwrite(ptrs->f, &(m->total), sizeof(int));
		twalk(m->tree, twalk_nodedumper, ptrs);
		gzwrite(ptrs->f, &sep, sizeof(int));
	}
	return 1;
}
int twalk_printchildren(const void *node, VISIT v, int depth, void *data) {
	if(v == postorder || v == leaf) {
		markov_t *m = *(markov_t **)node;
		printf("\"%s\" (%d)\n", m->key, m->total);
	}
	return 1;
}

/* Choose a random child, weighted by their 'total' member */
markov_t *markov_choose(markov_t *m) {
	struct { int n; markov_t *end; } d;
	d.n = rand() % m->total;
	d.end = NULL;
	twalk(m->tree, twalk_choose, &d);
	return d.end;
}

/* Puts every string in the string list in an array as preparation for
   dumping. Array indices will be used as key IDs */
void createstringarray(void *strings, void *ptrs) {
	twalk(strings, twalk_createstringarray, ptrs);
}

/* Dump all nodes to a file */
void markov_nodedumper(markov_t *m, void *ptrs) {
	twalk(m->tree, twalk_nodedumper, ptrs);
}

/* Prints all children */
void markov_printchildren(markov_t *m) {
	twalk(m->tree, twalk_printchildren, NULL);
}

/* Find or add a string to the tree at *l and return its pointer. Used
   to deduplicate data */
char *stringidx(void **l, char *s) {
	char **found = (char **)tsearch(s, l, (int (*)(const void*, const void*))strcmp);
	if(*found == s) {
		*found = strdup(s);
	}
	return *found;
}

/* Add a combination of words w to the markov chain at m */
void markov_add(markov_t *m, wordlist_t *w) {
	markov_t *f = NULL;

	if(w->num < m->order)
		return;

	m->total++;

	/* Just a word entry, no tree. Increase count and return. */
	if(m->order == -1)
		return;

	/* Find a matching follower */
	f = markov_search(m, w->w[w->num - 1 - m->order]);

	markov_add(f, w);
}

/* Find a key under a markov node, if it exists. */
markov_t *markov_find(markov_t *m, char *key) {
	markov_t **f = tfind(&key, &(m->tree), markovkeycomp);
	if(f)
		return *f;
	else {
		return NULL;
	}
}

/* Insert a new element to a chain. */
markov_t *markov_insert(markov_t *m, markov_t *new) {
	markov_t **f = tsearch(new, &(m->tree), markovkeycomp);
	return *f;
}

/* Get a word that might follow the wordlist in w */
char *markov_next(markov_t *m, wordlist_t *w) {
	markov_t *nx = m;
	if(w->num > m->order)
		return NULL;
	for(int i = 0; i < w->num; i++) {
		nx = markov_find(nx, w->w[i]);
		if(!nx) {
			return NULL;
		}
	}
	
	return markov_choose(nx)->key;
}

/* Return number of occurences of a combination */
int markov_num(markov_t *m, wordlist_t *w) {
	if(w->num > m->order + 1) {
		return 0;
	}
	for(int i = 0; i < w->num; i++) {
		m = markov_find(m, w->w[i]);
		if(!m) {
			return 0;
		}
	}
	return m->total;
}

/* Get a random starting point for a chain, optionally beginning with
   the wordlist in start */
wordlist_t *markov_randomstart(markov_t *m, wordlist_t *start) {
	int got = 0, savedgot;
	wordlist_t *ret = malloc(sizeof(wordlist_t));
	markov_t *next = m, *savednext;

	*ret = (wordlist_t) { .num = m->order, .w = calloc(m->order, sizeof(char *)) };

	if(start) {
		if(start->num > m->order)
			return NULL;
		for(int i = 0; i < start->num; i++) {
			next = markov_find(next, start->w[i]);
			if(!next)
				return NULL;
			ret->w[got++] = start->w[i];
		}
	}

	savedgot = got;
	savednext = next;
 restart:
	got = savedgot;
	next = savednext;
	while(got < m->order) {
		next = markov_choose(next);
		if(!next)
			goto restart;
		ret->w[got++] = next->key;
	}
	return ret;
}

/* As markov_find, but insert a new key if it doesn't exist. */
markov_t *markov_search(markov_t *m, char *key) {
	markov_t **f = tsearch(&key, &(m->tree), markovkeycomp);

	if((void *)*f == (void *)&key) {
		*f = malloc(sizeof(markov_t));
		**f = (markov_t) { .key = key, .order = m->order - 1, .total = 0, .tree = NULL };
	}

	return *f;
}

void markov_dump(void *strings, markov_t *m, int fd) {
	int nullstr = -1, sep = -2;
	gzFile gzf = gzdopen(fd, "wb");

	struct {
		int num, maxnum;
		char **ptrs;
		gzFile f;
	} ptrs;
	ptrs.num = 0;
	ptrs.maxnum = 1;
	ptrs.ptrs = malloc(sizeof(char *));
	ptrs.f = gzf;

	createstringarray(strings, &ptrs);

	for(int i = 0; i < ptrs.num; i++) {
		int length = strlen(ptrs.ptrs[i]);
		gzwrite(gzf, &length, sizeof(int));
		gzwrite(gzf, ptrs.ptrs[i], length);
	}
	gzwrite(gzf, &sep, sizeof(int));

	gzwrite(gzf, &nullstr, sizeof(int));
	gzwrite(gzf, &(m->order), sizeof(int));
	gzwrite(gzf, &(m->total), sizeof(int));
	markov_nodedumper(m, &ptrs);
	gzwrite(gzf, &sep, sizeof(int));
	gzflush(gzf, Z_FINISH);
	
	free(ptrs.ptrs);
}

markov_t *markov_loadrec(char **strings, int num, gzFile f) {
	int nullstr = -1,
		sep = -2;

	int idx = -1, order = -1, total = 0;
	char *key;
	
	if(gzread(f, &idx, sizeof(int)) < sizeof(int)) {
		printf("Fuck.\n");
		exit(EXIT_FAILURE);
	}
	if(idx == sep)
		return 0;
	if(idx == nullstr)
		key = NULL;
	else
		key = strings[idx];
	gzread(f, &order, sizeof(int));
	gzread(f, &total, sizeof(int));

	markov_t *m = malloc(sizeof(markov_t));
	m->key = key;
	m->order = order;
	m->total = total;
	m->tree = NULL;
	while(1) {
		markov_t *new = markov_loadrec(strings, num, f);
		if(!new)
			break;
		markov_insert(m, new);
	}
	return m;
}

void markov_load(void **strings, markov_t **m, int fd) {
	gzFile f = gzdopen(fd, "rb");
	int sep = -2;
	int strnum = 0, strnummax = 1;
	
	char **strarr = malloc(sizeof(char *));
	while(1) {
		int len;
		char c[4096];
		gzread(f, &len, sizeof(int));
		if(len == sep)
			break;

		if(strnum == strnummax) {
			strnummax *= 2;
			char **tmp = realloc(strarr, strnummax * sizeof(char*));
			if(!tmp)
				exit(EXIT_FAILURE);
			strarr = tmp;
		}

		gzread(f, c, len);
		c[len]='\0';
		strarr[strnum++] = stringidx(strings, c);
	}

	*m = markov_loadrec(strarr, strnum, f);

	free(strarr);
}
