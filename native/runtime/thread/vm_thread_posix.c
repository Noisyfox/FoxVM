//
// Created by noisyfox on 2018/10/5.
//

#include "vm_thread.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

typedef struct _ObjectMonitor ObjectMonitor;
typedef struct _NativeThreadContext NativeThreadContext;
typedef struct _BlockingListNode BlockingListNode;

struct _BlockingListNode {
    BlockingListNode *prev;
    BlockingListNode *next;

    NativeThreadContext *thread; // Will be NULL if it's list header
};

/**
 * NEVER run this on HEADER!
 */
static inline JAVA_VOID blocking_list_remove(BlockingListNode *node) {
    if (node->prev == NULL || node->next == NULL) {
        return;
    }

    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
}

static inline JAVA_VOID blocking_list_append(BlockingListNode *header, BlockingListNode *node) {
    // Make sure the node is free
    blocking_list_remove(node);

    // Add to the end of the list
    node->next = header;
    node->prev = header->prev;
    node->prev->next = node;
    header->prev = node;
}

static inline BlockingListNode *blocking_list_first(BlockingListNode *header) {
    BlockingListNode *next = header->next;

    return next == header ? NULL : next;
}

static inline void timeAdd(struct timespec *ts, JAVA_LONG timeout, JAVA_INT nanos) {
    ts->tv_sec += timeout / 1000;
    ts->tv_nsec += ((long) (timeout % 1000)) * 100000 + nanos;
    while (ts->tv_nsec > 1000000000) {
        ts->tv_nsec -= 1000000000;
        ts->tv_sec++;
    }
}

struct _ObjectMonitor {
    pthread_mutex_t masterMutex;
    pthread_cond_t blockingCondition;
    BlockingListNode blockingListHeader;
    volatile JAVA_LONG ownerThreadId;
    int reentranceCounter;
};

struct _NativeThreadContext {
    BlockingListNode waitingListNode;

    pthread_mutex_t masterMutex;
    pthread_cond_t blockingCondition;
    JAVA_BOOLEAN interrupted;

    pthread_t nativeThreadId;
    volatile VMThreadState threadState;

    // GC specific flags
    pthread_mutex_t gcMutex;
    pthread_cond_t gcCondition;
    JAVA_BOOLEAN stopTheWorld;
    JAVA_BOOLEAN inCheckPoint;
    JAVA_BOOLEAN waitingForResume;
};

int thread_init(VMThreadContext *ctx) {
    NativeThreadContext *nativeContext = malloc(sizeof(NativeThreadContext));
    // TODO: assert nativeContext != NULL
    memset(nativeContext, 0, sizeof(NativeThreadContext));
    nativeContext->waitingListNode.thread = nativeContext;
    nativeContext->interrupted = JAVA_FALSE;
    nativeContext->threadState = thrd_stat_new;
    nativeContext->stopTheWorld = JAVA_FALSE;
    nativeContext->inCheckPoint = JAVA_FALSE;
    nativeContext->waitingForResume = JAVA_FALSE;
    // TODO: check return value
    pthread_mutex_init(&nativeContext->masterMutex, NULL);
    pthread_cond_init(&nativeContext->blockingCondition, NULL);
    pthread_mutex_init(&nativeContext->gcMutex, NULL);
    pthread_cond_init(&nativeContext->gcCondition, NULL);

    ctx->nativeContext = nativeContext;

    return thrd_success;
}

JAVA_VOID thread_free(VMThreadContext *ctx) {
    NativeThreadContext *nativeThreadContext = ctx->nativeContext;
    if (nativeThreadContext != NULL) {
        ctx->nativeContext = NULL;

        // TODO: make sure it's not blocked

        pthread_cond_destroy(&nativeThreadContext->blockingCondition);
        pthread_mutex_destroy(&nativeThreadContext->masterMutex);

        pthread_cond_destroy(&nativeThreadContext->gcCondition);
        pthread_mutex_destroy(&nativeThreadContext->gcMutex);

        free(nativeThreadContext);
    }
}

static void thread_cleanup(void *param) {
    VMThreadContext *ctx = param;
    NativeThreadContext *nativeContext = ctx->nativeContext;

    pthread_mutex_lock(&nativeContext->masterMutex);
    {
        // Set the state to terminated
        nativeContext->threadState = thrd_stat_terminated;

        // Send signal
        pthread_cond_signal(&nativeContext->blockingCondition);

        pthread_mutex_unlock(&nativeContext->masterMutex);
    }

    // Run the termination callback
    ctx->terminated(ctx);
}

