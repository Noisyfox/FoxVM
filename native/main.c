#include <stdio.h>
#include "vm_main.h"


JAVA_VOID test_main(VM_PARAM_CURRENT_CONTEXT) {
    stack_frame_start(10, 1);
    local_transfer_arguments(vmCurrentContext, 1);

    bc_aconst_null();
    bc_astore(0);
    bc_aload(0);
    bc_astore(0);
    bc_bipush(15);
    bc_istore(0);
    bc_sipush(-20);
    bc_istore(0);
    bc_sipush(12);
    bc_iconst_4();
    bc_dup();
    bc_dup2();
    bc_iadd();
    bc_isub();
    bc_imul();
    bc_ineg();
    bc_dup();
    bc_iand();
    bc_idiv();
    bc_fconst_2();
    bc_swap();
    bc_iconst_1();
    bc_iconst_1();
    bc_ishl();
    bc_ineg();
    bc_iconst_1();
    bc_ishr();
    bc_iconst_1();
    bc_iushr();

    bc_lconst_1();
    bc_lconst_1();
    bc_lshl();
    bc_lneg();
    bc_lconst_1();
    bc_lshr();
    bc_lconst_1();
    bc_lushr();

    bc_l2i();
    bc_i2l();

    bc_nop();

    bc_iconst_1();
    bc_istore(0);
    bc_iinc(0, 2);

    // Loop 5 times
    bc_iconst_5();
L1:
    printf("Loop!\n");
    bc_iconst_1();
    bc_isub();
    bc_dup();
    bc_ifgt(L1);

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