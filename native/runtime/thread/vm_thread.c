//
// Created by noisyfox on 2018/12/2.
//

#include "vm_thread.h"
#include <string.h>
#include "classloader/vm_boot_classloader.h"
#include "vm_bytecode.h"
#include "vm_method.h"
#include "vm_field.h"
#include "vm_string.h"

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

static MethodInfo *java_lang_Thread_init = NULL;
static ResolvedField *java_lang_Thread_eetop = NULL;


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

JAVA_BOOLEAN thread_init_main(VM_PARAM_CURRENT_CONTEXT) {
    // Find the Thread <init> method
    java_lang_Thread_init = method_find(g_classInfo_java_lang_Thread, "<init>", "(Ljava/lang/ThreadGroup;Ljava/lang/String;)V");
    assert(java_lang_Thread_init);

    stack_frame_start(-1, 3, 1);

    // Create a new Thread instance
    bc_new(g_classInfo_java_lang_Thread);

    // Now there should be a inited Thread object at the top of the stack
    // We store it in local 0
    bc_store(0, VM_SLOT_OBJECT);

    JAVA_OBJECT threadObj = local_of(0).data.o;

    // Since `Thread.<init>()` calls `currentThread()`, we need to set it up before init the object
    // This is what OpenJDK does in hotspot\src\share\vm\runtime\thread.cpp: `create_initial_thread()`.
    // Find the eetop field
    java_lang_Thread_eetop = field_find(threadObj->clazz, "eetop", "J");
    assert(java_lang_Thread_eetop);

    // Set the eetop to the native context
    *((JAVA_LONG *) ptr_inc(threadObj, java_lang_Thread_eetop->info.offset))
            = (JAVA_LONG) ((intptr_t) vmCurrentContext);

    // Set the priority
    ResolvedField *fieldPriority = field_find(threadObj->clazz, "priority", "I");
    assert(fieldPriority);
    *((JAVA_INT *) ptr_inc(threadObj, fieldPriority->info.offset)) = 5; // java.lang.Thread#NORM_PRIORITY

    // Set the currentThread in context
    vmCurrentContext->currentThread = threadObj;

    // Then we prepare for calling <init>
    // First we load the object from local 0 to the stack
    bc_load(0, VM_SLOT_OBJECT);

    // Then we get the static field java.lang.ThreadGroup#mMain
    JAVA_CLASS cThreadGroup;
    bc_resolve_class(vmCurrentContext, &STACK_FRAME, g_classInfo_java_lang_ThreadGroup, &cThreadGroup);
    ResolvedField *fieldThreadGroupMain = field_find(cThreadGroup, "mMain", "Ljava/lang/ThreadGroup;");
    assert(fieldThreadGroupMain);
    JAVA_OBJECT mainGroup = *((JAVA_OBJECT*)ptr_inc(cThreadGroup, fieldThreadGroupMain->info.offset));
    assert(mainGroup);
    stack_push_object(mainGroup);

    // Then push the thread name
    JAVA_OBJECT strMain = string_create_utf8(vmCurrentContext, "main");
    assert(strMain);
    stack_push_object(strMain);

    // Call init
    bc_invoke_special(java_lang_Thread_init->code);

    stack_frame_end();

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

VMThreadContext *thread_managed_next(VMThreadContext *cursor) {
    if (cursor == NULL) {
        return g_threadList.next;
    }

    return cursor->next;
}

JAVA_VOID thread_stop_the_world(VM_PARAM_CURRENT_CONTEXT) {
    // Acquire the global thread lock.  We will hold this until the we resume the world.
    spin_lock_enter(vmCurrentContext, &g_threadGlobalLock);

    // Ask all threads to suspend.
    thread_iterate(thread) {
        if (thread == vmCurrentContext) {
            continue;
        }

        thread_suspend_single(vmCurrentContext, thread);
    }
}

JAVA_VOID thread_wait_until_world_stopped(VM_PARAM_CURRENT_CONTEXT) {
    // Wait until all thread to be stopped in safe region.
    thread_iterate(thread) {
        if (thread == vmCurrentContext) {
            continue;
        }

        thread_wait_until_saferegion(vmCurrentContext, thread);
    }
}

JAVA_VOID thread_resume_the_world(VM_PARAM_CURRENT_CONTEXT) {
    // Make a pass through all threads.
    thread_iterate(thread) {
        if (thread == vmCurrentContext) {
            continue;
        }

        thread_resume_single(vmCurrentContext, thread);
    }

    // Release the thread lock
    spin_lock_exit(&g_threadGlobalLock);
}
