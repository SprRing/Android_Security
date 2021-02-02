//
// Created by Yibo Guo on 11/26/20.
//

#include <time.h>
#include <stdio.h>

#include "ut_stack.h"

Stack * s;
int rdm;
char * cont;

bool ut_stack_push() {
    cont = calloc(rdm, sizeof(char));
    for (int i = 0; i < rdm; i++) {
        char * TBPush = malloc(sizeof(char));
        TBPush[0] = (char) (rand() % 0xFF);
        s->push(s, &TBPush[0]);
        cont[i] = TBPush[0];
        free(TBPush);
    } bool ret = (int) s->count(s) == rdm;
    char * contList = (char *) ((Node *) s)->cont;
    for (int i = 0; i < rdm; i++) {
        ret &= contList[i] == cont[i];
        if (!ret) printf("ipt: %d, \twrote: %d, \tat position %d\n", cont[i], contList[i], i);
    }
    return ret;
}

bool ut_stack_pop() {
    int popAmount = rdm % rand();
    bool ret = true;

    for (int i = 0; i < popAmount; i++) {
        char * poped = (char *)s->pop(s);
        if (cont[rdm - i - 1] != poped[0]) {
            ret = false;
            printf("Ipt: %d, \tpoped %d,\t at position %d\n", cont[rdm - i - 1], poped[0], rdm - i - 1);
        } free(poped);
    }
    int currStackLen = rdm - popAmount;
    ret = currStackLen == (int)s->count(s);
    cont = realloc(cont, (rdm - popAmount) * sizeof(char));
    char * contList = (char *) ((Node *) s)->cont;
    for (int i = 0; i < currStackLen; i++) {
        ret &= contList[i] == cont[i];
        if (!ret) printf("ipt: %d, \twrote: %d, \tat position %d\n", cont[i], contList[i], i);
    } rdm = currStackLen;
    return ret;
}

bool ut_stack_count() {
    int runCounts = rand() % 0xFFFF;
    for (int i = 0; i < runCounts; i++) {
        if (rand() % 1) {
            char * TBPush = malloc(sizeof(char));
            TBPush[0] = (char) (rand() % 0xFF);
            s->push(s, &TBPush[0]);
            free(TBPush);
            rdm++;
        } else {
            void * tar = s->pop(s);
            if (tar) {
                free(tar);
                rdm--;
            }
        }
    }
    return rdm == (int)s->count(s);
}

void ut_stack_main() {
    srand(time(0));
    rdm = rand() % TEST_RAND_MAX;

    s = newStack(sizeof(char), 0);
    printf("Stack push test: \t%s\n", ut_stack_push() ? "Passed" : "Failed");
    printf("Stack pop test: \t%s\n", ut_stack_pop() ? "Passed" : "Failed");
    printf("Stack count test: \t%s\n", ut_stack_count() ? "Passed" : "Failed");
    s->delete(s);
}
