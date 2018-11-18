//
// Created by noisyfox on 2018/11/19.
//

#include "vm_memory.h"
#include <sys/mman.h>
#include <unistd.h>

static size_t _page_size = 0;

JAVA_BOOLEAN mem_init() {
    _page_size = (size_t) sysconf(_SC_PAGESIZE);

    return JAVA_TRUE;
}

size_t mem_page_size() {
    return _page_size;
}

size_t mem_alloc_granularity() {
    return _page_size;
}

void *mem_reserve(void *addr, size_t size) {
    int flags = MAP_PRIVATE | MAP_NORESERVE | MAP_ANONYMOUS;

    if (addr) {
        // Make sure start addr is aligned
        if (((uintptr_t) addr) % mem_page_size() != 0) {
            // TODO: log unaligned address
            return NULL;
        }

        flags |= MAP_FIXED;
    }

    // PROT_NONE: prevent touching an uncommitted page
    void *target_addr = mmap(addr, size, PROT_NONE, flags, -1, 0);

    return target_addr == MAP_FAILED ? NULL : target_addr;
}

JAVA_BOOLEAN mem_commit(void *addr, size_t size) {
    void *res = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);

    return res == MAP_FAILED ? JAVA_FALSE : JAVA_TRUE;
}

JAVA_BOOLEAN mem_uncommit(void *addr, size_t size) {
    void *res = mmap(addr, size, PROT_NONE, MAP_PRIVATE | MAP_FIXED | MAP_NORESERVE | MAP_ANONYMOUS, -1, 0);

    return res == MAP_FAILED ? JAVA_FALSE : JAVA_TRUE;
}

JAVA_BOOLEAN mem_release(void *addr, size_t size) {
    return munmap(addr, size) == 0 ? JAVA_TRUE : JAVA_FALSE;
}
