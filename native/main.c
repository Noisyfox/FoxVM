#include <stdio.h>
#include "vm_main.h"
#include "_0P8CTestMain.h"

JAVA_VOID test_main(VM_PARAM_CURRENT_CONTEXT) {
    stack_frame_start(NULL, 1, 1);
    bc_prepare_arguments(1);

    bc_aconst_null();
    bc_invoke_static(&classInfo0P8CTestMain, method_0P8CTestMain_4Mmain_A1_9Pjava_lang6CString);

    stack_frame_end();
}

int main(int argc, char *argv[]) {
    return vm_main(argc, argv, test_main);
}