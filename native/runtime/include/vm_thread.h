//
// Created by noisyfox on 2018/10/5.
//

#ifndef FOXVM_VM_THREAD_H
#define FOXVM_VM_THREAD_H

#include "vm_base.h"
#include "vm_tlab.h"

typedef struct {
    uint32_t numberOfProcessors;
} SystemProcessorInfo;

extern SystemProcessorInfo g_systemProcessorInfo;

/** Init the thread system. */
JAVA_BOOLEAN thread_init();

typedef JAVA_VOID (*VMThreadCallback)(VM_PARAM_CURRENT_CONTEXT);

struct _VMThreadContext {
    VMThreadContext *next;

    VMThreadCallback entrance;
    VMThreadCallback terminated;

    JAVA_LONG threadId;
    JAVA_OBJECT currentThread;  // A java Thread object of this thread

//    VMStackSlot* stack;
//    int sta

    void *nativeContext;  // Platform specific thread context

    JAVA_OBJECT exception;  // Current exception object

    // Thread-local allocation buffer. Small objects will be allocated directly on it so no lock is needed.
    ThreadAllocContext tlab;
};

enum {
    thrd_success = 0,   // succeeded
    thrd_timeout,       // timeout
    thrd_error,         // failed
    thrd_interrupt,     // been interrupted
    thrd_lock,          // lock not owned by current thread
    thrd_started,       // thread already started
};

typedef enum {
    thrd_stat_invalid = -1,     // The thread is not inited.
    thrd_stat_new = 0,          // A thread that has not yet started is in this state.
    thrd_stat_runnable,         // A thread executing in the Java virtual machine is in this state.
    thrd_stat_blocked,          // A thread that is blocked waiting for a monitor lock is in this state.
    thrd_stat_waiting,          // A thread that is waiting indefinitely for another thread to perform a particular action is in this state.
    thrd_stat_timed_waiting,    // A thread that is waiting for another thread to perform an action for up to a specified waiting time is in this state.
    thrd_stat_terminated,       // A thread that has exited is in this state.
} VMThreadState;


int thread_native_init(VM_PARAM_CURRENT_CONTEXT);

JAVA_VOID thread_native_free(VM_PARAM_CURRENT_CONTEXT);

JAVA_LONG thread_get_native_id(VM_PARAM_CURRENT_CONTEXT);

/**
 * Attach the given context to current thread.
 *
 * The given context must be inited by calling `thread_native_init` first.
 */
JAVA_BOOLEAN thread_native_attach_main(VM_PARAM_CURRENT_CONTEXT);

JAVA_BOOLEAN thread_managed_add(VM_PARAM_CURRENT_CONTEXT);

JAVA_BOOLEAN thread_managed_remove(VM_PARAM_CURRENT_CONTEXT);

VMThreadContext *thread_managed_next(VMThreadContext *cursor);

// TODO: add thread parameters such as priority.
int thread_start(VM_PARAM_CURRENT_CONTEXT);

int thread_sleep(VM_PARAM_CURRENT_CONTEXT, JAVA_LONG timeout, JAVA_INT nanos);

JAVA_VOID thread_interrupt(VM_PARAM_CURRENT_CONTEXT, VMThreadContext *target);

// join() is implemented in pure Java code
//int thread_join(VM_PARAM_CURRENT_CONTEXT, VMThreadContext *target, JAVA_LONG timeout, JAVA_INT nanos);

VMThreadState thread_get_state(VM_PARAM_CURRENT_CONTEXT);

// For GC
/**
 * Ask all thread to stop in safe region except the caller thread.
 */
JAVA_VOID thread_stop_the_world(VM_PARAM_CURRENT_CONTEXT);

/**
 * Wait until all threads are suspended in the safe region.
 */
JAVA_VOID thread_wait_until_world_stopped(VM_PARAM_CURRENT_CONTEXT);

/**
 * Resume all suspended threads.
 */
JAVA_VOID thread_resume_the_world(VM_PARAM_CURRENT_CONTEXT);

/**
 * Require the target Thread to pause at check point.
 */
JAVA_VOID thread_suspend_single(VM_PARAM_CURRENT_CONTEXT, VMThreadContext *target);

/**
 * Wait until target thread runs into a safe region.
 */
JAVA_VOID thread_wait_until_saferegion(VM_PARAM_CURRENT_CONTEXT, VMThreadContext *target);

/**
 * Tell the target thread that it's safe to leave the check point and keep running.
 */
JAVA_VOID thread_resume_single(VM_PARAM_CURRENT_CONTEXT, VMThreadContext *target);

/**
 * Mark current thread is in a safe region, so GC thread can start marking this thread.
 * A safe region is a piece of code that does not do any heap allocation and also
 * won't mutate any pointers.
 *
 * This function is reentrant so you can call this even if it's already in a safe region.
 * There is a counter tracking the reentrance.
 */
JAVA_VOID thread_enter_saferegion(VM_PARAM_CURRENT_CONTEXT);

/**
 * Try leaving the safe region.
 *
 * This function will first decrease the safe region reentrance counter. If the counter
 * is still larger than 0, then the thread is still in safe region and this function will
 * return.
 *
 * Otherwise when the counter reaches 0, it will first check if the GC thread is currently
 * working on this thread (by calling thread_suspend_single() on this thread without calling
 * thread_resume_single()). If so, this method will block until GC thread finishes its work.
 * Then this function marks the thread as not in safe region.
 */
