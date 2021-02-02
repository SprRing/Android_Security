
#include <stdio.h>

#include "object/apkDecompiler.h"
#include "unit_tests/ut_stack.h"
#include "unit_tests/ut_dirScanner.h"

int main(int argc, String * argv) {
    if (argc < 5) {
        //./a.out ../pkg/apk_decompiler/taobao.apk ../pkg/apk_decompiler/dex2jar-2.0/d2j-dex2jar.sh ../pkg/apk_decompiler/jd_cli/jd-cli.jar ~/Downloads/taobao
        printf("Use ./a.out <yourAPK> <dex2jarLocation> <jd-cliLocation> <outputDir>\n");
        return 1;
    }

    printf("Hello, World!\n");

    /* This section only contains unit tests */
    // ut_stack_main();
    // ut_dirScanner_main();

    AD * a = newAD();

    // Initial the decompiler with the script address and the subject apk address
    a->init(a, argv[2], argv[3], argv[1])->setDecomTar(a, argv[4])->exe(a)->delete(a);

    DS * s = newScanner(argv[4]);
    s->scan(s)->parseFile(s)->buildInclude(s, s->dirTree)->extractFunction(s)->extractPermission(s)->pemCheck2(s);
    s->pemTrack(s, "android.permission.ACCESS_NETWORK_STATE")->pemTrack(s, "android.permission.READ_PHONE_STATE")->pemTrack(s, "android.permission.WRITE_EXTERNAL_STORAGE");

    // s->printDirTree(s);
    // s->printIncludes(s);
    // s->printCallee(s);
    // s->printPem(s);

    s->delete(s);

    return 0;
}
