
#ifndef CSE444P_DIRSCANNER_H
#define CSE444P_DIRSCANNER_H

#include "universal_node.h"

typedef struct dirScanner_t DS;

struct dirScanner_t {
    Node node;
    String dir;
    Node * dirTree;

    DS * (* scan) (DS *);
    DS * (* parseFile) (DS *);
    DS * (* buildInclude) (DS *, Node * root);
    DS * (* extractFunction) (DS *);
    DS * (* extractPermission) (DS *);
    DS * (* printDirTree) (DS *);
    DS * (* printIncludes) (DS *);
    DS * (* printCallee) (DS *);
    DS * (* printPem) (DS *);
    DS * (* pemCheck2) (DS *);
    DS * (* pemTrack) (DS *, String);
    bool (* delete) (DS *);
};

DS * newScanner (String);

#endif //CSE444P_DIRSCANNER_H
