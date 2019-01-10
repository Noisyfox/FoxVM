//
// Created by noisyfox on 2018/11/17.
//

#ifndef FOXVM_VM_MEMORY_STRUCT_H
#define FOXVM_VM_MEMORY_STRUCT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t pageSize;
    uint32_t allocGranularity;
} SystemMemoryInfo;

typedef struct {
    uint64_t totalPhys;
    uint64_t availPhys;
    uint64_t totalVirt;
    uint64_t availVirt;
} MemoryStatus;

// The size of each TLAB, 8k
#define TLAB_SIZE_MIN  ((size_t)(8*1024))

// Object larger than 1/4 of TLAB_SIZE will alloc out of TLAB
#define TLAB_MAX_ALLOC_RATIO 4

// Large objects go directly to old gen
#define LARGE_OBJECT_SIZE_MIN ((size_t)(85000))

// Min size of each chunk in old gen
#define OLD_GEN_CHUNK_SIZE_MIN ((size_t)4*1024)

typedef struct _AllocContext ThreadAllocContext;

struct _AllocContext {
    uint8_t *tlabHead; // beginning of this TLAB
    uint8_t *tlabCurrent;  // current allocation position of TLAB
    uint8_t *tlabLimit; // The end of TLAB
};

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

#endif //FOXVM_VM_MEMORY_STRUCT_H
