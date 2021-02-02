
#include <string.h>
#include <err.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <zconf.h>
#include <ctype.h>

#include "dirScanner.h"
#include "stack.h"
#include "fnMeta.h"
#include "fileCus.h"

/*
 *
typedef struct dirScanner_t DS;

struct dirScanner_t {
    Node node;
    String dir;
    Node * dirTree;

    DS * (* scan) (DS *);
    DS * (* parseFile) (DS *);
    DS * (* buildInclude) (DS *, Node * root);
    DS * (* parseFn) (DS *, Node * root);
    bool (* delete) (DS *);
};

DS * newScanner (String);
 */

static bool dirTreeDeletable(void *node) {
    Node *this = (Node *) node;

    if (this->contLen > 1) {
        Node *last = &(((Node *) this->cont)[this->contLen - 1]);

        if (last->nodeIdentifier) free(last->nodeIdentifier);
        if (last->cont && last->contDeleteFn) last->contDeleteFn(last);
        memset(last, 0, sizeof(Node));

        this->contLen--;
        Node *tmpRm = calloc(this->contLen, sizeof(Node));
        memcpy(tmpRm, this->cont, this->contLen * sizeof(Node));
        free(this->cont);
        this->cont = tmpRm;
        return dirTreeDeletable(this);
    } else if (this->contLen == 1) {
        Node *thisContNode = (Node *) this->cont;
        thisContNode->delete(thisContNode);
    }

    if (this->subType == 8) {
        FC *f = (FC *) this->cont;
        return f->delete(f);
    }
    return 1;
}

static Node *internal_service_scan_t(Node *n) {
    if (!n || !n->nodeIdentifier) {
        warnx("Scanner: Invalid object node");
        return n;
    }
    DIR *dir = opendir(n->nodeIdentifier);
    if (!dir) {
        warn("%s scanner", n->nodeIdentifier);
        return n;
    }

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;

        n->contLen++;
        if (!n->cont) {
            n->cont = calloc(n->contLen, sizeof(Node));
        } else {
            void *oldContPtr = n->cont;
            n->cont = calloc(n->contLen, sizeof(Node));
            memcpy(n->cont, oldContPtr, (n->contLen - 1) * sizeof(Node));
            free(oldContPtr);
        }

        Node *tmpNewNode = newNode(DirTree, 0);
        tmpNewNode->changeType(tmpNewNode, DirTree);
        tmpNewNode->subType = de->d_type; // 4 for dir, 8 for file
        if (tmpNewNode->nodeIdentifier) free(tmpNewNode->nodeIdentifier);
        tmpNewNode->nodeIdentifier = calloc(strlen(n->nodeIdentifier) + strlen(de->d_name) + 2, sizeof(char));
        char tmpNewNodeNameGen[2096];
        sprintf(tmpNewNodeNameGen, "%s%s%s", n->nodeIdentifier, "/", de->d_name);
        strcpy(tmpNewNode->nodeIdentifier, tmpNewNodeNameGen);
        tmpNewNode->contDeleteFn = dirTreeDeletable;

        Node *this = &((Node *) n->cont)[n->contLen - 1];
        memcpy(this, tmpNewNode, sizeof(Node));
        free(tmpNewNode);
        tmpNewNode = 0;

        if (this->subType == 4)(void) internal_service_scan_t(this);
    }
    closedir(dir);
    return n;
}

static void *internal_service_travers_print_t(Node *n) {
    printf("%s (type %d)\n", n->nodeIdentifier, n->subType);
    return 0;
}

static String capFnCont(String fname, const ssize_t start, const ssize_t end) {
    String ret = 0;
    fd f = 0;
    if (end <= start) goto fin;
    f = open(fname, O_RDONLY);
    if (f == -1) {
        warn("FS::capFnCont()::open()");
        goto fin;
    }
    ssize_t filePtr = 0;
    size_t proc = 0, rem = end - start + 1;
    (void) proc;
    ret = calloc(rem + 1, sizeof(char));
    memset(ret, 0, rem + 1);

    char buff[BUFF_LEN];
    ssize_t res = 0;
    while ((res = read(f, buff, BUFF_LEN))) {
        if (res == -1) {
            warn("FS::capFnCont()::read()");
            free(ret);
            ret = 0;
            goto fin;
        }
        filePtr += res;
        if (filePtr < start) continue;
        ssize_t thisBuffStart = start <= filePtr - res ? 0 : start - (filePtr - res) - 1; // first char will be at pos 0
        ssize_t thisBuffLen = end >= filePtr ? res - thisBuffStart : res - (filePtr - end) - thisBuffStart;
        //printf("this post: (%ld) [%ld : %ld]\n", res, thisBuffStart, thisBuffLen);
        //res = write(1, buff + thisBuffStart, thisBuffLen);
        //(void) res;
        memcpy(ret + strlen(ret), buff + thisBuffStart, thisBuffLen);
        if (filePtr >= end) goto fin;
    }

    fin:
    if (f) close(f);
    return ret;
}

#define DEBUG 0
#define RESULT 0
#define TEST 0
const char* KEYWORDS[] = {"if", "for", "switch", "while", "catch", "return"};

char * toLowerCase(char * str){
    for (unsigned long i = 0;i < strlen(str); ++ i)
        str[i] = (char)tolower(str[i]);
    return str;
}

