
#ifndef CSE444P_STACK_H
#define CSE444P_STACK_H

#include "universal_node.h"

typedef struct stack_t Stack;

struct stack_t {
    Node n;
    size_t contTypeSize;
    void * (* contSearchFn) (Stack *, void *);

    Stack * (* push) (Stack *s, void *cont);
    void * (* pop) (Stack *s);
    void * (* peek) (Stack *);
    void * (* search) (Stack *, void *);
    size_t (* count) (Stack *);
    bool (* delete) (Stack *);
};

Stack *newStack(size_t contTypeSize, void *(*searchFn)(Stack *, void *));

#endif //CSE444P_STACK_H