static void *thread_bootstrap_enter(void *param) {
    // Setup unexpected exit handler for clean up
    pthread_cleanup_push(thread_cleanup, param) ;
            VMThreadContext *ctx = param;
            NativeThreadContext *nativeContext = ctx->nativeContext;

            pthread_mutex_lock(&nativeContext->masterMutex);
            {
                // Update thread state
                nativeContext->threadState = thrd_stat_runnable;

                // Notify the thread bootstrap success
                pthread_cond_signal(&nativeContext->blockingCondition);

                pthread_mutex_unlock(&nativeContext->masterMutex);
            }

            // Run the main entrance
            ctx->entrance(ctx);

    pthread_cleanup_pop(1); // Clean up
    return NULL;
}

int thread_start(VMThreadContext *ctx) {
    // TODO: check if VMThreadCallback is set

    NativeThreadContext *nativeContext = ctx->nativeContext;
    int ret;
    pthread_mutex_lock(&nativeContext->masterMutex);
    {
        if (nativeContext->threadState != thrd_stat_new) {
            ret = thrd_started;
        } else {
            // Create & start native thread. Use detached thread since java thread doesn't require join()
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_create(&nativeContext->nativeThreadId, &attr, thread_bootstrap_enter,
                           ctx); // TODO: check return value
            pthread_attr_destroy(&attr);
            // Wait until the thread is running
            pthread_cond_wait(&nativeContext->blockingCondition,
                              &nativeContext->masterMutex); // TODO: check return value
            // Check if truly running
            if (nativeContext->threadState == thrd_stat_terminated) {
                // Something wrong during thread bootstrap
                ret = thrd_error;
            } else {
                // All good
                ret = thrd_success;
            }
        }
        pthread_mutex_unlock(&nativeContext->masterMutex);
    }
    return ret;
}

int thread_sleep(VMThreadContext *ctx, JAVA_LONG timeout, JAVA_INT nanos) {
    NativeThreadContext *nativeContext = ctx->nativeContext;

    int cond_ret;
    JAVA_BOOLEAN interrupt;
    pthread_mutex_lock(&nativeContext->masterMutex);
    {
        // Wait for certain time
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts); // TODO: handle error
        timeAdd(&ts, timeout, nanos);
        cond_ret = pthread_cond_timedwait(&nativeContext->blockingCondition, &nativeContext->masterMutex,
                                          &ts);

        interrupt = nativeContext->interrupted;
        nativeContext->interrupted = JAVA_FALSE;

        pthread_mutex_unlock(&nativeContext->masterMutex);
    }

    if (interrupt) {
        return thrd_interrupt;
    }

    switch (cond_ret) {
        case ETIMEDOUT:
            return thrd_success;
        case 0:
            // ????????????????
        default:
            return thrd_error;
    }
}

JAVA_VOID thread_interrupt(VMThreadContext *current, VMThreadContext *target) {
    NativeThreadContext *nativeContext = target->nativeContext;
    pthread_mutex_lock(&nativeContext->masterMutex);
    {
        // Check if the thread is alive first
        switch (nativeContext->threadState) {
            case thrd_stat_runnable:
            case thrd_stat_blocked:
            case thrd_stat_waiting:
            case thrd_stat_timed_waiting:
                nativeContext->interrupted = JAVA_TRUE;
                pthread_cond_signal(&nativeContext->blockingCondition); // Unblocking the target thread
                break;
            case thrd_stat_new:
            case thrd_stat_terminated:
            default:
                // No effect
                break;
        }

        pthread_mutex_unlock(&nativeContext->masterMutex);
    }
}

VMThreadState thread_get_state(VMThreadContext *ctx) {
    NativeThreadContext *nativeContext = ctx->nativeContext;
    VMThreadState state;
    pthread_mutex_lock(&nativeContext->masterMutex);
    {
        state = nativeContext->threadState;
        pthread_mutex_unlock(&nativeContext->masterMutex);
    }

    return state;
}

JAVA_VOID thread_stop_the_world(VMThreadContext *current, VMThreadContext *target) {
    NativeThreadContext *nativeContext = target->nativeContext;
    pthread_mutex_lock(&nativeContext->gcMutex);
    {
        nativeContext->stopTheWorld = JAVA_TRUE;
        pthread_mutex_unlock(&nativeContext->gcMutex);
    }
}

JAVA_VOID thread_wait_until_checkpoint(VMThreadContext *current, VMThreadContext *target) {
    NativeThreadContext *nativeContext = target->nativeContext;
    pthread_mutex_lock(&nativeContext->gcMutex);
    {
        while (!nativeContext->inCheckPoint) {
            pthread_cond_wait(&nativeContext->gcCondition, &nativeContext->gcMutex);
        }
        pthread_mutex_unlock(&nativeContext->gcMutex);
    }
}