bool isKeyword(char * fun){

//    printf("isKeyword(): %s\n", fun);
    for (int i = 0;i < 6; ++ i){
//        printf("%s, %s\n", fun, KEYWORDS[i]);
        if (strcmp(fun,KEYWORDS[i]) == 0){
            return true;
        }
    }
    return false;
}

static void fnSplit(FM* fnNode){
    //identify modifier
    if (TEST)printf("Start fnSplit\n");

    if (DEBUG)printf("fncont: %s\n", fnNode->cont);

    char str[15];
    memset(str, 0, sizeof(str));
    strncpy(str,fnNode->cont,14);
    char * sep = strtok(str, " ");
    if (DEBUG)printf("sep: %s\n",sep);
    if (strcmp(sep,"public") == 0) fnNode->modifier = PUBLIC;
    else if (strcmp(sep, "private") == 0) fnNode->modifier = PRIVATE;
    else if (strcmp(sep, "protected") == 0) fnNode->modifier = PROTECTED;
    else fnNode->modifier = DEFAULT;
    sep = 0;
    free(sep);
    if (DEBUG)printf("Finished set modifier.\n");

    int fnStart = 0, fnEnd = 0, end = -1;
    int isSpace = 0;
    while (fnNode->cont[fnEnd] != '('){
        if(DEBUG)printf("%d:%c, %d:%c, %d\n", fnStart, fnNode->cont[fnStart], fnEnd, fnNode->cont[fnEnd], isSpace);
        if (fnNode->cont[fnEnd] == ' '){
            end = fnEnd;
            isSpace = 1;
        }
        else if (isSpace){
            isSpace = 0;
            fnStart = fnEnd;
        }
        fnEnd ++;
    }
    if (end < fnStart) end = fnEnd;
    fnNode->n.nodeIdentifier = malloc( (end - fnStart + 2) * sizeof(char));
    memset(fnNode->n.nodeIdentifier, 0, (end - fnStart + 2) * sizeof(char));
    strncpy(fnNode->n.nodeIdentifier, fnNode->cont + fnStart, end - fnStart);
    if(RESULT)printf("Fun Identifier: %s\n", fnNode->n.nodeIdentifier);

    fnStart = -1;
    end = -1;

    bool isCommentOn = false;
    bool isLongComment = false;
    bool isStringOpen = false;
    int isComment = -2;

    fnNode->calleeLen = 0;
    fnNode->pemLen = 0;

    if (DEBUG)printf("Start split:\n");

    while (fnNode->cont[fnEnd] != '{'){

        fnEnd ++;
    }
    if (DEBUG)printf("{ found, fnEnd = %d\n",fnEnd);
    if (DEBUG)printf("Current: fnStart: %d, fnEnd: %d\n", fnStart, fnEnd);
    fnEnd ++;
    while ((unsigned long)fnEnd < strlen(fnNode->cont)) {
        if (DEBUG)printf("Work on: fnStart: %d, fnEnd: %d\n", fnStart, fnEnd);
        if (DEBUG)printf("Flags: isCommentOn: %d, isLongComment: %d, indexComment: %d, isStringOpen: %d\n",
                         isCommentOn, isLongComment, isComment, isStringOpen);
        //check if comments
        if (fnNode->cont[fnEnd] == '/'){
            if (isCommentOn == false){
                if (fnEnd - isComment != 1) fnEnd = isComment;
                else isCommentOn = true;
            }
            else {
                if (fnEnd - isComment == 1) isCommentOn = false;
            }
            fnEnd++;
            continue;
        }
        if (fnNode->cont[fnEnd] == '*'){
            if (isCommentOn == false){
                if (fnEnd - isComment == 1) {
                    isCommentOn = true;
                    isLongComment = true;
                }
            }
            else {
                isComment = fnEnd;
            }
            fnEnd++;
            continue;
        }

        if (fnNode->cont[fnEnd] == '\n' && isCommentOn == true && isLongComment == false){
            isCommentOn = false;
            fnEnd++;
            continue;
        }
        if (DEBUG)printf("filter out comments\n");

        //check if string("...")
        if (fnNode->cont[fnEnd] == '\"'){
            if (!isStringOpen) isStringOpen = true;
            else isStringOpen = false;
        }

        if (isStringOpen){
            fnEnd++;
            continue;
        }
        if (DEBUG)printf("filter out strings\n");


        //check if "\..."
        if (fnNode->cont[fnEnd] == '\\'){
            fnEnd = fnEnd+2;
            continue;
        }

        if (DEBUG)printf("filter out ...\n");

        if ((fnStart == -1) && ((fnNode->cont[fnEnd] >= 'a'&& fnNode->cont[fnEnd] <= 'z') ||
                                (fnNode->cont[fnEnd] >= 'A' && fnNode->cont[fnEnd] <= 'Z'))){
            if (DEBUG)printf("Found possible head of functions: %c\n",fnNode->cont[fnEnd]);
            fnStart = fnEnd;
            fnEnd++;
            if (DEBUG)printf("finished: fnStart = %d [%c], fnEnd = %d [%c]\n",
                             fnStart, fnNode->cont[fnStart], fnEnd, fnNode->cont[fnEnd]);
            continue;

        }
        if (fnStart != -1 && ((fnNode->cont[fnEnd] >= 'a'&& fnNode->cont[fnEnd] <= 'z') ||
                              (fnNode->cont[fnEnd] >= 'A' && fnNode->cont[fnEnd] <= 'Z')||
                              (fnNode->cont[fnEnd] >= '0' && fnNode->cont[fnEnd] <= '9')||
                              fnNode->cont[fnEnd] == '.'|| fnNode->cont[fnEnd] == '_')){
            if (DEBUG)printf("Legal function char\n");
            fnEnd++;
            if (DEBUG)printf("finished: fnStart = %d [%c], fnEnd = %d [%c]\n",
                             fnStart, fnNode->cont[fnStart], fnEnd, fnNode->cont[fnEnd]);
            continue;


        }

        if (fnStart != -1){
            if (DEBUG)printf("Found not legal function char\n");
            if (fnNode->cont[fnEnd] == ' ' || fnNode->cont[fnEnd] =='(') {
                if (DEBUG)printf("filter spaces\n");
                end = fnEnd;
                while (fnNode->cont[fnEnd] == ' ') {
                    fnEnd++;
                }
                if (DEBUG)printf("Current fnEnd: %d [%c]\n", fnEnd, fnNode->cont[fnEnd]);
                if (fnNode->cont[fnEnd] == '(') {
                    if (DEBUG)printf("Function found %d %d %d\n", fnStart, fnEnd, end);
                    //filter out for, while, switch, catch, if, return
                    char * funName = malloc( (end - fnStart + 2) * sizeof(char));
                    memset(funName, 0, (end - fnStart + 2) * sizeof(char));
                    strncpy(funName, fnNode->cont + fnStart, end - fnStart);
                    if (isKeyword(funName)) {
                        fnStart = -1;
                        fnEnd++;
                        continue;
                    }
                    if(DEBUG)printf("calleeLen: %d\n", (int)fnNode->calleeLen);
                    if (fnNode->calleeLen == 0){
                        fnNode->fnCallee = malloc(sizeof(String));
                    }
                    else
                        fnNode->fnCallee = realloc(fnNode->fnCallee, (fnNode->calleeLen + 1)* sizeof(String));
                    fnNode->fnCallee[fnNode->calleeLen] = funName;
                    if(DEBUG)printf("Function: %s\n", funName);
                    fnNode->calleeLen++;
                    fnStart = -1;
                    fnEnd++;
                    //check if permission
//                    if (strstr(toLowerCase(funName),"permission")!= 0){
//                        char * pemName = malloc(strlen(funName) * sizeof(char));
//                        memset(pemName,0,strlen(funName) * sizeof(char));
//                        strcpy(pemName,funName);
//                        if (fnNode->pemLen == 0)
//                            fnNode->pem = malloc(sizeof(String));
//                        else
//                            fnNode->pem = realloc(fnNode->pem,(fnNode->pemLen + 1) * sizeof(String));
//                        fnNode->pem[fnNode->pemLen] = pemName;
//                        if(RESULT) printf("Permission: %s\n", pemName);
//                        fnNode->pemLen++;
//                    }

                    if (DEBUG)printf("finished: fnStart = %d [%c], fnEnd = %d [%c]\n",
                                     fnStart, fnNode->cont[fnStart], fnEnd, fnNode->cont[fnEnd]);
                    funName = 0;
                    free(funName);
                    continue;
                }
                else{
                    if (DEBUG)printf("Not function, reset\n");
                    //illegal
                    fnStart = -1;
                    if (DEBUG)printf("finished: fnStart = %d [%c], fnEnd = %d [%c]\n",
                                     fnStart, fnNode->cont[fnStart], fnEnd, fnNode->cont[fnEnd]);
                    continue;
                }
            }
            else {
                if (DEBUG)printf("Not function , rest\n");
                //illegal
                fnStart = -1;
                if (DEBUG)printf("finished: fnStart = %d [%c], fnEnd = %d [%c]\n",
                                 fnStart, fnNode->cont[fnStart], fnEnd, fnNode->cont[fnEnd]);
                continue;
            }

        }

        if (DEBUG)printf("finished: fnStart = %d [%c], fnEnd = %d [%c]\n",
                         fnStart, fnNode->cont[fnStart], fnEnd, fnNode->cont[fnEnd]);
        fnEnd ++;
    }

    // printf("(fn)Function count: %zu\n", fnNode->calleeLen);
    if(TEST)printf("End fnSplit\n");
}

