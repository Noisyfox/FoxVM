//
// Created by noisyfox on 2020/9/2.
//

#include "vm_native.h"
#include "vm_thread.h"
#include <stdio.h>
#include <stdint.h>

static VMSpinLock g_jniMethodLock = OPA_INT_T_INITIALIZER(0);

static void* g_libHandleThis = NULL;

static struct JNINativeInterface jni;

// A special object that marks the current handle has been deleted
// by `DeleteLocalRef`, `DeleteGlobalRef` or `DeleteWeakGlobalRef`.
// We can't just put NULL in a deleted ref because we use NULL to
// mark a `WeakGlobalRef` that has been reclaimed by GC. That ref
// cannot be reused until `DeleteWeakGlobalRef` has been called.
static JavaObjectBase g_deletedHandle;

// Value that an empty ref table is filled with
static const JAVA_OBJECT g_badHandle = (JAVA_OBJECT) UINT64_C(0xABABABABABABABAB);

static JAVA_VOID native_clear_reftable(RefTable *table) {
    for (jint i = 0; i < table->capacity; i++) {
        table->objects[i] = g_badHandle;
    }
    table->top = 0;
}

JAVA_BOOLEAN native_init() {
    spin_lock_init(&g_jniMethodLock);

    g_libHandleThis = native_load_library(NULL);
    assert(g_libHandleThis != NULL);

    return JAVA_TRUE;
}

void native_make_root_stack_frame(NativeStackFrame *root) {
    VMStackFrame *base = &root->baseFrame;

    native_frame_init(root, NULL, 0);

    base->prev = base;
    base->next = base;
}

void native_frame_init(NativeStackFrame *frame, JAVA_OBJECT* refs, jint capacity) {
    VMStackFrame *base = &frame->baseFrame;

    base->prev = NULL;
    base->next = NULL;
    base->type = VM_STACK_FRAME_NATIVE;
    base->thisClass = NULL;

    frame->isJniLocalFrame = JAVA_FALSE;

    // init ref table
    frame->refTable.next = NULL;
    frame->refTable.capacity = capacity;
    frame->refTable.top = 0;
    frame->refTable.objects = refs;
    native_clear_reftable(&frame->refTable);
}

JAVA_VOID native_thread_attach_jni(VM_PARAM_CURRENT_CONTEXT) {
    vmCurrentContext->jni = &jni;
}

void* native_bind_method(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method) {
    printf("Resolving native method %s\n", method->longName);

    spin_lock_enter(vmCurrentContext, &g_jniMethodLock);
    // Check again after obtaining the lock
    void *nativePtr = method->nativePtr;
    if (nativePtr) {
        spin_lock_exit(&g_jniMethodLock);
        return nativePtr;
    }

    // The VM checks for a method name match for methods that reside in the native library.
    // The VM looks first for the short name; that is, the name without the argument signature.
    nativePtr = native_find_symbol(g_libHandleThis, method->shortName);
    if (!nativePtr) {
        // It then looks for the long name, which is the name with the argument signature.
        nativePtr = native_find_symbol(g_libHandleThis, method->longName);
    }

    method->nativePtr = nativePtr;
    spin_lock_exit(&g_jniMethodLock);

    if (!nativePtr) {
        // TODO: throw UnsatisfiedLinkError instead.
        assert(!"Unable to find native method");
        exit(-5);
    }

    return nativePtr;
}

JAVA_VOID native_enter_jni(VM_PARAM_CURRENT_CONTEXT) {
    // This cannot be called inside a checkpoint
    assert(!thread_in_saferegion(vmCurrentContext));

    // Enter the saferegion so GC can start working on this thread
    thread_enter_saferegion(vmCurrentContext);
}

JAVA_VOID native_exit_jni(VM_PARAM_CURRENT_CONTEXT) {
    assert(thread_in_saferegion(vmCurrentContext));
    // Leave saferegion so GC won't move any object
    thread_leave_saferegion(vmCurrentContext);
    // Make sure we do left the saferegion
    assert(!thread_in_saferegion(vmCurrentContext));
}

jobject native_get_local_ref(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj) {
    // This cannot be called inside a saferegion because the obj might be moved by GC
    assert(!thread_in_saferegion(vmCurrentContext));

    VMStackFrame *top = stack_frame_top(vmCurrentContext);
    assert(top->type == VM_STACK_FRAME_NATIVE);
    NativeStackFrame *frame = (NativeStackFrame *) top;

    if (obj == JAVA_NULL) {
        return NULL;
    }

    return NULL;
}

JAVA_OBJECT native_dereference(VM_PARAM_CURRENT_CONTEXT, jobject ref) {
    // This cannot be called inside a saferegion because the obj might be moved by GC
    assert(!thread_in_saferegion(vmCurrentContext));

    VMStackFrame *top = stack_frame_top(vmCurrentContext);
    assert(top->type == VM_STACK_FRAME_NATIVE);
    NativeStackFrame *frame = (NativeStackFrame *) top;

    if (ref == NULL) {
        return JAVA_NULL;
    }

    JAVA_OBJECT obj = *((JAVA_OBJECT *) ref);
    assert(obj != &g_deletedHandle);

    if (obj == JAVA_NULL) {
        // TODO: assert it's a weak global ref
        return JAVA_NULL;
    }

    // Check if it points to a unused ref
    assert(obj != g_badHandle);

    return obj;
}

// Implementations of JNI interface methods
static jfieldID GetStaticFieldID(JNIEnv* env, jclass cls, const char* name, const char* sig) {
    return NULL;
}

static struct JNINativeInterface jni = {
    .reserved0 = NULL,
    .reserved1 = NULL,
    .reserved2 = NULL,
    .reserved3 = NULL,
    .GetStaticFieldID = GetStaticFieldID,
};
