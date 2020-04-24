#include <stdio.h>
#include "vm_main.h"


JAVA_VOID test_main(VM_PARAM_CURRENT_CONTEXT, JAVA_CLASS clazz, void *args) {
    STACK_FRAME_START(0, 0);

    printf("Hello, World!\n");

    int count = 0;
    while(heap_alloc(vmCurrentContext, /*85000 - 100*/ 20)) {
        count ++;
    }
    printf("Total alloc count: %d\n", count);

    thread_sleep(vmCurrentContext, 5000, 0);

    STACK_FRAME_END();
}

int main(int argc, char *argv[]) {
    return vm_main(argc, argv, test_main);
}