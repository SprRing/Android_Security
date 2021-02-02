
#ifndef CSE444P_FILECUS_H
#define CSE444P_FILECUS_H

#include "universal_node.h"

typedef struct fileCus_t FC;

struct fileCus_t {
    Node node;
    Node ** includes;
    size_t includeSize;

    FC * (* append_include) (FC *, Node * root, String);
    FC * (* buildInclude) (FC * fc, Node * root);
    bool (* delete) (FC *);
};

FC * newFile();

#endif //CSE444P_FILECUS_H