static void * internal_extractFn_t(Node *n){
    // printf("Extract Fn: %s\n", n->nodeIdentifier);
    if (n->cont == NULL) return n;
    FC * fnList = n->cont;
//    printf("FC: %u\n", fnList);
    if (DEBUG) printf("Cont len: %u\n", fnList->node.contLen);
    for (size_t i = 0; i < fnList->node.contLen; i++) {
        FM* p_fm = &((FM *) fnList->node.cont)[i];
        fnSplit(p_fm);
        if (DEBUG) printf("Start: %u, End: %u\n", p_fm->startChar, p_fm->endChar);
        p_fm = 0;
        free(p_fm);
    }
    fnList = 0;
    free(fnList);
//    ((FM *) realRet->node.cont)[i]
    return n;
}



static void * internal_extractPm_t(Node *n){
    // printf("Node: %s\n", n->nodeIdentifier);
    if (n->cont == NULL) return n;
    FC * fnList = n->cont;
    for (size_t i = 0; i < fnList->node.contLen; i++) {
        FM* p_fm = &(((FM *) fnList->node.cont)[i]);
        // printf("(pm)Function count: %zu\n", p_fm->calleeLen);
        for (size_t j = 0;j < p_fm->calleeLen; j ++){
            String fnName = p_fm->fnCallee[j];
            // printf("Function name: %s\n", fnName);
            String fnNameLow = calloc(strlen(fnName) + 1, sizeof(char));
            strcpy(fnNameLow, fnName);
            toLowerCase(fnNameLow);
            if (strstr(fnNameLow,"permission")!= 0) {
                char * pemName = malloc(strlen(fnName) * sizeof(char) + 1);
                memset(pemName,0,strlen(fnName) * sizeof(char));
                strcpy(pemName,fnName);
                if (p_fm->pemLen == 0){

                    p_fm->pem = malloc(sizeof(String));
                }
                else
                    p_fm->pem = realloc(p_fm->pem,(p_fm->pemLen + 1) * sizeof(String));
                p_fm->pem[p_fm->pemLen] = pemName;
                if(RESULT) printf("Permission: %s\n", pemName);
                p_fm->pemLen++;
                pemName = 0;
                free(pemName);
            }
        }
        p_fm = 0;
        free(p_fm);
//        fm.fnCallee;
//        printf("callee: s\n",fm.fnCallee);
    }
    fnList = 0;
    free(fnList);
    return n;
}

