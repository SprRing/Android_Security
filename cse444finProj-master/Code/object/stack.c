
#include <string.h>
#include <stdio.h>

#include "stack.h"

/*
 * struct stack_t {
    Node n;
    size_t contTypeSize;

    Stack * (* contSearchFn) (Stack *);
    Stack * (* push) (Stack *s, void *cont);
    void * (* pop) (Stack *s);
    void * (* peek) (Stack *);
    Stack * (* search) (Stack *);
    size_t (* count) (Stack *);
    bool (* delete) (Stack *);
};
 */

static Stack *push_t(Stack *s, void *cont) {
    if (!s || !cont) return s;
    Node *n = (Node *) s;
    n->contLen++;
    if (n->contLen == 1) {
        n->cont = calloc(1, s->contTypeSize + 1);
    } else n->cont = realloc(n->cont, n->contLen * s->contTypeSize + 1);

    char * tar = (char *)(n->cont);
    tar += (n->contLen - 1) * s->contTypeSize;

    memcpy(tar, cont, s->contTypeSize);
    return s;
}

static void *pop_t(Stack *s) {
    if (!s) return 0;
    Node * n = (Node *) s;
    if (n->contLen == 0) return 0;
    n->contLen--;
    void * ret = malloc(s->contTypeSize + 1);

    char *src = (char *) n->cont + n->contLen * s->contTypeSize;
    memcpy(ret, src, s->contTypeSize);
    if (n->contLen == 0) free(n->cont);
    else n->cont = realloc(n->cont, n->contLen * s->contTypeSize + 1);
    return ret;
}

static void *peek_t (Stack * s) {
    if (!s || ((Node *) s)->contLen == 0) return 0;
    return (void *) (((char *)((Node *) s)->cont) + (((Node *) s)->contLen - 1) * sizeof(void *));
}

static void * search_t (Stack *s, void * tar) {
    if (!s || !s->contSearchFn) return 0;
    return s->contSearchFn(s, tar);
}

static size_t count_t (Stack * s) {
    if (!s) return 0;
    return ((Node *) s)->contLen;
}

static bool delete_t (Stack *s) {
    if (!s) return 1;
    Node * n = (Node *) s;
    if (n->contLen) free(n->cont);
    memset(s, 0, sizeof(Stack));
    free(s);
    s = 0;
    return 1;
}

Stack *newStack(size_t contTypeSize, void *(*searchFn)(Stack *, void *)) {
    Stack *ret = calloc(1, sizeof(Stack));

    ret->contTypeSize = contTypeSize;
    ret->contSearchFn = searchFn;

    ret->push = push_t;
    ret->pop = pop_t;
    ret->peek = peek_t;
    ret->search = search_t;
    ret->count = count_t;
    ret->delete = delete_t;

    return ret;
}
