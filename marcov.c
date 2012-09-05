#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "search.h"
#include "marcov.h"

marcov_t *
marcov_find(marcov_t *m, char *key);
marcov_t *
marcov_insert(marcov_t *m, marcov_t *new);
marcov_t *
marcov_search(marcov_t *m, char *key);


/* Comparison function for tsearch/tfind */
int marcovkeycomp(const void *a, const void *b) {
	return strcmp(((marcov_t *)a)->key, ((marcov_t *)b)->key);
}

/* Compares a char *a and marcov_t *b->key */
int strptrcmp(const void *a, const void *b) {
	return strcmp((char*)a, *(char **)b);
}

/* Actions for twalk */
int twalk_choose(const void *node, VISIT v, int depth, void *data) {
	struct { int n; marcov_t *end; } *s = data;
	
	if(v == postorder || v == leaf) {
		if(s->n < (*(marcov_t **)node)->total) {
			s->end = *(marcov_t **)node;
			return 0;
		} else {
			s->n -= (*(marcov_t **)node)->total;
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
	marcov_t *m = *(marcov_t **)node;
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
		marcov_t *m = *(marcov_t **)node;
		printf("\"%s\" (%d)\n", m->key, m->total);
	}
	return 1;
}

/* Choose a random child, weighted by their 'total' member */
marcov_t *marcov_choose(marcov_t *m) {
	struct { int n; marcov_t *end; } d;
	if(!m) {
		return NULL;
	}
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
void marcov_nodedumper(marcov_t *m, void *ptrs) {
	twalk(m->tree, twalk_nodedumper, ptrs);
}

/* Prints all children */
void marcov_printchildren(marcov_t *m) {
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

/* Add a combination of words w to the marcov chain at m */
void marcov_add(marcov_t *m, wordlist_t *w) {
	marcov_t *f = NULL;

	if(w->num < m->order)
		return;

	m->total++;

	/* Just a word entry, no tree. Increase count and return. */
	if(m->order == -1)
		return;

	/* Find a matching follower */
	f = marcov_search(m, w->w[w->num - 1 - m->order]);

	marcov_add(f, w);
}

/* Decrement the combination, remove if necessary */
void marcov_dec(marcov_t *m, wordlist_t *w) {
	marcov_t *f = NULL;

	if(w->num < m->order)
		return;

	m->total--;

	/* Just a word entry, no tree. Increase count and return. */
	if(m->order == -1)
		return;

	/* Find a matching follower */
	f = marcov_search(m, w->w[w->num - 1 - m->order]);

	marcov_dec(f, w);

	if(f->total == 0) {
		tdelete(w->w[w->num - 1 - m->order], &(m->tree), marcovkeycomp);
	}
}

/* Find a key under a marcov node, if it exists. */
marcov_t *marcov_find(marcov_t *m, char *key) {
	marcov_t **f = tfind(&key, &(m->tree), marcovkeycomp);
	if(f)
		return *f;
	else {
		return NULL;
	}
}

marcov_t *marcov_find_prefix(marcov_t *m, wordlist_t *w, int len) {
	if(!m || !w || len < 0 || len > m->order || len > w->num) {
		return NULL;
	}

	marcov_t *nx = m;
	for(int i = 0; i < w->num; i++) {
		nx = marcov_find(nx, w->w[i]);
		if(!nx) {
			return NULL;
		}
	}
	
	return nx;
}

char *marcov_getline(marcov_t *m) {
	int len = 0, maxlen = 1024, i;
	char *out = malloc(maxlen);
	char *nl = "\n";
	wordlist_t wl = (wordlist_t) { .num = 1, .w = &nl },
		   *start;
	start = marcov_randomstart(m, &wl);
	if(!start) {
		free(out);
		return NULL;
	}
	wl = *start;
	free(start);
	i = 0;
	out[0] = '\0';
	do {
		if(i < m->order - 1) {
			i++;
		} else {
			int j;
			char *w = marcov_next(m, &wl);
			for(j = 1; j < wl.num; j++)
				wl.w[j-1] = wl.w[j];
			wl.w[j-1] = w;
		}
		while(len + strlen(wl.w[i]) >= maxlen) {
			char *tmp = realloc(out, 2 * maxlen);
			if(!tmp) {
				exit(EXIT_FAILURE);
			}
			maxlen *= 2;
			out = tmp;
		}
		len += sprintf(out + len, "%s%s", (len > 0 && wl.w[i][0] != '\n') ? " " : "", wl.w[i]);
	} while(strcmp("\n", wl.w[i]));
	return out;
}

/* Insert a new element to a chain. */
marcov_t *marcov_insert(marcov_t *m, marcov_t *new) {
	marcov_t **f = tsearch(new, &(m->tree), marcovkeycomp);
	return *f;
}

/* Get a word that might follow the wordlist in w */
char *marcov_next(marcov_t *m, wordlist_t *w) {
	if(!m)
		return NULL;
	m = marcov_choose(marcov_find_prefix(m, w, m->order));
	if(m) {
		return m->key;
	} else {
		return NULL;
	}
}

/* Return number of occurences of a combination */
int marcov_num(marcov_t *m, wordlist_t *w) {
	if(w->num > m->order + 1) {
		return 0;
	}
	for(int i = 0; i < w->num; i++) {
		m = marcov_find(m, w->w[i]);
		if(!m) {
			return 0;
		}
	}
	return m->total;
}

/* Get a random starting point for a chain, optionally beginning with
   the wordlist in start */
wordlist_t *marcov_randomstart(marcov_t *m, wordlist_t *start) {
	int got = 0, savedgot;
	wordlist_t *ret = malloc(sizeof(wordlist_t));
	marcov_t *next = m, *savednext;

	*ret = (wordlist_t) { .num = m->order, .w = calloc(m->order, sizeof(char *)) };

	if(start) {
		if(start->num > m->order)
			return NULL;
		for(int i = 0; i < start->num; i++) {
			next = marcov_find(next, start->w[i]);
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
		next = marcov_choose(next);
		if(!next)
			goto restart;
		ret->w[got++] = next->key;
	}
	return ret;
}

/* As marcov_find, but insert a new key if it doesn't exist. */
marcov_t *marcov_search(marcov_t *m, char *key) {
	marcov_t **f = tsearch(&key, &(m->tree), marcovkeycomp);

	if((void *)*f == (void *)&key) {
		*f = malloc(sizeof(marcov_t));
		**f = (marcov_t) { .key = key, .order = m->order - 1, .total = 0, .tree = NULL };
	}

	return *f;
}

void marcov_dump(void *strings, marcov_t *m, int fd) {
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
	marcov_nodedumper(m, &ptrs);
	gzwrite(gzf, &sep, sizeof(int));
	gzflush(gzf, Z_FINISH);
	
	free(ptrs.ptrs);
}

marcov_t *marcov_loadrec(char **strings, int num, gzFile f) {
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

	marcov_t *m = malloc(sizeof(marcov_t));
	m->key = key;
	m->order = order;
	m->total = total;
	m->tree = NULL;
	while(1) {
		marcov_t *new = marcov_loadrec(strings, num, f);
		if(!new)
			break;
		marcov_insert(m, new);
	}
	return m;
}

void marcov_load(void **strings, marcov_t **m, int fd) {
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

	*m = marcov_loadrec(strarr, strnum, f);

	free(strarr);
}

void marcov_walk(marcov_t *m, int (*function)(const void *, VISIT, int, void *), void *data) {
	twalk(m->tree, function, data);
}

