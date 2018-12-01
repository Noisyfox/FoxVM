//
// Created by noisyfox on 2018/9/23.
//
// Low level memory operations
//

#ifndef FOXVM_VM_MEMORY_H
#define FOXVM_VM_MEMORY_H

#include "vm_base.h"
#include "vm_memory_base.h"
#include <stddef.h>
#include <stdint.h>

// Helper functions to align pointers
#define align_up_(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))
#define align_down_(value, alignment) ((value) & ~((alignment) - 1))

static inline JAVA_BOOLEAN is_size_aligned_up(size_t size, size_t alignment) {
    return align_up_(size, alignment) == size ? JAVA_TRUE : JAVA_FALSE;
}

static inline size_t align_size_up(size_t size, size_t alignment) {
    return align_up_(size, alignment);
}

static inline JAVA_BOOLEAN is_ptr_aligned(void *ptr, size_t alignment) {
    return align_up_((intptr_t) ptr, (intptr_t) alignment) == (intptr_t) ptr ? JAVA_TRUE : JAVA_FALSE;
}

static inline void *align_ptr(void *ptr, size_t alignment) {
    return (void *) align_up_((intptr_t) ptr, (intptr_t) alignment);
}

static inline JAVA_BOOLEAN is_size_aligned_down(size_t size, size_t alignment) {
    return align_down_(size, alignment) == size ? JAVA_TRUE : JAVA_FALSE;
}

static inline size_t align_size_down(size_t size, size_t alignment) {
    return align_down_(size, alignment);
}

#define ptr_inc(ptr, offset) ((void*)((uintptr_t)(ptr)) + (offset))
#define ptr_dec(ptr, offset) ((void*)((uintptr_t)(ptr)) - (offset))

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

extern SystemMemoryInfo g_systemMemoryInfo;

JAVA_BOOLEAN mem_init();

static inline uint32_t mem_page_size() {
    return g_systemMemoryInfo.pageSize;
}

static inline uint32_t mem_alloc_granularity() {
    return g_systemMemoryInfo.allocGranularity;
}

JAVA_BOOLEAN mem_get_status(MemoryStatus *status);

void *mem_reserve(void *addr, size_t size, size_t alignment_hint);

JAVA_BOOLEAN mem_commit(void *addr, size_t size);

JAVA_BOOLEAN mem_uncommit(void *addr, size_t size);

JAVA_BOOLEAN mem_release(void *addr, size_t size);


#endif //FOXVM_VM_MEMORY_H
