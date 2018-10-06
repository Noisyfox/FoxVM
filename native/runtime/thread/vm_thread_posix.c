//
// Created by noisyfox on 2018/10/5.
//

#include "vm_thread.h"
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>

typedef struct {
    sem_t mutex;
    sem_t condition;
    sem_t ownerMutex;

    JAVA_LONG ownerThreadId;
    int reentranceCounter;
} VMMonitor;

enum {
    block_none = 0,
    block_lock,
    block_condition,
    block_sleep,
    block_join
};

typedef struct {
    pthread_mutex_t blockingMutex;
    int blockBy;
    VMMonitor *currentBlockingMonitor;
    JAVA_BOOLEAN interrupted;
} NativeThreadContext;


int thread_init(VMThreadContext *ctx) {
    return thrd_success;
}

JAVA_VOID thread_free(VMThreadContext *ctx) {

}

int thread_sleep(VMThreadContext *ctx, JAVA_LONG timeout, JAVA_INT nanos) {
    return thrd_success;
}

JAVA_VOID thread_interrupt(VMThreadContext *current, VMThreadContext *target) {
    NativeThreadContext *nativeThreadContext = target->nativeContext;

    pthread_mutex_lock(&nativeThreadContext->blockingMutex); // TODO: check return value
    if (!nativeThreadContext->interrupted) {
        nativeThreadContext->interrupted = JAVA_TRUE;

        VMMonitor *m = nativeThreadContext->currentBlockingMonitor;
        switch (nativeThreadContext->blockBy) {
            case block_lock:
                sem_post(&m->mutex);
                break;
            case block_condition:
                sem_post(&m->condition);
                break;
            case block_none:
            default:
                break;
        }
    }
    pthread_mutex_unlock(&nativeThreadContext->blockingMutex);
}

int thread_join(VMThreadContext *current, VMThreadContext *target, JAVA_LONG timeout, JAVA_INT nanos) {
    return thrd_success;
}

JAVA_VOID thread_stop_the_world(VMThreadContext *current, VMThreadContext *target) {

}

int thread_wait_until_checkpoint(VMThreadContext *current, VMThreadContext *target) {
    if (current == target) {
        return thrd_success;
    }
    return thrd_success;
}

JAVA_VOID thread_resume(VMThreadContext *current, VMThreadContext *target) {

}

JAVA_VOID thread_enter_checkpoint(VMThreadContext *ctx) {

}

int thread_leave_checkpoint(VMThreadContext *ctx) {
    return thrd_success;
}


int monitor_create(VMThreadContext *ctx, JAVA_OBJECT obj) {
    if (obj->monitor != NULL) {
        return thrd_success;
    }

    VMMonitor *m = malloc(sizeof(VMMonitor));
    // TODO: assert m != NULL
    memset(m, 0, sizeof(VMMonitor));
    // TODO: check return value
    sem_init(&m->mutex, 0, 1);
    sem_init(&m->condition, 0, 0);
    sem_init(&m->ownerMutex, 0, 1);

    obj->monitor = m;

    return thrd_success;
}

