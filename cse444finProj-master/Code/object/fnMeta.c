
#include <string.h>

#include "fnMeta.h"

/*
 * #include "universal_node.h"

typedef struct fnmeta_t FM;

struct fnmeta_t {
    Node n;
    // Use node->subType for visibility, i.e. public, private, default (package)
    u_int8_t modifier;
    u_int32_t startChar, endChar;
    String * fnCallee, * pem;
    size_t calleeLen, pemLen;
    String cont;
};

FM * newFM(void);
 */

FM * newFM(void) {
    FM * ret = calloc(1, sizeof(FM));
    memset(ret, 0, sizeof(FM));
    return ret;
}
