//
// Created by noisyfox on 2018/11/17.
//

#include "vm_gc.h"
#include "vm_memory.h"

int heap_init() {
    mem_init();
}

void tlab_init(VM_PARAM_CURRENT_CONTEXT, ThreadAllocContext *tlab) {
    tlab->tlabHead = 0;
    tlab->tlabCurrent = 0;
    tlab->tlabLimit = 0;
}

/**
 * Get a new TLAB buffer.
 * @param __vmCurrentThreadContext
 * @return 0 for success, 1 otherwise.
 */
int alloc_tlab(VM_PARAM_CURRENT_CONTEXT) {

}

void *heap_alloc(VM_PARAM_CURRENT_CONTEXT, size_t size) {
    if (size <= TLAB_MAX_ALLOC) {
        // Try alloc on TLAB
        ThreadAllocContext *tlab = &vmCurrentContext->tlab;

        while (1) {
            uint8_t *result = tlab->tlabCurrent;
            uint8_t *advance = result + size;

            if (advance <= tlab->tlabLimit) {
                // Alloc from TLAB by increasing the current pointer
                tlab->tlabCurrent = advance;
                return result;
            } else {
                // Need a new TLAB
                int r = alloc_tlab(vmCurrentContext);
                if (r) {
                    return NULL;
                }
            }
        }
    } else if (size > LARGE_OBJECT_SIZE) {
        // Alloc on old gen

    } else {
        // Alloc in yong gen, outside TLAB

    }
}
