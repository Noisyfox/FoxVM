//
// Created by noisyfox on 2018/11/17.
//

#ifndef FOXVM_VM_GC_H
#define FOXVM_VM_GC_H

#include "vm_thread.h"

typedef struct {
    size_t maxSize;
    uint32_t newRatio;
    uint32_t survivorRatio;
} HeapConfig;

/**
 * Initialize global application heap.
 * @return 0 if success, 1 otherwise.
 */
int heap_init(HeapConfig *config);

void tlab_init(VM_PARAM_CURRENT_CONTEXT, ThreadAllocContext *tlab);

/**
 * @param __vmCurrentThreadContext
 * @param size
 * @return NULL if out of memory, a pointer otherwise.
 */
void *heap_alloc(VM_PARAM_CURRENT_CONTEXT, size_t size);

#endif //FOXVM_VM_GC_H
