//
// Created by noisyfox on 2018/10/1.
//

#include "vm_monitor.h"
#include "c11/threads.h"
#include <string.h>

typedef struct {
    mtx_t mutex;
    cnd_t condition;
    JAVA_LONG ownerThreadId;
    int count;
} VMMonitor;

JAVA_VOID monitorCreate(VMThreadContext *ctx, JAVA_OBJECT obj) {
    if (obj->monitor != NULL) {
        return;
    }

    VMMonitor *m = malloc(sizeof(VMMonitor));
    // TODO: assert m != NULL
    memset(m, 0, sizeof(VMMonitor));
    // TODO: check return value
    mtx_init(&m->mutex, mtx_timed);
    cnd_init(&m->condition);

    obj->monitor = m;
}

JAVA_VOID monitorFree(VMThreadContext *ctx, JAVA_OBJECT obj) {
    VMMonitor *m = obj->monitor;
    if (m != NULL) {
        obj->monitor = NULL;

        cnd_destroy(&m->condition);
        mtx_destroy(&m->mutex);

        free(m);
    }
}

JAVA_VOID monitorEnter(VMThreadContext *ctx, JAVA_OBJECT obj) {
    if (obj->monitor == NULL) {
        JAVA_CLASS clazz = obj->clazz;

        if (clazz == (JAVA_CLASS) JAVA_NULL) {
            // TODO: fail
            return;
        }

        // TODO: put monitorEnter & monitorExit in a try-finally block
        monitorEnter(ctx, (JAVA_OBJECT) clazz); // Class monitor is guaranteed to be inited when class is load.
        monitorCreate(ctx, obj);
        monitorExit(ctx, (JAVA_OBJECT) clazz);
    }
    // Do lock
    VMMonitor *m = obj->monitor;
    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id == m->ownerThreadId) { // re-entrance
        m->count++;
        return;
    }
    mtx_lock(&m->mutex); // TODO: handle error
    m->ownerThreadId = current_thread_id;
    m->count++;
}

JAVA_VOID monitorExit(VMThreadContext *ctx, JAVA_OBJECT obj) {
    VMMonitor *m = obj->monitor;
    if (m == NULL) {
        return; // TODO: fail
    }

    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return; // TODO: fail
    }
    m->count--;
    if (m->count > 0) {
        return;
    }
    m->count = 0;
    m->ownerThreadId = 0;
    mtx_unlock(&m->mutex); // TODO: handle error
}

static inline void timeAdd(struct timespec *ts, JAVA_LONG timeout, JAVA_INT nanos) {
    ts->tv_sec += timeout / 1000;
    ts->tv_nsec += ((long) (timeout % 1000)) * 100000 + nanos;
    while (ts->tv_nsec > 1000000000) {
        ts->tv_nsec -= 1000000000;
        ts->tv_sec++;
    }
}

JAVA_VOID monitorWait(VMThreadContext *ctx, JAVA_OBJECT obj, JAVA_LONG timeout, JAVA_INT nanos) {
    VMMonitor *m = obj->monitor;
    if (m == NULL) {
        return; // TODO: fail
    }

    // Make sure the obj lock is held by current thread
    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return; // TODO: fail
    }

    const int prev_count = m->count;

    // Release the ownership of this obj
    m->ownerThreadId = 0;
    m->count = 0;

    if (timeout == 0 && nanos == 0) {
        // Wait for unlimited time
        cnd_wait(&m->condition, &m->mutex); // TODO: handle error
    } else {
        // Wait for certain time
        struct timespec ts;
        timespec_get(&ts, TIME_UTC); // TODO: handle error
        timeAdd(&ts, timeout, nanos);
        cnd_timedwait(&m->condition, &m->mutex, &ts); // TODO: handle error
    }

    // Regain the ownership
    m->ownerThreadId = current_thread_id;
    m->count = prev_count;
}

JAVA_VOID monitorNotify(VMThreadContext *ctx, JAVA_OBJECT obj) {
    VMMonitor *m = obj->monitor;
    if (m == NULL) {
        return; // TODO: fail
    }

    // Make sure the obj lock is held by current thread
    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return; // TODO: fail
    }

    cnd_signal(&m->condition);
}

JAVA_VOID monitorNotifyAll(VMThreadContext *ctx, JAVA_OBJECT obj) {
    VMMonitor *m = obj->monitor;
    if (m == NULL) {
        return; // TODO: fail
    }

    // Make sure the obj lock is held by current thread
    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return; // TODO: fail
    }

    cnd_broadcast(&m->condition);
}
