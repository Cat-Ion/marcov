#ifndef MARKOVSEARCH_H
#define MARKOVSEARCH_H

typedef enum { FIND, ENTER } ACTION;
typedef enum { preorder, postorder, endorder, leaf } VISIT;

void *tdelete(const void *, void **, int(*)(const void *, const void *));
void *tfind(const void *, void *const *, int(*)(const void *, const void *));
void *tsearch(const void *, void **, int (*)(const void *, const void *));
void twalk(const void *, int (*)(const void *, VISIT, int, void *), void *);

#endif