static void *internal_service_travers_parseFn_t(Node *n) {
    if (!n) return 0;
    String file = n->nodeIdentifier;
    if (strcmp(file + strlen(file) - strlen(".java"), ".java")) return 0;
    //printf("parsing: %s\n", file);

    fd f = open(file, O_RDONLY);
    if (f == -1) {
        warn("FS::run()");
        return 0;
    }

    Stack *s = newStack(sizeof(char), 0);
    FM *ret = 0;
    bool inFn = false;
    bool posSetting = false;

    char buff[BUFF_LEN];
    memset(buff, 0, BUFF_LEN);
    ssize_t res;
    int fnCounter = 0;

    ssize_t currCharPos = -1, currBegin = 0, currTerm = 0;

    for ((res = read(f, buff, BUFF_LEN));; (res = read(f, buff, BUFF_LEN))) {
        if (res == -1) {
            warn("FS::run()");
            break;
        }
        if (!res) break;

        for (ssize_t i = 0; i < res; i++) {
            currCharPos++;
            if (s->count(s) == 1 && !inFn && !posSetting && buff[i] != '\n' && buff[i] != ' ') {
                currBegin = currCharPos;
                posSetting = true;
            }
            if (posSetting && !inFn && buff[i] == ';') posSetting = false;

            if (buff[i] == '{') {
                s->push(s, &buff[i]);
                if (s->count(s) > 1) inFn = true;
            } else if (buff[i] == '}') {
                free(s->pop(s));
                if (s->count(s) == 1) {
                    fnCounter++;
                    inFn = false;
                    posSetting = false;
                    currTerm = currCharPos;
                    if (fnCounter == 1)
                        ret = calloc(1, sizeof(FM));
                    else ret = realloc(ret, fnCounter * sizeof(FM));
                    ret[fnCounter - 1].startChar = currBegin;
                    ret[fnCounter - 1].endChar = currTerm;
                }
            }
        }
    }
    close(f);
    //printf("Total function count %d\n", fnCounter);

    FC *realRet = newFile();
    ((Node *) realRet)->nodeIdentifier = calloc(strlen(file) + 1, sizeof(char));
    memcpy(((Node *) realRet)->nodeIdentifier, file, strlen(file));
    realRet->node.cont = ret;
    realRet->node.contLen = fnCounter;

    for (size_t i = 0; i < realRet->node.contLen; i++) {
        //printf("Function %lu/%u: [%u:%u]\n", i + 1, realRet->node.contLen, ((FM *)realRet->node.cont)[i].startChar, ((FM *)realRet->node.cont)[i].endChar);
        ((FM *) realRet->node.cont)[i].startChar++;
        ((FM *) realRet->node.cont)[i].endChar++;
        ((FM *) realRet->node.cont)[i].cont = capFnCont(file, ((FM *) realRet->node.cont)[i].startChar,
                                                        ((FM *) realRet->node.cont)[i].endChar);
        //if (((FM *)realRet->node.cont)[i].cont) printf("%s\n", ((FM *)realRet->node.cont)[i].cont);
    }

    s->delete(s);
    if (n->cont) free(n->cont);
    return n->cont = realRet;
}

static void * internal_service_travers_printBI_t (Node *n) {
    FC * f = (FC *) n->cont;
    if (strcmp(n->nodeIdentifier + strlen(n->nodeIdentifier) - strlen(".java"), ".java")) return 0;
    for (size_t i = 0; i < f->includeSize; i++) {
        printf("%s -> %s\n", f->node.nodeIdentifier, f->includes[i]->nodeIdentifier);
    } return 0;
}

