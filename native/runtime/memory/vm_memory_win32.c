//
// Created by noisyfox on 2018/11/19.
//

#include "vm_memory.h"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

JAVA_BOOLEAN mem_init() {
    memset(&g_systemMemoryInfo, 0, sizeof(g_systemMemoryInfo));

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    g_systemMemoryInfo.pageSize = si.dwPageSize;
    g_systemMemoryInfo.allocGranularity = si.dwAllocationGranularity;

    return JAVA_TRUE;
}

JAVA_BOOLEAN mem_get_status(MemoryStatus *status) {
    memset(status, 0, sizeof(MemoryStatus));

    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memStatus)) {
        status->totalPhys = memStatus.ullTotalPhys;
        status->availPhys = memStatus.ullAvailPhys;
        status->totalVirt = memStatus.ullTotalVirtual;
        status->availVirt = memStatus.ullAvailVirtual;

        return JAVA_TRUE;
    } else {
        return JAVA_FALSE;
    }
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
