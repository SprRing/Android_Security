
#include "fileCus.h"

/*
 * struct fileCus_t {
    Node node;
    Node ** includes;
    size_t includeSize;

    FC * (* append_include) (FC *, Node * root, String);
    FC * (* buildInclude) (FC * fc, Node * root);
    bool (* delete) (FC *);
};

FC * newFile();
 */
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>
#include <zconf.h>

#include "fileCus.h"
#include "fnMeta.h"
#include "dirScanner.h"

/*
 * struct fileCus_t {
    Node node;
    Node ** includes;
    size_t includeSize;
    FC * (* append_include) (FC *, String);
    bool (* delete) (FC *);
};
 */

static bool dl_contFM(void *n) {
    /*Node *node = (Node *) n;
    for (u_int32_t i = 0; i < node->contLen; i++) {
        char *ptr;
        ptr = (void *) (((char *) node->cont) + i * sizeof(FM));
        FM *fm = (FM *) ptr;

        if (fm->n.nodeIdentifier) free(fm->n.nodeIdentifier);

        free(fm->cont);
        if (fm->calleeShort) free(fm->calleeShort);
        if (fm->calleeResides) free(fm->calleeResides);
        fm->cont = 0;
    }
    free(node->cont);
    node->cont = 0;*/
    return n;
}

static Node * findIncludeNode(Node * curr, String name, size_t remLen) {
    if (!curr) return 0;

    //printf("this: %s\n", name);
    bool importEnds = remLen == strlen(name);
    if (importEnds && !strcmp(name, "*")) return curr;
    if (importEnds) {
        String tmpStr = calloc(strlen(name) + 6, sizeof(char));
        sprintf(tmpStr, "%s.java", name);
        name = tmpStr;
    }

    Node * next = 0;
    for (unsigned int i = 0; i < curr->contLen; i++) {
        Node tmp = ((Node *) curr->cont)[i];
        //printf("name: %s, tmp; %s\n", name, tmp.nodeIdentifier);
        String shortTmpIdName = 0;
        for (size_t j = strlen(tmp.nodeIdentifier) - 1; j; j--) {
            if (tmp.nodeIdentifier[j] == '/') {
                shortTmpIdName = tmp.nodeIdentifier + j + 1;
                break;
            }
        }
        if (!strcmp(name, shortTmpIdName)) {
            //printf("next: %s\n", shortTmpIdName);
            next = &((Node *) curr->cont)[i];
            break;
        }
    }

    if (importEnds) free(name);
    if (importEnds || !next) return next;
    return findIncludeNode(next, name + strlen(name) + 1, remLen - strlen(name) - 1);
}

static FC *fc_append_include_t(FC *f, Node * root, String src) {
    String inc = calloc(strlen(src), sizeof(char) + 1);
    memcpy(inc, src, strlen(src));

    if (strlen(inc) < strlen("import  ;")) goto fin;
    size_t strPos = strlen("import ");
    for (; strPos < strlen(inc) && (inc[strPos] == ' ' || inc[strPos] == '\t' || inc[strPos] == '\n'); strPos++);
    for (size_t i = strlen(inc) - 1; i >= strPos; i--)
        if (inc[i] == ';' || inc[i] == ' ' || inc[i] == '\n' || inc[i] == '\t') inc[i] = '\0';
    if ((inc + strPos)[0] == '.') goto fin;
    //printf("import: %s\n", inc + strPos);

    String srcStart = inc + strPos;
    size_t srcLen = strlen(srcStart);
    String realIncludeSrc = calloc(srcLen + 1, sizeof(char));
    memcpy(realIncludeSrc, srcStart, srcLen);
    for (size_t i = 0; i < srcLen; i++) if (realIncludeSrc[i] == '.') realIncludeSrc[i] = '\0';
    Node * tar = findIncludeNode(root, realIncludeSrc, srcLen);
    if (tar) {
        //printf("src: %s, tarType: %d, tarID: %s\n",f->node.nodeIdentifier, tar->subType, tar->nodeIdentifier);
        f->includeSize++;
        if (!f->includes) f->includes = calloc(f->includeSize, sizeof(Node *));
        else f->includes = realloc(f->includes, f->includeSize * sizeof(Node *));
        f->includes[f->includeSize - 1] = tar;
    }// else printf("No such node\n");

    free(realIncludeSrc);

    fin:
    free(inc);
    return f;
}

static FC * fc_buildInclude_t (FC * fc, Node * root) {

    if (! fc || ! root) return fc;
    fd f = 0;
    if (fc->node.contLen) {
        size_t fileBeFuncLen = ((FM *)fc->node.cont)[0].startChar;
        String fileBeforeFunc = calloc(fileBeFuncLen, sizeof(char));
        size_t rem = fileBeFuncLen - 1;

        f = open(((Node *) fc)->nodeIdentifier, O_RDONLY);
        if (f == -1) {
            warn("FS::run()::include");
            return fc;
        }

        size_t res = read(f, fileBeforeFunc, rem);
        rem -= res;
        if (rem) {
            size_t pos = fileBeFuncLen - rem;
            res = read(f, fileBeforeFunc + pos, rem);
            rem -= res;
        }// printf("file before functions:\n%s\n", fileBeforeFunc);

        String import = fileBeforeFunc;

        while ((import = strstr(import, "import"))) {
            String splitter = strstr(import, ";");
            String importCont = calloc(splitter - import + 2, sizeof(char));
            memcpy(importCont, import, splitter - import + 1);
            //printf("this import: %s\n", importCont);
            fc->append_include(fc, root, importCont);
            free(importCont);
            import = splitter;
        } free(fileBeforeFunc);
    }

    if (f) close(f);
    return fc;
}

static bool fc_delete_t(FC *f) {
    if (!f) return f;
    Node *n = (Node *) f;
    if (!n) return 0;
    if (n->nodeIdentifier) free(n->nodeIdentifier);
    //printf("%d, %d\n", n->cont != 0, n->contDeleteFn != 0);
    if (n->cont && n->contDeleteFn) {
        //printf("in n delete cont\n");
        n->contDeleteFn(n);
    }

    if (f->includes) free(f->includes);
    memset(f, 0, sizeof(FC));
    free(f);
    f = 0;
    return 1;
}

FC *newFile() {
    FC *ret = calloc(sizeof(FC), 1);
    memset(ret, 0, sizeof(FC));
    ((Node *) ret)->contDeleteFn = dl_contFM;

    ret->append_include = fc_append_include_t;
    ret->buildInclude = fc_buildInclude_t;
    ret->delete = fc_delete_t;
    return ret;
}

/*
int sampleRunFileCus() {
    FC *f = newFile();

    Node * root = runDirScan(".");
    f = f->append_include(f, root, "import object;");
    f = f->append_include(f, root, "import object.tmp;");
    f = f->append_include(f, root, "import object.test;");
    f = f->append_include(f, root, "import object.*;");

    printf("include length: %lu\n", f->includeSize);
    for (size_t i = 0; i < f->includeSize; i++) printf("%s\n", f->includes[i]->nodeIdentifier);
    root->delete(root);
    return f->delete(f);
}*/