static void * internal_service_travers_printCallee_t (Node *n) {
    if (strcmp(n->nodeIdentifier + strlen(n->nodeIdentifier) - strlen(".java"), ".java")) return 0;
    FC * f = (FC *) n->cont;
    for (size_t i = 0; i < f->node.contLen; i++) {
        FM * fm = &((FM *) f->node.cont)[i];
        for (size_t j = 0; j < fm->calleeLen; j++) {
            if (!fm->calleeResides[j]) continue;
            printf("%s::%s() -> %s\n", n->nodeIdentifier, fm->n.nodeIdentifier, fm->fnCallee[j]);
            printf("Callee equivalents to %s::%s()\n", fm->calleeResides[j] ? fm->calleeResides[j]->nodeIdentifier : "(nil)", fm->calleeShort[j]);
        }
    } return 0;
}

static void * internal_service_travers_printPem_t (Node *n) {
    if (strcmp(n->nodeIdentifier + strlen(n->nodeIdentifier) - strlen(".java"), ".java")) return 0;
    FC * f = (FC *) n->cont;
    for (size_t i = 0; i < f->node.contLen; i++) {
        FM * fm = &((FM *) f->node.cont)[i];
        for (size_t j = 0; j < fm->pemLen; j++) printf("%s::%s(), %s\n", n->nodeIdentifier, fm->n.nodeIdentifier, fm->pem[j]);
    } return 0;
}

static void * internal_service_travers_pemCheck2_t (Node *n) {
    if (strcmp(n->nodeIdentifier + strlen(n->nodeIdentifier) - strlen(".java"), ".java")) return 0;
    FC * f = (FC *) n->cont;
    for (size_t i = 0; i < f->node.contLen; i++) {
        FM * fm = &((FM *) f->node.cont)[i];
        String pem = fm->cont;
        while ((pem = strstr(pem, "android.permission"))) {
            size_t pemStrLength = strlen("android.permission");
            for (; (pem + pemStrLength)[0] != '\"' && (pem + pemStrLength)[0] != '\''; pemStrLength++);
            String pemStr = calloc(pemStrLength + 1, sizeof(char));
            memcpy(pemStr, pem, pemStrLength);
            //printf("%s\n", pemStr);
            fm->pemLen++;
            if (fm->pemLen == 1) fm->pem = calloc(1, sizeof(String));
            else fm->pem = realloc(fm->pem, fm->pemLen * sizeof(String));
            fm->pem[fm->pemLen - 1] = pemStr;
            pem ++;
        }
    } return 0;
}

static void * internal_service_travers_pemCheck3_t (Node *n) {
    if (strcmp(n->nodeIdentifier + strlen(n->nodeIdentifier) - strlen(".java"), ".java")) return 0;
    FC * f = (FC *) n->cont;
    for (size_t i = 0; i < f->node.contLen; i++) {
        FM * fm = &((FM *) f->node.cont)[i];
        String pem = fm->cont;
        while ((pem = strstr(pem, "Manifest.permission"))) {
            size_t pemStrLength = strlen("Manifest.permission");
            for (; (pem + pemStrLength)[0] != '\"' && (pem + pemStrLength)[0] != '\''; pemStrLength++);
            String pemStr = calloc(pemStrLength + 1, sizeof(char));
            memcpy(pemStr, pem, pemStrLength);
            //printf("%s\n", pemStr);
            fm->pemLen++;
            if (fm->pemLen == 1) fm->pem = calloc(1, sizeof(String));
            else fm->pem = realloc(fm->pem, fm->pemLen * sizeof(String));
            fm->pem[fm->pemLen - 1] = pemStr;
            pem ++;
        }
    } return 0;
}

static void internal_service_travers_t(Node *n, int tarType, void *(*serviceFn)(Node *)) {
    if (!n) return;
    for (unsigned int i = 0; i < n->contLen; i++) {
        //printf("%d: %d\n", ((Node *) n->cont)[i].subType, tarType);
        if (((Node *) n->cont)[i].subType == tarType) {
            //printf("Should go parse service\n");
            serviceFn(&((Node *) n->cont)[i]);
        }
        if (((Node *) n->cont)[i].subType == 4) internal_service_travers_t(&((Node *) n->cont)[i], tarType, serviceFn);
    }
}

static DS *scan_t(DS *s) {
    if (!s || !s->dir) return s;
    Node *root = newNode(DirTree, s->dir);
    root->contDeleteFn = dirTreeDeletable;
    s->dirTree = internal_service_scan_t(root);
    s->dirTree->subType = 4;
    return s;
}

static DS *printTree_t(DS *s) {
    if (!s || !s->dirTree) return s;
    internal_service_travers_t(s->dirTree, 8, internal_service_travers_print_t);
    return s;
}

static DS *parse_t(DS *s) {
    //printf("in parse\n");
    internal_service_travers_t(s->dirTree, 8, internal_service_travers_parseFn_t);
    return s;
}

static DS * extractFn_t(DS *s){
    internal_service_travers_t(s->dirTree, 8, internal_extractFn_t);
    return s;
}

static DS * extractPm_t(DS *s){
    internal_service_travers_t(s->dirTree, 8, internal_extractPm_t);
    return s;
}

