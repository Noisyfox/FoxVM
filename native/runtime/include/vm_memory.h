//
// Created by noisyfox on 2018/9/23.
//
// Low level memory operations
//

#ifndef FOXVM_VM_MEMORY_H
#define FOXVM_VM_MEMORY_H

#include "vm_base.h"

void test();

#if defined(HAVE_POSIX_MEMALIGN)

#include <stdlib.h>

static inline void *mem_aligned_malloc(size_t size, size_t alignment) {
    void *ptr;
    int ret = posix_memalign(&ptr, alignment, size);

    if (!ret) {
        return ptr;
    }

    return NULL;
}

#define mem_aligned_free(ptr) free(ptr)

#elif defined(HAVE_ALIGNED_MALLOC)

#include <malloc.h>

#define mem_aligned_malloc(size, align) _aligned_malloc(size, align)
#define mem_aligned_free(ptr) _aligned_free(ptr)

#else
#error no aligned malloc implementation specified
#endif

#endif //FOXVM_VM_MEMORY_H
