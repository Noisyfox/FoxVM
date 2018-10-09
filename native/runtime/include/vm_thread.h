//
// Created by noisyfox on 2018/10/5.
//

#ifndef FOXVM_VM_THREAD_H
#define FOXVM_VM_THREAD_H

#include "vm_base.h"

typedef struct _VMThreadContext VMThreadContext;

typedef JAVA_VOID (*VMThreadCallback)(VMThreadContext *);

struct _VMThreadContext {
    VMThreadCallback entrance;
    VMThreadCallback terminated;

    JAVA_LONG threadId;
    JAVA_OBJECT currentThread;

//    VMStackSlot* stack;
//    int sta

    void *nativeContext;

    JAVA_OBJECT exception;
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
    thrd_stat_new = 0,          // A thread that has not yet started is in this state.
    thrd_stat_runnable,         // A thread executing in the Java virtual machine is in this state.
    thrd_stat_blocked,          // A thread that is blocked waiting for a monitor lock is in this state.
    thrd_stat_waiting,          // A thread that is waiting indefinitely for another thread to perform a particular action is in this state.
    thrd_stat_timed_waiting,    // A thread that is waiting for another thread to perform an action for up to a specified waiting time is in this state.
    thrd_stat_terminated,       // A thread that has exited is in this state.
} VMThreadState;


int thread_init(VMThreadContext *ctx);

JAVA_VOID thread_free(VMThreadContext *ctx);

// TODO: add thread parameters such as priority.
int thread_start(VMThreadContext *ctx);

int thread_sleep(VMThreadContext *ctx, JAVA_LONG timeout, JAVA_INT nanos);

JAVA_VOID thread_interrupt(VMThreadContext *current, VMThreadContext *target);

// join() is implemented in pure Java code
//int thread_join(VMThreadContext *current, VMThreadContext *target, JAVA_LONG timeout, JAVA_INT nanos);

VMThreadState thread_get_state(VMThreadContext *ctx);

// For GC
/**
 * Require the target Thread to pause at check point.
 */
JAVA_VOID thread_stop_the_world(VMThreadContext *current, VMThreadContext *target);

/**
 * Wait until target thread runs into check point and paused.
 */
int thread_wait_until_checkpoint(VMThreadContext *current, VMThreadContext *target);

/**
 * Tell the target thread that it's safe to leave the check point and keep running.
 */
JAVA_VOID thread_resume_the_world(VMThreadContext *current, VMThreadContext *target);

/**
 * Mark current thread is in a check point, so GC thread can start marking this thread.
 */
JAVA_VOID thread_enter_checkpoint(VMThreadContext *ctx);

/**
 * Mark the current thread is gonna leave the check point. If the GC thread is currently
 * working on this thread (by calling thread_stop_the_world() on this thread without calling
 * thread_resume_the_world()), then this method will block until GC thread finishes its work.
 */
int thread_leave_checkpoint(VMThreadContext *ctx);


int monitor_create(VMThreadContext *ctx, JAVA_OBJECT obj);

JAVA_VOID monitor_free(VMThreadContext *ctx, JAVA_OBJECT obj);

int monitor_enter(VMThreadContext *ctx, JAVA_OBJECT obj);

int monitor_exit(VMThreadContext *ctx, JAVA_OBJECT obj);

int monitor_wait(VMThreadContext *ctx, JAVA_OBJECT obj, JAVA_LONG timeout, JAVA_INT nanos);

int monitor_notify(VMThreadContext *ctx, JAVA_OBJECT obj);

int monitor_notify_all(VMThreadContext *ctx, JAVA_OBJECT obj);

#endif //FOXVM_VM_THREAD_H