static void internal_service_BI(Node *n, Node *root) {
    if (!n) return;
    for (unsigned int i = 0; i < n->contLen; i++) {
        //printf("%d: %d\n", ((Node *) n->cont)[i].subType, tarType);
        if (((Node *) n->cont)[i].subType == 8) {
            //printf("Should go parse service\n");
            // printf("%s\n", ((Node *) n->cont)[i].nodeIdentifier);
            FC *f = (FC *) ((Node *) n->cont)[i].cont;
            if (!strcmp(((Node *) n->cont)[i].nodeIdentifier + strlen(((Node *) n->cont)[i].nodeIdentifier) -
                        strlen(".java"), ".java"))
                f->buildInclude(f, root);
        }
        if (((Node *) n->cont)[i].subType == 4) internal_service_BI(&((Node *) n->cont)[i], root);
    }
}

static DS *buildInclude_t(DS *s, Node *root) {
    internal_service_BI(root, root);
    return s;
}

static DS * printBI_t (DS * s) {
    internal_service_travers_t(s->dirTree, 8, internal_service_travers_printBI_t);
    return s;
}

static DS * printCallee_t(DS * s) {
    internal_service_travers_t(s->dirTree, 8, internal_service_travers_printCallee_t);
    return s;
}

static DS * printPem_t(DS * s) {
    internal_service_travers_t(s->dirTree, 8, internal_service_travers_printPem_t);
    return s;
}

static DS * pemCheck2_t (DS * s) {
    internal_service_travers_t(s->dirTree, 8, internal_service_travers_pemCheck2_t);
    internal_service_travers_t(s->dirTree, 8, internal_service_travers_pemCheck3_t);
    return s;
}

static bool pemTrack_append_list_extCheck (Node * head, String FID, FM * fm) {
    if (! head) return 0;
    if (!FID || !fm) return head;
    for (; head->prev; head = head->prev);
    for (Node * tmp = head; tmp; tmp = tmp->next) {
        if (!strcmp(FID, tmp->nodeIdentifier) && fm == tmp->cont) return 1;
    } return 0;
}

static Node * pemTrack_append_list (Node * head, Node * tail, String FID, FM * fm) {
    if (!FID || !fm) return head;
    if (pemTrack_append_list_extCheck(head, FID, fm)) {
        // warnx("target exists");
        return head;
    }
    Node * n = newNode(NT_LINKEDLIST, FID);
    n->cont = fm;
    if (!head) return n;

    tail = head;
    while (tail->next) tail = tail->next;
    tail->next = n;
    tail->next->prev = tail;
    tail = tail->next;
    return head;
}

static Node * internal_service_pemTrack_findParent(Node * subRoot, const Node * n, Node * ret) {
    if (! subRoot || ! n) return ret;
    // printf("%s: (T%u: %u)\n", subRoot->nodeIdentifier, subRoot->subType, subRoot->contLen);
    if (subRoot->subType != 4) return ret;
    Node * update = 0;
    for (size_t i = 0; i < subRoot->contLen; i++) {
        // printf("now: %s %s\n", n->nodeIdentifier, ((Node *) subRoot->cont)[i].nodeIdentifier);
        if (strstr(n->nodeIdentifier, ((Node *) subRoot->cont)[i].nodeIdentifier) && ((Node *) subRoot->cont)[i].subType == 4) {
            update = &((Node *) subRoot->cont)[i];
            // printf("update: %s\n", update->nodeIdentifier);
            break;
        }
    }
    if (!update) return ret;
    return internal_service_pemTrack_findParent(update, n, update);
}

static bool cmpNothPathNLevName(String id, String levelName) {
    if (!id || !levelName) return 0;
    String end = id + strlen(id) - 1;
    for (; end[0] != '/'; end--);
    end++;
    return !strcmp(end, levelName);
}

static Node * internal_service_pemTrack_findChild(Node * parent, String thisLevelName) {
    if (!parent || parent->subType != 4) return 0;
    // printf("parent: %s, thisLevel: %s\n", parent->nodeIdentifier, thisLevelName);
    for (size_t i = 0; i < parent->contLen; i++) {
        if (((Node *) parent->cont)[i].subType == 4 && cmpNothPathNLevName(((Node *) parent->cont)[i].nodeIdentifier, thisLevelName)) return &((Node *) parent->cont)[i];
        else if (((Node *) parent->cont)[i].subType == 8) {
            String levelNameTmp = calloc(strlen(thisLevelName) + 6, sizeof(char));
            sprintf(levelNameTmp, "%s.java", thisLevelName);
            if (cmpNothPathNLevName(((Node *) parent->cont)[i].nodeIdentifier, levelNameTmp)) {
                free(levelNameTmp);
                return &((Node *) parent->cont)[i];
            } free (levelNameTmp);
        }
    }
    return 0;
}

static Node * pemTrack_L1_scan(Node * head, Node * tail, Node * n, String pem) {
    if (!n || !pem) return head;
    for (unsigned int i = 0; i < n->contLen; i++) {
        Node * curr = &((Node *) n->cont)[i];
        if (curr->subType == 8) {
            FC * fc = (FC *) curr->cont;
            for (size_t fmCount = 0; fmCount < fc->node.contLen; fmCount++) {
                FM * fm = &((FM *) fc->node.cont)[fmCount];
                for (size_t pemCount = 0; pemCount < fm->pemLen; pemCount++) {
                    String currPem = fm->pem[pemCount];
                    if (!strcmp(currPem, pem)) {
                        head = pemTrack_append_list(head, tail, fc->node.nodeIdentifier, fm);
                        tail = head;
                        while (tail->next) tail = tail->next;
                    }
                }
            }
        }
        if (curr->subType == 4) head = pemTrack_L1_scan(head, tail, &((Node *) n->cont)[i], pem);
    } return head;
}