JAVA_VOID thread_resume_the_world(VMThreadContext *current, VMThreadContext *target) {
    NativeThreadContext *nativeContext = target->nativeContext;
    pthread_mutex_lock(&nativeContext->gcMutex);
    {
        nativeContext->stopTheWorld = JAVA_FALSE;
        if (nativeContext->waitingForResume) {
            pthread_cond_signal(&nativeContext->gcCondition);
        }
        pthread_mutex_unlock(&nativeContext->gcMutex);
    }
}

JAVA_VOID thread_enter_checkpoint(VMThreadContext *ctx) {
    NativeThreadContext *nativeContext = ctx->nativeContext;
    pthread_mutex_lock(&nativeContext->gcMutex);
    {
        nativeContext->inCheckPoint = JAVA_TRUE;
        if (nativeContext->stopTheWorld) {
            // Notify GC thread
            pthread_cond_signal(&nativeContext->gcCondition);
        }
        pthread_mutex_unlock(&nativeContext->gcMutex);
    }
}

JAVA_VOID thread_leave_checkpoint(VMThreadContext *ctx) {
    NativeThreadContext *nativeContext = ctx->nativeContext;
    pthread_mutex_lock(&nativeContext->gcMutex);
    {
        nativeContext->waitingForResume = JAVA_TRUE;
        while (nativeContext->stopTheWorld) {
            pthread_cond_wait(&nativeContext->gcCondition, &nativeContext->gcMutex);
        }
        nativeContext->waitingForResume = JAVA_FALSE;
        nativeContext->inCheckPoint = JAVA_FALSE;

        pthread_mutex_unlock(&nativeContext->gcMutex);
    }
}


int monitor_create(VMThreadContext *ctx, JAVA_OBJECT obj) {
    if (obj->monitor != NULL) {
        return thrd_success;
    }

    ObjectMonitor *m = malloc(sizeof(ObjectMonitor));
    // TODO: assert m != NULL
    memset(m, 0, sizeof(ObjectMonitor));
    m->blockingListHeader.thread = NULL; // NULL indicates it's header node
    m->blockingListHeader.next = &m->blockingListHeader;
    m->blockingListHeader.prev = &m->blockingListHeader;
    // TODO: check return value
    pthread_mutex_init(&m->masterMutex, NULL);
    pthread_cond_init(&m->blockingCondition, NULL);

    obj->monitor = m;

    return thrd_success;
}

JAVA_VOID monitor_free(VMThreadContext *ctx, JAVA_OBJECT obj) {
    ObjectMonitor *m = obj->monitor;
    if (m != NULL) {
        obj->monitor = NULL;

        // TODO: make sure the blocking list is empty

        pthread_cond_destroy(&m->blockingCondition);
        pthread_mutex_destroy(&m->masterMutex);

        free(m);
    }
}

int monitor_enter(VMThreadContext *ctx, JAVA_OBJECT obj) {
    if (obj->monitor == NULL) {
        JAVA_CLASS clazz = obj->clazz;

        if (clazz == (JAVA_CLASS) JAVA_NULL) {
            // Class monitor should be created by class loader
            return thrd_error;
        }

        int ret = monitor_enter(ctx,
                                (JAVA_OBJECT) clazz); // Class monitor is guaranteed to be inited when class is load.
        if (ret != thrd_success) {
            return ret;
        }
        ret = monitor_create(ctx, obj);
        monitor_exit(ctx, (JAVA_OBJECT) clazz);
        if (ret != thrd_success) {
            return ret;
        }
    }

    // Do lock
    ObjectMonitor *m = obj->monitor;
    JAVA_LONG current_thread_id = ctx->threadId;
    pthread_mutex_lock(&m->masterMutex);
    {
        while (1) {
            JAVA_LONG currentOwner = m->ownerThreadId;
            if (currentOwner == 0 // free to lock
                || currentOwner == current_thread_id) { // Reentrance
                m->ownerThreadId = current_thread_id;
                m->reentranceCounter++;
                break;
            } else {
                pthread_cond_wait(&m->blockingCondition, &m->masterMutex); // Waiting for ownership to be released
            }
        }

        pthread_mutex_unlock(&m->masterMutex);
    }

    return thrd_success;
}

