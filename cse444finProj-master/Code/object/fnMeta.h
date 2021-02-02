
#ifndef CSE444P_FNMETA_H
#define CSE444P_FNMETA_H

#include "universal_node.h"

typedef struct fnmeta_t FM;

typedef enum {
    PUBLIC, PRIVATE, PROTECTED, DEFAULT
} Modifier;

struct fnmeta_t {
    Node n;
    // Use node->subType for visibility, i.e. public, private, default (package)
    u_int8_t modifier;
    u_int32_t startChar, endChar;
    Node * * calleeResides;
    String * fnCallee, * pem, * calleeShort;
    size_t calleeLen, pemLen;
    String cont;
};

FM * newFM(void);

#endif //CSE444P_FNMETA_H
