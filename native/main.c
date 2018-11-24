#include <stdio.h>
#include "vm_main.h"

int main() {
    printf("Hello, World!\n");

    HeapConfig heapConfig = {
            .maxSize=0,
            .newRatio=2,
            .survivorRatio=8
    };

    heap_init(&heapConfig);
    return 0;
}