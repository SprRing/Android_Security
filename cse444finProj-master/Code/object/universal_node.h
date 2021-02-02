
#ifndef CSE444P_UNIVERSAL_NODE_H
#define CSE444P_UNIVERSAL_NODE_H

#include <stdlib.h>
#include <stdbool.h>

#include "../util/util.h"

typedef enum {
    nodeType_default, DirTree, NT_STACK, NT_FILE, NT_SCANNER, NT_LINKEDLIST
} NodeType;

typedef struct uniNode Node;

struct uniNode {
    NodeType nt;
    String nodeIdentifier;
    void * cont;
    u_int32_t contLen;
    u_int8_t subType;
    Node * prev, * next;

    bool (* contDeleteFn) (void * node);
    Node * (* changeType) (Node *, NodeType);
    Node * (* setID) (Node *, String);
    Node * (* setSubType) (Node *, u_int8_t);
    Node * (* init) (Node *, void * content, u_int32_t contLen, bool (* contDeletableFn) (void *));
    bool (* delete) (Node *);
};

Node * newNode (NodeType, String id);

#endif //CSE444P_UNIVERSAL_NODE_H