JAVA_VOID monitor_free(VMThreadContext *ctx, JAVA_OBJECT obj) {
    VMMonitor *m = obj->monitor;
    if (m != NULL) {
        obj->monitor = NULL;

        sem_destroy(&m->ownerMutex);
        sem_destroy(&m->condition);
        sem_destroy(&m->mutex);

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
    VMMonitor *m = obj->monitor;
    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id == m->ownerThreadId) { // re-entrance
        m->reentranceCounter++;
        return thrd_success;
    }

    // Lock with interrupt support
    NativeThreadContext *nativeThreadContext = ctx->nativeContext;
    pthread_mutex_lock(&nativeThreadContext->blockingMutex); // TODO: check return value
    nativeThreadContext->currentBlockingMonitor = m; // Register blocking monitor
    nativeThreadContext->blockBy = block_lock;
    JAVA_BOOLEAN interrupted = nativeThreadContext->interrupted;
    if (interrupted) {
        nativeThreadContext->currentBlockingMonitor = NULL;
        nativeThreadContext->blockBy = block_none;

        nativeThreadContext->interrupted = JAVA_FALSE; // Clear interrupt flag
    }
    pthread_mutex_unlock(&nativeThreadContext->blockingMutex);
    if (interrupted) {
        return thrd_interrupt;
    }

    int interrupt_retry_count = 0;
    while (1) {
        // Get the lock
        sem_wait(&m->mutex); // TODO: check return value

        // Check if we truly get the lock
        JAVA_BOOLEAN owned = JAVA_FALSE;
        sem_wait(&m->ownerMutex); // TODO: check return value
        if (m->ownerThreadId == 0) {
            // The lock is free to get
            m->ownerThreadId = current_thread_id;
            owned = JAVA_TRUE;
        }
        sem_post(&m->ownerMutex); // TODO: check return value

        if (owned) {
            // Yeah we get the lock!
            // Check if current thread is interrupted
            pthread_mutex_lock(&nativeThreadContext->blockingMutex); // TODO: check return value
            interrupted = nativeThreadContext->interrupted;
            nativeThreadContext->currentBlockingMonitor = NULL; // We already not blocked, so unregister it.
            nativeThreadContext->blockBy = block_none;
            if (interrupted) {
                // Don't release the lock since interrupt will increase the lock by extra 1, so we remove it here.

                nativeThreadContext->interrupted = JAVA_FALSE; // Clear interrupt flag
            }
            pthread_mutex_unlock(&nativeThreadContext->blockingMutex);

            if (interrupted) {
                // Reset owner
                sem_wait(&m->ownerMutex); // TODO: check return value
                m->ownerThreadId = 0;
                sem_post(&m->ownerMutex); // TODO: check return value
                return thrd_interrupt;
            } else {
                // So the lock is ours!
                m->reentranceCounter++;
                return thrd_success;
            }
        } else {
            // Already locked by other thread, so this means we are waked by interrupt
            // Check if current thread is interrupted
            pthread_mutex_lock(&nativeThreadContext->blockingMutex); // TODO: check return value
            interrupted = nativeThreadContext->interrupted;
            if (interrupted) {
                nativeThreadContext->currentBlockingMonitor = NULL;
                nativeThreadContext->blockBy = block_none;
                // Don't release the lock since interrupt will increase the lock by extra 1, so we remove it here.

                nativeThreadContext->interrupted = JAVA_FALSE; // Clear interrupt flag
            }
            pthread_mutex_unlock(&nativeThreadContext->blockingMutex);
            if (interrupted) {
                return thrd_interrupt;
            } else {
                // We are not interrupted, so that means another thread that is waiting on the
                // same lock is interrupted, we release the lock and try again.
                sem_post(&m->mutex); // TODO: check return value
                if (interrupt_retry_count > 0) {
                    // Sleep for a while so we give other thread chance to get the lock
                    int sleep_for =
                            (1 << (interrupt_retry_count > 11 ? 11 : interrupt_retry_count)) * 1000; // Max 2048 ms.
                    const struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = sleep_for};
                    nanosleep(&sleep_time, NULL);
                }
                interrupt_retry_count++;
                sched_yield();
            }
        }
    }
}

int monitor_exit(VMThreadContext *ctx, JAVA_OBJECT obj) {
    VMMonitor *m = obj->monitor;
    if (m == NULL) {
        return thrd_error;
    }

    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return thrd_error;
    }
    m->reentranceCounter--;

    if (m->reentranceCounter > 0) {
        return thrd_success;
    }

    sem_wait(&m->ownerMutex); // TODO: check return value
    m->ownerThreadId = 0;
    m->reentranceCounter = 0;
    sem_post(&m->mutex); // TODO: check return value
    sem_post(&m->ownerMutex); // TODO: check return value

    return thrd_success;
}

int monitor_wait(VMThreadContext *ctx, JAVA_OBJECT obj, JAVA_LONG timeout, JAVA_INT nanos) {
    VMMonitor *m = obj->monitor;
    if (m == NULL) {
        return thrd_error;
    }

    // Make sure the obj lock is held by current thread
    JAVA_LONG current_thread_id = ctx->threadId;
    if (current_thread_id != m->ownerThreadId) {
        return thrd_error;
    }

    const int prev_count = m->reentranceCounter;


    return thrd_success;
}

JAVA_VOID monitor_notify(VMThreadContext *ctx, JAVA_OBJECT obj) {

}

JAVA_VOID monitor_notify_all(VMThreadContext *ctx, JAVA_OBJECT obj) {

}
