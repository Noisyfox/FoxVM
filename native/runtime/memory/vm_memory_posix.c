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

void *mem_reserve(void *addr, size_t size, size_t alignment_hint) {
    // Make sure start addr is aligned
    if (is_ptr_aligned(addr, alignment_hint) != JAVA_FALSE) {
        // TODO: log unaligned address
        return NULL;
    }
    if (((uintptr_t) addr) % mem_page_size() != 0) {
        // TODO: log unaligned address
        return NULL;
    }

    int flags = MAP_PRIVATE | MAP_NORESERVE | MAP_ANONYMOUS;
    if (addr) {
        flags |= MAP_FIXED;
    }

    size_t required_size = size;
    if (addr == NULL) {
        required_size = size + alignment_hint;
    }

    // PROT_NONE: prevent touching an uncommitted page
    void *target_addr = mmap(addr, required_size, PROT_NONE, flags, -1, 0);
    if (target_addr == MAP_FAILED) {
        return NULL;
    } else {
        if (addr) {
            if (addr == target_addr) {
                return target_addr;
            } else {
                munmap(target_addr, required_size);
                return NULL;
            }
        } else {
            // Free extra padding region
            void *aligned_addr = align_ptr(target_addr, alignment_hint);
            void *aligned_end = aligned_addr + size;
            void *target_end = target_addr + required_size;
            if (aligned_addr > target_addr) {
                munmap(target_addr, aligned_addr - target_addr);
            }
            if (aligned_end < target_end) {
                munmap(aligned_end, target_end - aligned_end);
            }

            return aligned_addr;
        }
    }
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
