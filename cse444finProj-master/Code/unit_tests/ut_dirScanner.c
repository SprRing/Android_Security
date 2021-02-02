
#include "ut_dirScanner.h"

static void ut_ds_scanDir() {
    DS * s = newScanner("/home/tongxuan/Downloads/taobao");
    s->scan(s);//->printDirTree(s);
    s->delete(s);
}

static void ut_ds_parse() {
    DS * s = newScanner("/home/tongxuan/Downloads/taobao");
    s->scan(s);
    s->parseFile(s);
    s->delete(s);
}

static void ut_ds_BI() {
    DS * s = newScanner("/home/tongxuan/Downloads/taobao");
    s->scan(s);//->printDirTree(s);
    s->parseFile(s)->buildInclude(s, s->dirTree);//->printIncludes(s);
    s->delete(s);
}

void ut_dirScanner_main() {
    ut_ds_scanDir();
    ut_ds_parse();
    ut_ds_BI();
}
