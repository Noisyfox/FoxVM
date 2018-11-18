//
// Created by noisyfox on 2018/11/17.
//

#ifndef FOXVM_VM_MEMORY_STRUCT_H
#define FOXVM_VM_MEMORY_STRUCT_H

#include <stdint.h>

// The size of each TLAB, 8k
#define TLAB_SIZE  ((size_t)(8*1024))

// Object larger than half of TLAB_SIZE will alloc out of TLAB
#define TLAB_MAX_ALLOC (TLAB_SIZE / 2)

// Large objects go directly to old gen
#define LARGE_OBJECT_SIZE ((size_t)(85000))

typedef struct _AllocContext ThreadAllocContext;

struct _AllocContext {
    uint8_t *tlabHead; // beginning of this TLAB
    uint8_t *tlabCurrent;  // current allocation position of TLAB
    uint8_t *tlabLimit; // The end of TLAB
};

#endif //FOXVM_VM_MEMORY_STRUCT_H