static bool findRealFileAndFnForInclude(Node * inc, String tarFn) {
    if (!inc || !tarFn) return 0;
    if (strstr(tarFn, ".")) return 0;
    FC * fc = (FC *) inc->cont;
    for (size_t i = 0; i < fc->node.contLen; i++) {
        FM * fm = &((FM *) fc->node.cont)[i];
        if (!strcmp(fm->n.nodeIdentifier, tarFn)) return 1;
    } return 0;
}

static Node * findRealFileAndFn (DS * s, Node * thisFile, String tarFn) {
    // printf("input file: %s, fn: %s\n", thisFile->nodeIdentifier, tarFn);
    if (!strstr(tarFn, ".")) return thisFile;
    Node * parent = internal_service_pemTrack_findParent(s->dirTree, thisFile, 0);
    // printf("Found parent: %s\n", parent->nodeIdentifier);

    String thisLevel = calloc(strstr(tarFn, ".") - tarFn + 1, sizeof(char));
    strncpy(thisLevel, tarFn, strstr(tarFn, ".") - tarFn);
    // printf("This level: %s\n", thisLevel);
    Node * nextRecursion = internal_service_pemTrack_findChild(parent, thisLevel);
    free(thisLevel);
    if (! nextRecursion) {
        // printf("Next recussion: nil\n");
        return 0;
    } // printf("Next recussion: %s::%s()\n", nextRecursion->nodeIdentifier, strstr(tarFn, ".") + 1);
    return findRealFileAndFn(s, nextRecursion, strstr(tarFn, ".") + 1);
}

static bool ifFnExists(FC * f, String fn) {
    if (!f || !fn) return 0;
    for (size_t i = 0; i < f->node.contLen; i++) if (!strcmp(((FM *)f->node.cont)[i].n.nodeIdentifier, fn)) return 1;
    return 0;
}

static Node * pemTrack_L2_analysis(DS * s, Node * head, Node * tail, Node * n) {
    if (!n) return head;
    for (unsigned int i = 0; i < n->contLen; i++) {
        Node * curr = &((Node *) n->cont)[i];
        if (curr->subType == 8) {
            FC * fc = (FC *) curr->cont;
            for (size_t fmCount = 0; fmCount < fc->node.contLen; fmCount++) {
                FM * fm = &((FM *) fc->node.cont)[fmCount];
                fm->calleeShort = calloc(fm->calleeLen, sizeof(String));
                fm->calleeResides = calloc(fm->calleeLen, sizeof(Node *));
                for (size_t pemCount = 0; pemCount < fm->calleeLen; pemCount++) {
                    String currPem = fm->fnCallee[pemCount];
                    // printf("From %s::%s() called %s()\n", curr->nodeIdentifier, fm->n.nodeIdentifier, currPem);
                    Node * residesFile = findRealFileAndFn(s, curr, currPem);
                    String shortFnName = currPem + strlen(currPem) - 1;
                    if (strstr(currPem, ".")) {
                        for (;shortFnName[0] != '.'; shortFnName--);
                        shortFnName++;
                    } else shortFnName = currPem;
                    // printf("Real file: %s::%s()\n", residesFile ? residesFile->nodeIdentifier : "(nil)", shortFnName);

                    if (!residesFile) {
                        // printf("From %s::%s() called %s()\n", curr->nodeIdentifier, fm->n.nodeIdentifier, currPem);
                        for (size_t incCount = 0; incCount < fc->includeSize; incCount++) {
                            String tarClassName = calloc(strstr(currPem, ".") - currPem + 1 + 6, sizeof(char));
                            memset(tarClassName, 0, strstr(currPem, ".") - currPem + 1 + 6);
                            strncpy(tarClassName + 1, currPem, strstr(currPem, ".") - currPem);
                            tarClassName[0] = '/';
                            tarClassName[strlen(tarClassName)] = '.';
                            tarClassName[strlen(tarClassName)] = 'j';
                            tarClassName[strlen(tarClassName)] = 'a';
                            tarClassName[strlen(tarClassName)] = 'v';
                            tarClassName[strlen(tarClassName)] = 'a';
                            // printf("Target class name: %s\n", tarClassName);
                            if (strstr(fc->includes[incCount]->nodeIdentifier, tarClassName) && findRealFileAndFnForInclude(fc->includes[incCount], strstr(currPem, ".") + 1)) {
                                // printf("Current include: %s\n", fc->includes[incCount]->nodeIdentifier);
                                residesFile = fc->includes[incCount];
                                // printf("Found file: %s\n", residesFile->nodeIdentifier);
                                break;
                            }
                        }
                    }

                    if (residesFile && ifFnExists((FC *)residesFile->cont, shortFnName)) {
                        fm->calleeShort[pemCount] = shortFnName;
                        fm->calleeResides[pemCount] = residesFile;
                    } else  {
                        fm->calleeShort[pemCount] = 0;
                        fm->calleeResides[pemCount] = 0;
                    }
                }
            }
        }
        if (curr->subType == 4) head = pemTrack_L2_analysis(s, head, tail, &((Node *) n->cont)[i]);
    } return head;
}

