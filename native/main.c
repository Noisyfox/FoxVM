#include <stdio.h>
#include "vm_main.h"


JAVA_VOID test_main(VM_PARAM_CURRENT_CONTEXT, JAVA_CLASS clazz, void *args) {
    printf("Hello, World!\n");

    thread_sleep(vmCurrentContext, 5000, 0);
}

int main(int argc, char *argv[]) {
    return vm_main(argc, argv, test_main);
}