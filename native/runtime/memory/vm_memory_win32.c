//
// Created by noisyfox on 2018/11/19.
//

#include "vm_memory.h"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

static size_t _page_size = 0;
static size_t _alloc_granularity = 0;

JAVA_BOOLEAN mem_init() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    _page_size = si.dwPageSize;
    _alloc_granularity = si.dwAllocationGranularity;

    return JAVA_TRUE;
}

size_t mem_page_size() {
    return _page_size;
}

size_t mem_alloc_granularity() {
    return _alloc_granularity;
}

void *mem_reserve(void *addr, size_t size, size_t alignment_hint) {
    if (((uintptr_t) addr) % mem_alloc_granularity() != 0) {
        // TODO: log unaligned address
        return NULL;
    }
    if (size % mem_alloc_granularity() != 0) {
        // TODO: log unaligned size
        return NULL;
    }

    // TODO: make sure addr is aligned
    void *target_addr = VirtualAlloc(addr, size, MEM_RESERVE, PAGE_READWRITE);

    if (target_addr != NULL && addr != NULL && target_addr != addr) {
        // TODO: ????
        return NULL;
    }

    return target_addr;
}

JAVA_BOOLEAN mem_commit(void *addr, size_t size) {
    if (size == 0) {
        return JAVA_TRUE;
    }

    if (((uintptr_t) addr) % mem_page_size() != 0) {
        // TODO: log unaligned address
        return JAVA_FALSE;
    }
    if (size % mem_page_size() != 0) {
        // TODO: log unaligned size
        return JAVA_FALSE;
    }

    void *res = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);

    return res == NULL ? JAVA_FALSE : JAVA_TRUE;
}

JAVA_BOOLEAN mem_uncommit(void *addr, size_t size) {
    if (size == 0) {
        return JAVA_TRUE;
    }

    if (((uintptr_t) addr) % mem_page_size() != 0) {
        // TODO: log unaligned address
        return JAVA_FALSE;
    }
    if (size % mem_page_size() != 0) {
        // TODO: log unaligned size
        return JAVA_FALSE;
    }

    return VirtualFree(addr, size, MEM_DECOMMIT) ? JAVA_TRUE : JAVA_FALSE;
}

JAVA_BOOLEAN mem_release(void *addr, size_t size) {
    return VirtualFree(addr, size, MEM_RELEASE) ? JAVA_TRUE : JAVA_FALSE;
}
