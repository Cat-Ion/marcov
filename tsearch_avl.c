/* Copyright © 2005-2012 Rich Felker
   
   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* Minor changes made to walk/twalk to make the functions actually
   useful */
#include <stdlib.h>
#include "search.h"

struct node {
	const void *key;
	struct node *left;
	struct node *right;
	int height;
};

static int delta(struct node *n) {
	return (n->left ? n->left->height:0) - (n->right ? n->right->height:0);
}

static void updateheight(struct node *n) {
	n->height = 0;
	if (n->left && n->left->height > n->height)
		n->height = n->left->height;
	if (n->right && n->right->height > n->height)
		n->height = n->right->height;
	n->height++;
}

static struct node *rotl(struct node *n) {
	struct node *r = n->right;
	n->right = r->left;
	r->left = n;
	updateheight(n);
	updateheight(r);
	return r;
}

static struct node *rotr(struct node *n) {
	struct node *l = n->left;
	n->left = l->right;
	l->right = n;
	updateheight(n);
	updateheight(l);
	return l;
}

static struct node *balance(struct node *n) {
	int d = delta(n);

	if (d < -1) {
		if (delta(n->right) > 0)
			n->right = rotr(n->right);
		return rotl(n);
	} else if (d > 1) {
		if (delta(n->left) < 0)
			n->left = rotl(n->left);
		return rotr(n);
	}
	updateheight(n);
	return n;
}

static struct node *find(struct node *n, const void *k,
	int (*cmp)(const void *, const void *))
{
	int c;

	if (!n)
		return 0;
	c = cmp(k, n->key);
	if (c == 0)
		return n;
	if (c < 0)
		return find(n->left, k, cmp);
	else
		return find(n->right, k, cmp);
}

static struct node *insert(struct node **n, const void *k,
	int (*cmp)(const void *, const void *), int *new)
{
	struct node *r = *n;
	int c;

	if (!r) {
		*n = r = malloc(sizeof **n);
		if (r) {
			r->key = k;
			r->left = r->right = 0;
			r->height = 1;
		}
		*new = 1;
		return r;
	}
	c = cmp(k, r->key);
	if (c == 0)
		return r;
	if (c < 0)
		r = insert(&r->left, k, cmp, new);
	else
		r = insert(&r->right, k, cmp, new);
	if (*new)
		*n = balance(*n);
	return r;
}

static struct node *movr(struct node *n, struct node *r) {
	if (!n)
		return r;
	n->right = movr(n->right, r);
	return balance(n);
}

static struct node *remove(struct node **n, const void *k,
	int (*cmp)(const void *, const void *), struct node *parent)
{
	int c;

	if (!*n)
		return 0;
	c = cmp(k, (*n)->key);
	if (c == 0) {
		struct node *r = *n;
		*n = movr(r->left, r->right);
		free(r);
		return parent;
	}
	if (c < 0)
		parent = remove(&(*n)->left, k, cmp, *n);
	else
		parent = remove(&(*n)->right, k, cmp, *n);
	if (parent)
		*n = balance(*n);
	return parent;
}

void *tdelete(const void *restrict key, void **restrict rootp,
	int(*compar)(const void *, const void *))
{
	/* last argument is arbitrary non-null pointer
	   which is returned when the root node is deleted */
	return remove((void*)rootp, key, compar, *rootp);
}

void *tfind(const void *key, void *const *rootp,
	int(*compar)(const void *, const void *))
{
	return find(*rootp, key, compar);
}

void *tsearch(const void *key, void **rootp,
	int (*compar)(const void *, const void *))
{
	int new = 0;
	return insert((void*)rootp, key, compar, &new);
}

static void walk(const struct node *r, int (*action)(const void *, VISIT, int, void *), int d, void *data)
{
	if (r == 0)
		return;
	if (r->left == 0 && r->right == 0) {
		if(!action(r, leaf, d, data)) return;
	} else {
		if(!action(r, preorder, d, data)) return;
		walk(r->left, action, d+1, data);
		if(!action(r, postorder, d, data)) return;
		walk(r->right, action, d+1, data);
		if(!action(r, endorder, d, data)) return;
	}
}

void twalk(const void *root, int (*action)(const void *, VISIT, int, void *), void *data)
{
	walk(root, action, 0, data);
}