int monitor_exit(VMThreadContext *ctx, JAVA_OBJECT obj) {
    ObjectMonitor *m = obj->monitor;
    if (m == NULL) {
        return thrd_error;
    }

    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return thrd_lock;
    }

    pthread_mutex_lock(&m->masterMutex);
    {
        m->reentranceCounter--;

        if (m->reentranceCounter <= 0) {
            m->reentranceCounter = 0;
            m->ownerThreadId = 0; // Release ownership
            pthread_cond_signal(&m->blockingCondition); // Notify next blocking thread
        }

        pthread_mutex_unlock(&m->masterMutex);
    }

    return thrd_success;
}

int monitor_wait(VMThreadContext *ctx, JAVA_OBJECT obj, JAVA_LONG timeout, JAVA_INT nanos) {
    ObjectMonitor *m = obj->monitor;
    if (m == NULL) {
        return thrd_error;
    }

    // Make sure the obj lock is held by current thread
    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return thrd_lock;
    }

    NativeThreadContext *nativeContext = ctx->nativeContext;
    int prev_count;

    pthread_mutex_lock(&m->masterMutex);
    {
        // Attach to blocking list
        blocking_list_append(&m->blockingListHeader, &nativeContext->waitingListNode);
        prev_count = m->reentranceCounter; // First save the status first
        // Release ownership
        m->reentranceCounter = 0;
        m->ownerThreadId = 0;
        pthread_cond_signal(&m->blockingCondition); // Notify next blocking thread

        pthread_mutex_unlock(&m->masterMutex);
    }

    // Wait until timeout/notify/interrupt
    int cond_ret;
    pthread_mutex_lock(&nativeContext->masterMutex);
    {
        if (timeout == 0 && nanos == 0) {
            // Wait for unlimited time
            cond_ret = pthread_cond_wait(&nativeContext->blockingCondition, &nativeContext->masterMutex);
        } else {
            // Wait for certain time
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts); // TODO: handle error
            timeAdd(&ts, timeout, nanos);
            cond_ret = pthread_cond_timedwait(&nativeContext->blockingCondition, &nativeContext->masterMutex,
                                              &ts);
        }
        pthread_mutex_unlock(&nativeContext->masterMutex);
    }

    // Re-acquire the lock
    pthread_mutex_lock(&m->masterMutex);
    {
        // Remove from blocking list anyway
        blocking_list_remove(&nativeContext->waitingListNode);

        // Get the lock
        while (1) {
            JAVA_LONG currentOwner = m->ownerThreadId;
            if (currentOwner == 0 // free to lock
                || currentOwner == current_thread_id) { // Reentrance, shouldn't happen, but anyway
                m->ownerThreadId = current_thread_id;
                m->reentranceCounter = prev_count; // Restore the reentrance count
                break;
            } else {
                pthread_cond_wait(&m->blockingCondition, &m->masterMutex); // Waiting for ownership to be released
            }
        }

        pthread_mutex_unlock(&m->masterMutex);
    }

    // Now we are locked, check interrupt flag
    JAVA_BOOLEAN interrupt;
    pthread_mutex_lock(&nativeContext->masterMutex);
    {
        interrupt = nativeContext->interrupted;
        nativeContext->interrupted = JAVA_FALSE; // Clear the interrupt flag
        pthread_mutex_unlock(&nativeContext->masterMutex);
    }
    if (interrupt) {
        return thrd_interrupt;
    }

    // Not interrupted, so check if timeout / other error
    switch (cond_ret) {
        case 0:
            return thrd_success;
        case ETIMEDOUT:
            return thrd_timeout;
        default:
            return thrd_error;
    }
}

static inline int _monitor_notify(VMThreadContext *ctx, JAVA_OBJECT obj, JAVA_BOOLEAN all) {
    ObjectMonitor *m = obj->monitor;
    if (m == NULL) {
        return thrd_error;
    }

    // Make sure the obj lock is held by current thread
    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return thrd_lock;
    }

    pthread_mutex_lock(&m->masterMutex);
    {
        BlockingListNode *node;
        while ((node = blocking_list_first(&m->blockingListHeader)) != NULL) {
            blocking_list_remove(node);
            NativeThreadContext *t = node->thread;
            pthread_mutex_lock(&t->masterMutex);
            {
                pthread_cond_signal(&t->blockingCondition);// Wake up the thread
                pthread_mutex_unlock(&t->masterMutex);
            }
            if (!all) {
                break;
            }
        }

        pthread_mutex_unlock(&m->masterMutex);
    }

    return thrd_success;
}

int monitor_notify(VMThreadContext *ctx, JAVA_OBJECT obj) {
    return _monitor_notify(ctx, obj, JAVA_FALSE);
}

int monitor_notify_all(VMThreadContext *ctx, JAVA_OBJECT obj) {
    return _monitor_notify(ctx, obj, JAVA_TRUE);
}
