//
// Created by noisyfox on 2018/12/2.
//

#include "vm_thread.h"
#include <string.h>

#if defined(HAVE_GET_NPROCS)

#include <sys/sysinfo.h>

#elif defined(HAVE_SC_NPROCESSORS_ONLN)

#include <unistd.h>

#elif defined(HAVE_HW_AVAILCPU)

#include <sys/sysctl.h>

#elif defined(WIN32)

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#else
#error Not supported on this platform.
#endif

SystemProcessorInfo g_systemProcessorInfo = {0};

static VMSpinLock g_threadGlobalLock = OPA_INT_T_INITIALIZER(0);

static VMThreadContext g_threadList = {0};


JAVA_BOOLEAN thread_init() {
    memset(&g_systemProcessorInfo, 0, sizeof(SystemProcessorInfo));

#if defined(HAVE_GET_NPROCS)

    int np = get_nprocs();
    if (np < 1) {
        np = 1;
    }
    g_systemProcessorInfo.numberOfProcessors = (uint32_t) np;

#elif defined(HAVE_SC_NPROCESSORS_ONLN)

    long np = sysconf(_SC_NPROCESSORS_ONLN);
    if (np < 1) {
        np = 1;
    }
    g_systemProcessorInfo.numberOfProcessors = (uint32_t) np;

#elif defined(HAVE_HW_AVAILCPU)

    int mib[4];
    int numCPU;
    size_t len = sizeof(numCPU);

    /* set the mib for hw.ncpu */
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    /* get the number of CPUs from the system */
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    if (numCPU < 1) {
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &numCPU, &len, NULL, 0);
        if (numCPU < 1) {
            numCPU = 1;
        }
    }

    g_systemProcessorInfo.numberOfProcessors = (uint32_t) numCPU;

#elif defined(WIN32)

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    g_systemProcessorInfo.numberOfProcessors = si.dwNumberOfProcessors;

#endif

    spin_lock_init(&g_threadGlobalLock);

    return JAVA_TRUE;
}

static inline JAVA_BOOLEAN thread_add(VMThreadContext *target) {
    if (target->next != NULL || g_threadList.next == target) {
        // This check doesn't cover all error cases!
        return JAVA_FALSE;
    }

    target->next = g_threadList.next;
    g_threadList.next = target;

    return JAVA_TRUE;
}

static inline JAVA_BOOLEAN thread_remove(VMThreadContext *target) {
    VMThreadContext *cursor = &g_threadList;

    while (cursor->next != NULL) {
        if (cursor->next == target) {
            cursor->next = target->next;
            target->next = NULL;
            return JAVA_TRUE;
        }

        cursor = cursor->next;
    }

    return JAVA_FALSE;
}

JAVA_BOOLEAN thread_managed_add(VM_PARAM_CURRENT_CONTEXT) {
    spin_lock_enter(vmCurrentContext, &g_threadGlobalLock);
    JAVA_BOOLEAN result = thread_add(vmCurrentContext);
    spin_lock_exit(&g_threadGlobalLock);

    return result;
}

JAVA_BOOLEAN thread_managed_remove(VM_PARAM_CURRENT_CONTEXT) {
    spin_lock_enter(vmCurrentContext, &g_threadGlobalLock);
    JAVA_BOOLEAN result = thread_remove(vmCurrentContext);
    spin_lock_exit(&g_threadGlobalLock);

    return result;
}

static inline VMThreadContext *next_thread(VMThreadContext *cursor) {
    if (cursor == NULL) {
        return g_threadList.next;
    }

    return cursor->next;
}

JAVA_VOID thread_stop_the_world(VM_PARAM_CURRENT_CONTEXT) {
    // Acquire the global thread lock.  We will hold this until the we resume the world.
    spin_lock_enter(vmCurrentContext, &g_threadGlobalLock);

    VMThreadContext *thread = NULL;
    // Ask all threads to suspend.
    while ((thread = next_thread(thread)) != NULL) {
        if (thread == vmCurrentContext) {
            continue;
        }

        thread_suspend_single(vmCurrentContext, thread);
    }
    // Wait until all thread to be stopped in safe region.
    while ((thread = next_thread(thread)) != NULL) {
        if (thread == vmCurrentContext) {
            continue;
        }

        thread_wait_until_checkpoint(vmCurrentContext, thread);
    }
}

JAVA_VOID thread_resume_the_world(VM_PARAM_CURRENT_CONTEXT) {
    VMThreadContext *thread = NULL;
    // Make a pass through all threads.
    while ((thread = next_thread(thread)) != NULL) {
        if (thread == vmCurrentContext) {
            continue;
        }

        thread_resume_single(vmCurrentContext, thread);
    }

    // Release the thread lock
    spin_lock_exit(&g_threadGlobalLock);
}
