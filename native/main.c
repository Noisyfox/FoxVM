#include <stdio.h>
#include "vm_main.h"


JAVA_VOID test_main(VM_PARAM_CURRENT_CONTEXT) {
    stack_frame_start(1, 1);
    local_transfer_arguments(vmCurrentContext, 1);

    bc_aconst_null();
    bc_astore(0);
    bc_aload(0);
    bc_astore(0);
    bc_bipush(15);
    bc_istore(0);
    bc_sipush(-20);

    bc_line(0);

    printf("Hello, World!\n");

    int count = 0;
    while(heap_alloc(vmCurrentContext, /*85000 - 100*/ 20)) {
        count ++;
    }
    printf("Total alloc count: %d\n", count);

    thread_sleep(vmCurrentContext, 5000, 0);

    stack_frame_end();
}

int main(int argc, char *argv[]) {
    return vm_main(argc, argv, test_main);
}