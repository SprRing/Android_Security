
#ifndef CODE_APK_DECOMPILER_H
#define CODE_APK_DECOMPILER_H

#include <stdbool.h>

#include "../util/util.h"

typedef struct apkDecompiler AD;

struct apkDecompiler {
    String dex_loc, jd_loc, apk_loc, decom_tar;

    AD * (* init) (AD *, String dex2jar, String jd_cli, String apk);
    AD * (* setDecomTar) (AD *, String target);
    bool (* validate) (AD *);
    AD * (* exe) (AD *);
    void (* delete) (AD *);
};

AD * newAD(void);

int sampleRunAPKDecom(void);

#endif //CODE_APK_DECOMPILER_H
