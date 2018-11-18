//
// Created by noisyfox on 2018/9/23.
//
// Low level memory operations
//

#ifndef FOXVM_VM_MEMORY_H
#define FOXVM_VM_MEMORY_H

#include "vm_base.h"

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

JAVA_BOOLEAN mem_init();

size_t mem_page_size();

size_t mem_alloc_granularity();

void *mem_reserve(void *addr, size_t size);

JAVA_BOOLEAN mem_commit(void *addr, size_t size);

JAVA_BOOLEAN mem_uncommit(void *addr, size_t size);

JAVA_BOOLEAN mem_release(void *addr, size_t size);


#endif //FOXVM_VM_MEMORY_H