static Node * internal_service_pemTrack_L3_pemMatch (String calleeURI, String calleeFN, Node * head) {
    // printf("in here\n");
    if (!calleeURI || !calleeFN || !head) return 0;
    for (; head->prev; head = head->prev);
    for (Node * tmp = head; tmp; tmp = tmp->next) {
        // mprintf("Callee URI: %s\npemURI: %s\nCallee fn: %s\npemFn: %s\n", calleeURI, tmp->nodeIdentifier, calleeFN, ((FM *)tmp->cont)->n.nodeIdentifier);
        if (!strcmp(tmp->nodeIdentifier, calleeURI) && !strcmp(((FM *)tmp->cont)->n.nodeIdentifier, calleeFN)) return tmp;
    } return 0;
}

static Node * pemTrack_L3_searchCallers (DS * s, Node * n, Node * head) {
    if (!s || !head || !n) return head;
    for (unsigned int i = 0; i < n->contLen; i++) {
        if (((Node *) n->cont)[i].subType == 8) {
            Node * thisNode = &((Node *) n->cont)[i];
            FC * fc = (FC *) thisNode->cont;
            for (size_t fmCount = 0; fmCount < fc->node.contLen; fmCount++) {
                FM * fm = & ((FM *) fc->node.cont)[fmCount];
                for (size_t calleeCount = 0; calleeCount < fm->calleeLen; calleeCount++) {
                    Node * pemMatched = 0;
                    // printf("%p\n", (void *) fm->calleeResides[calleeCount]);
                    if ((pemMatched = internal_service_pemTrack_L3_pemMatch(fm->calleeResides[calleeCount] ? fm->calleeResides[calleeCount]->nodeIdentifier : 0, fm->calleeShort[calleeCount] ? fm->calleeShort[calleeCount] : 0, head))) {
                        // printf("%s::%s() triggered %s::%s()\n", fc->node.nodeIdentifier, fm->n.nodeIdentifier, pemMatched->nodeIdentifier, ((FM *) pemMatched->cont)->n.nodeIdentifier);
                        Node * tail = head;
                        while (tail->next) tail = tail->next;
                        pemTrack_append_list (head, tail, fc->node.nodeIdentifier, fm);
                    }
                }
            }
        }
        if (((Node *) n->cont)[i].subType == 4) pemTrack_L3_searchCallers(s, &((Node *) n->cont)[i], head);
    } return head;
}

static size_t countLinkdedListSize(Node * head) {
    if (! head) return 0;
    for (; head->prev; head = head->prev);
    size_t ret = 0;
    for (Node * tmp = head; tmp && ++ret; tmp = tmp->next);
    return ret;
}

static DS * pemTrack_t (DS * s, const String pem) {
    Node * head = 0;
    Node * tail = 0;
    head = pemTrack_L1_scan(head, tail, s->dirTree, pem); // Find out all functions directly calling such permission
    tail = head;
    while (tail->next) tail = tail->next;
    // printf("%p\n", (void *) head);
    head = pemTrack_L2_analysis(s, head, tail, s->dirTree); // Make all callees in the format of Node and String.
    printf("\n");

    size_t pemFnCount = 0;
    for (;;) {
        head = pemTrack_L3_searchCallers(s, s->dirTree, head);
        for (;head->prev; head = head->prev);
        size_t countAfterUpdate = countLinkdedListSize(head);
        if (countAfterUpdate == pemFnCount) break;
        pemFnCount = countAfterUpdate;
    }

    printf("%lu functions captured for permission %s\n", pemFnCount, pem);
    for (Node * tmp = head; tmp; tmp = tmp->next) printf("%s::%s\n", tmp->nodeIdentifier, ((FM *)tmp->cont)->n.nodeIdentifier);

    for (Node * tmp = head; tmp;) {
        head = head->next;
        free(tmp);
        tmp = head;
    }

    return s;
}

static bool delete_t(DS *s) {
    if (!s) return 1;
    if (s->dir) free(s->dir);

    // internal_service_travers_t(s->dirTree, 8, internal_service_travers_cleanCallee_t);
    Node *n = &s->node;
    if (n->nodeIdentifier) free(n->nodeIdentifier);
    if (s->dirTree) {
        s->dirTree->delete(s->dirTree);
    }

    memset(s, 0, sizeof(DS));
    free(s);
    s = 0;
    return 1;
}

DS *newScanner(String dir) {
    DS *ret = calloc(1, sizeof(DS));
    memset(ret, 0, sizeof(DS));

    Node *tmpNode = newNode(NT_SCANNER, 0);
    memcpy(ret, tmpNode, sizeof(Node));
    free(tmpNode);
    if (dir) {
        ret->dir = calloc(strlen(dir) + 1, sizeof(char));
        strcpy(ret->dir, dir);
    }

    ret->scan = scan_t;
    ret->printDirTree = printTree_t;
    ret->parseFile = parse_t;
    ret->extractFunction = extractFn_t;
    ret->extractPermission = extractPm_t;
    ret->buildInclude = buildInclude_t;
    ret->printIncludes = printBI_t;
    ret->printCallee = printCallee_t;
    ret->printPem = printPem_t;
    ret->pemCheck2 = pemCheck2_t;
    ret->pemTrack = pemTrack_t;
    ret->delete = delete_t;
    return ret;
}
