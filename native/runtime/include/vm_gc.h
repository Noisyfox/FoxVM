//
// Created by noisyfox on 2020/4/13.
//

#ifndef FOXVM_VM_GC_H
#define FOXVM_VM_GC_H

#include "vm_base.h"

typedef struct {

} HeapConfig;

/**
 * Initialize global application heap.
 * @return 0 if success, 1 otherwise.
 */
int heap_init(HeapConfig *config);

/**
 * Allocates an object with the given size.
 *
 * @param __vmCurrentThreadContext
 * @param size
 * @return NULL if out of memory, a pointer to a piece of zeroed memory otherwise.
 */
void *heap_alloc(VM_PARAM_CURRENT_CONTEXT, size_t size);

/**
 * Force allocating in large object heap.
 *
 * @param __vmCurrentThreadContext
 * @param size
 * @return  NULL if out of memory, a pointer to a piece of zeroed memory otherwise.
 */
void *heap_alloc_loh(VM_PARAM_CURRENT_CONTEXT, size_t size);

void *heap_alloc_uncollectable(size_t size);
void heap_free_uncollectable(void* ptr);

#endif //FOXVM_VM_GC_H
