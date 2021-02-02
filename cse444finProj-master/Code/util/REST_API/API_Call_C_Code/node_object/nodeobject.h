#include "util.h"


typedef struct nodeobject O1;

struct nodeobject {
    
    String ID;
    String *perms[100000];
    
    O1 * (* callAPI) (String *);
};

O1 * newO1(String ID);