JAVA_VOID thread_leave_saferegion(VM_PARAM_CURRENT_CONTEXT);

/**
 * Check if gc is trying to suspend the thread. If stopTheWorld==true then this function will
 * block until gc is finished.
 *
 * If this function is called outside the safe region, then it's roughly equivalent to call
 * thread_leave_saferegion() immediately after thread_enter_saferegion().
 *
 * If this function is called inside the safe region, then it will block if stopTheWorld==true
 * even though the counter is larger than 0, which is different from thread_enter_saferegion().
 *
 * @return JAVA_TRUE if it's been blocked by GC, JAVA_FALSE otherwise.
 */
JAVA_BOOLEAN thread_checkpoint(VM_PARAM_CURRENT_CONTEXT);

// For object lock

int monitor_create(VM_PARAM_CURRENT_CONTEXT, VMStackSlot *obj);

JAVA_VOID monitor_free(VM_PARAM_CURRENT_CONTEXT, VMStackSlot *obj);

int monitor_enter(VM_PARAM_CURRENT_CONTEXT, VMStackSlot *obj);

int monitor_exit(VM_PARAM_CURRENT_CONTEXT, VMStackSlot *obj);

int monitor_wait(VM_PARAM_CURRENT_CONTEXT, VMStackSlot *obj, JAVA_LONG timeout, JAVA_INT nanos);

int monitor_notify(VM_PARAM_CURRENT_CONTEXT, VMStackSlot *obj);

int monitor_notify_all(VM_PARAM_CURRENT_CONTEXT, VMStackSlot *obj);

/**
 * Naive & simple Spin Lock based on CAS.
 * Non-reentrant!
 */
#define VM_SPIN_LOCK_FREE 0
#define VM_SPIN_LOCK_HELD 1

typedef OPA_int_t VMSpinLock;

static inline void spin_lock_init(VMSpinLock *lock) {
    OPA_store_int(lock, VM_SPIN_LOCK_FREE);
}

static inline JAVA_BOOLEAN spin_lock_try_enter(VMSpinLock *lock) {
    return OPA_cas_int(lock, VM_SPIN_LOCK_FREE, VM_SPIN_LOCK_HELD) == VM_SPIN_LOCK_FREE ? JAVA_TRUE : JAVA_FALSE;
}

static inline void spin_lock_exit(VMSpinLock *lock) {
    OPA_store_int(lock, VM_SPIN_LOCK_FREE);
}

static inline JAVA_BOOLEAN spin_lock_is_locked(VMSpinLock *lock) {
    return OPA_load_int(lock) == VM_SPIN_LOCK_FREE ? JAVA_FALSE : JAVA_TRUE;
}

static inline void spin_lock_enter(VM_PARAM_CURRENT_CONTEXT, VMSpinLock *lock) {
    thread_enter_saferegion(vmCurrentContext);

    retry:

    if (spin_lock_try_enter(lock) != JAVA_TRUE) {
        unsigned int retry_count = 0;
        while (spin_lock_is_locked(lock)) {
            if (++retry_count & 7) {
                int spin_count = 1024 * g_systemProcessorInfo.numberOfProcessors;
                for (int i = 0; i < spin_count; i++) {
                    if (spin_lock_is_locked(lock) == JAVA_FALSE) {
                        break;
                    }
                    OPA_busy_wait();
                }
            } else {
                // If we're waiting for gc to finish, we should block immediately
                if (thread_checkpoint(vmCurrentContext) != JAVA_TRUE) {
                    // Not blocked, then wait for a longer time (5ms)
                    // TODO: handle error
                    thread_sleep(vmCurrentContext, 5, 0);
                }
            }
        }
        goto retry;
    }

    thread_leave_saferegion(vmCurrentContext);
}

///**
// * Get the lock without enter safe region
// */
//static inline void spin_lock_enter_unsafe(VMSpinLock *lock) {
//    retry:
//
//    if (spin_lock_try_enter(lock) != JAVA_TRUE) {
//        unsigned int retry_count = 0;
//        while (spin_lock_is_locked(lock)) {
//            if (++retry_count & 7) {
//                int spin_count = 1024 * g_systemProcessorInfo.numberOfProcessors;
//                for (int i = 0; i < spin_count; i++) {
//                    if (spin_lock_is_locked(lock) == JAVA_FALSE) {
//                        break;
//                    }
//                    OPA_busy_wait();
//                }
//            } else {
//                // TODO: wait for a longer time
//            }
//        }
//        goto retry;
//    }
//}
//
///**
// * Acquire a lock then release it immediately.
// *
// * Operations will be wrapped inside a whole safe region.
// */
//static inline void spin_lock_touch(VM_PARAM_CURRENT_CONTEXT, VMSpinLock *lock) {
//    thread_enter_saferegion(vmCurrentContext);
//
//    spin_lock_enter_unsafe(lock);
//    spin_lock_exit(lock);
//
//    thread_leave_saferegion(vmCurrentContext);
//}
//
///**
// * Acquire a lock then release it immediately without enter safe region.
// */
//static inline void spin_lock_touch_unsafe(VMSpinLock *lock) {
//    spin_lock_enter_unsafe(lock);
//    spin_lock_exit(lock);
//}

#endif //FOXVM_VM_THREAD_H
