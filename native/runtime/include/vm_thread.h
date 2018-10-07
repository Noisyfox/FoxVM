//
// Created by noisyfox on 2018/10/5.
//

#ifndef FOXVM_VM_THREAD_H
#define FOXVM_VM_THREAD_H

#include "vm_base.h"

typedef struct {
    JAVA_LONG threadId;
    JAVA_OBJECT currentThread;

//    VMStackSlot* stack;
//    int sta

    void *nativeContext;

    JAVA_OBJECT exception;
} VMThreadContext;

enum {
    thrd_success = 0,   // succeeded
    thrd_timeout,       // timeout
    thrd_error,         // failed
    thrd_interrupt,     // been interrupted
    thrd_lock           // lock not owned by current thread
};


int thread_init(VMThreadContext *ctx);

JAVA_VOID thread_free(VMThreadContext *ctx);

int thread_sleep(VMThreadContext *ctx, JAVA_LONG timeout, JAVA_INT nanos);

JAVA_VOID thread_interrupt(VMThreadContext *current, VMThreadContext *target);

int thread_join(VMThreadContext *current, VMThreadContext *target, JAVA_LONG timeout, JAVA_INT nanos);

// For GC
JAVA_VOID thread_stop_the_world(VMThreadContext *current, VMThreadContext *target);
int thread_wait_until_checkpoint(VMThreadContext *current, VMThreadContext *target);
JAVA_VOID thread_resume(VMThreadContext *current, VMThreadContext *target);
JAVA_VOID thread_enter_checkpoint(VMThreadContext *ctx);
int thread_leave_checkpoint(VMThreadContext *ctx);


int monitor_create(VMThreadContext *ctx, JAVA_OBJECT obj);

JAVA_VOID monitor_free(VMThreadContext *ctx, JAVA_OBJECT obj);

int monitor_enter(VMThreadContext *ctx, JAVA_OBJECT obj);

int monitor_exit(VMThreadContext *ctx, JAVA_OBJECT obj);

int monitor_wait(VMThreadContext *ctx, JAVA_OBJECT obj, JAVA_LONG timeout, JAVA_INT nanos);

int monitor_notify(VMThreadContext *ctx, JAVA_OBJECT obj);

int monitor_notify_all(VMThreadContext *ctx, JAVA_OBJECT obj);

#endif //FOXVM_VM_THREAD_H
