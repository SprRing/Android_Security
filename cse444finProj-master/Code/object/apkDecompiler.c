
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <err.h>
#include <time.h>

#include "apkDecompiler.h"

/***
 * Object AD initializer
 * @param a The object to be operate on
 * @param dex DEX2JAR execution address
 * @param jd_cli jd_cli execution address
 * @param apk Address of the APK to be decompiled
 * @return The AD object which passed in been initialized.
 */

static AD * ad_init (AD * a, String dex, String jd_cli, String apk) {
    if (!a || !dex || ! jd_cli || !apk) return 0;
    a->dex_loc = calloc(strlen(dex) + 1, sizeof(char));
    a->jd_loc = calloc(strlen(jd_cli) + 1, sizeof(char));
    a->apk_loc = calloc(strlen(apk) + 1, sizeof(char));

    strcpy(a->dex_loc, dex);
    strcpy(a->jd_loc, jd_cli);
    strcpy(a->apk_loc, apk);

    return a;
}

/***
 * Object AD validator
 * @param a The AD object to be validated
 * @return If such object initialized and valid
 */

static bool ad_validate (AD * a) {
    if (!a || !a->jd_loc || !a->dex_loc || !a->apk_loc) return false;
    return access(a->dex_loc, F_OK) != -1
           && access(a->jd_loc, F_OK) != -1
           && access(a->apk_loc, F_OK) != -1;
}

/***
 * The decompiler executor powered by Shell Script
 * @param a The decompiler object to be executed
 * @return The object which passed in
 */

static AD * ad_exe (AD * a) {
    if (!a || !a->validate(a)) {
        warnx("AD::exe(): Invalid object");
        return a;
    }
    bool cusOutput = a->decom_tar;

    // Step 1, decompile apk into jar
    char intermediateJarName[64];
    sprintf(intermediateJarName, "apkdcom%lu.jar", (unsigned long) time(0));
    printf("%s\n", intermediateJarName);
    char jarAdd[6 + strlen(intermediateJarName)];
    sprintf(jarAdd, "/tmp/%s", intermediateJarName);
    u_int32_t dexCmdLen = strlen(a->dex_loc) + strlen(a->apk_loc) + strlen(jarAdd) + strlen("  -o ") + strlen(" 2>&1") + 1;
    String dexCmd = calloc(dexCmdLen, sizeof(char));
    sprintf(dexCmd, "%s %s -o %s %s", a->dex_loc, a->apk_loc, jarAdd, "2>&1");
    printf("Issuing cmd (%u:%lu): %s\n", dexCmdLen, strlen(dexCmd), dexCmd);
    if (system(dexCmd) == -1) warn("Shell fault");
    free(dexCmd);

    // Step 2, decompile jar into java source
    u_int32_t cusLen = cusOutput ? strlen(" -od ") + strlen(a->decom_tar) : 0;
    u_int32_t baseLen = strlen("java -jar  ") + strlen(jarAdd) + strlen(a->jd_loc) + strlen(" 2>&1") + 1;
    u_int32_t cmdLen = cusLen + baseLen;
    String jdCmd = calloc(cmdLen, sizeof(char));
    sprintf(jdCmd, "java -jar %s%s%s %s %s", a->jd_loc, cusOutput ? " -od " : "",  cusOutput ? a->decom_tar : "", jarAdd, "2>&1");
    printf("Issuing cmd (%u:%lu): %s\n", cmdLen, strlen(jdCmd), jdCmd);
    if (system(jdCmd) == -1) warn("Shell fault");
    free(jdCmd);

    // Step 3, clean up temp file
    if (!(access(jarAdd, F_OK) != -1 && !remove(jarAdd) && access(jarAdd, F_OK) == -1))
        warnx("Failed to clean up temporary file");

    return a;
}

/***
 * Object destructor
 * @param a The decompiler object to be destructed
 */

static void ad_delete (AD * a) {
    if (!a) return;
    free(a->apk_loc);
    free(a->dex_loc);
    free(a->jd_loc);
    if (a->decom_tar) free(a->decom_tar);

    memset(a, 0, sizeof(AD));
    free(a);
    a = 0;
}

/***
 * Set customized decompilation target output directory
 * @param a The decompiler object
 * @param tar The target address
 * @return The object, or 0 if invalid parameter passed
 */

static AD * ad_setDecmpTar(AD * a, String tar) {
    if (!a || !tar) return 0;
    a->decom_tar = calloc(strlen(tar) + 1, sizeof(char));
    strcpy(a->decom_tar, tar);
    return a;
}

/***
 * Decompiler object constructor
 * @return The newly constructed AD object
 */

AD * newAD(void) {
    AD * ret = calloc(1, sizeof(AD));
    memset(ret, 0, sizeof(AD));

    ret->init = ad_init;
    ret->setDecomTar = ad_setDecmpTar;
    ret->validate = ad_validate;
    ret->exe = ad_exe;
    ret->delete = ad_delete;

    return ret;
}

/***
 * The instructional function shows how to make the object works as expected
 * @return Always return 0
 */

int sampleRunAPKDecom (void) {
    // Call constructor to build a new object
    AD * a = newAD();

    // Initial the decompiler with the script address and the subject apk address
    a = a->init(a, "../pkg/apk_decompiler/dex2jar-2.0/d2j-dex2jar.sh", "../pkg/apk_decompiler/jd_cli/jd-cli.jar", "../pkg/apk_decompiler/sample_apk/taobao.apk");

    // Set a customuzed output address if needed, or, by default, will decompile to the working directory
    // Call a.exe() to execute the decompile instruction
    a = a->setDecomTar(a, "~/Downloads/output")->exe(a);

    // Call object destructor to destroy the object and free memory
    a->delete(a);

    /*
     * As a shorter version, you can write:
     * AD * a = newAD();
     * a = a->init(a, "", "", "")->setDecomTar(a, "")->exe(a);
     * a->delete(a);
     *
     * AD::setDecomTar() is called on need base
     */

    return 0;
}
