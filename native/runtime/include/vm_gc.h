//
// Created by noisyfox on 2018/11/17.
//
// High level memory allocation and garbage collection
//

#ifndef FOXVM_VM_GC_H
#define FOXVM_VM_GC_H

#include "vm_thread.h"

typedef struct {
    size_t maxSize;
    uint32_t newRatio;
    uint32_t survivorRatio;
} HeapConfig;

// Large objects go directly to old gen
#define LARGE_OBJECT_SIZE_MIN ((size_t)(85000))

// Min size of each chunk in old gen
#define OLD_GEN_CHUNK_SIZE_MIN ((size_t)4*1024)

// The data after the header is unused yet
#define HEAP_FLAG_EMPTY ((uint32_t) 0x0)

// The data after the header is a normal object
#define HEAP_FLAG_NORMAL ((uint32_t) 0x1)

// The original object after the header has been moved to another place and
// the data now is a forward pointer structure
#define HEAP_FLAG_FORWARD ((uint32_t) 0x2)

/**
 * Heap object header (small structure that immediately precedes every object in the GC heap). Only
 * the GC uses this so far, and only to store a couple of bits of information.
 */
typedef struct {
    // TODO: add padding if target x64
    uint32_t flag;
} ObjectHeapHeader;

typedef struct {
    void *newAddress;
    size_t remainingSize; // Size of the original object - sizeof(ForwardPointer)
} ForwardPointer;

/**
 * Initialize global application heap.
 * @return 0 if success, 1 otherwise.
 */
int heap_init(VM_PARAM_CURRENT_CONTEXT, HeapConfig *config);

int gc_thread_start(VM_PARAM_CURRENT_CONTEXT);

int gc_thread_shutdown(VM_PARAM_CURRENT_CONTEXT);

void tlab_init(VM_PARAM_CURRENT_CONTEXT, ThreadAllocContext *tlab);

/**
 * @param __vmCurrentThreadContext
 * @param size
 * @return NULL if out of memory, a pointer otherwise.
 */
void *heap_alloc(VM_PARAM_CURRENT_CONTEXT, size_t size);

#endif //FOXVM_VM_GC_H
