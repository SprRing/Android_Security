
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "universal_node.h"

static Node * node_changeType(Node * n, NodeType nt) {
    if (!n) return 0;
    n->nt = nt;
    return n;
}

static Node * node_setID (Node * n, String id) {
    if (!n) return 0;
    if (n->nodeIdentifier) free(n->nodeIdentifier);
    n->nodeIdentifier = calloc(strlen(id) + 1, sizeof(char));
    strcpy(n->nodeIdentifier, id);
    return n;
}

static Node * node_setSubType (Node * n, u_int8_t subType) {
    if (!n) return 0;
    n->subType = subType;
    return n;
}

static Node * node_init (Node * n, void * content, u_int32_t contLen, bool (* contDeletableFn) (void *)) {
    if (!n || !content) return n;
    n->cont = content;
    n->contLen = contLen;
    n->contDeleteFn = contDeletableFn;
    return n;
}

static bool node_delete(Node * n) {
    if (!n) return 0;
    if (n->nodeIdentifier) free(n->nodeIdentifier);
    if (n->cont && n->contDeleteFn) {
        n->contDeleteFn(n);
    }
    memset(n, 0, sizeof(Node));

    free(n);
    n = 0;
    return 1;
}

Node * newNode (NodeType nt, String id) {
    Node * ret = calloc(1, sizeof(Node));
    memset(ret, 0, sizeof(Node));
    ret->nt = nt;

    if (id) {
        ret->nodeIdentifier = calloc(strlen(id) + 1, sizeof(char));
        strcpy(ret->nodeIdentifier, id);
    }

    ret->changeType = node_changeType;
    ret->setID = node_setID;
    ret->setSubType = node_setSubType;
    ret->init = node_init;
    ret->delete = node_delete;
    //printf("added: %p\n", (void *) ret->changeType);

    return ret;
}
