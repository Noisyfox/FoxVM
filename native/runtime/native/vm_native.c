//
// Created by noisyfox on 2020/9/2.
//

#include "vm_native.h"
#include "vm_thread.h"
#include <stdio.h>

static VMSpinLock g_jniMethodLock = OPA_INT_T_INITIALIZER(0);

static void* g_libHandleThis = NULL;

static struct JNINativeInterface jni;

JAVA_BOOLEAN native_init() {
    spin_lock_init(&g_jniMethodLock);

    g_libHandleThis = native_load_library(NULL);
    assert(g_libHandleThis != NULL);

    return JAVA_TRUE;
}

void native_make_root_stack_frame(NativeStackFrame *root) {
    VMStackFrame *base = &root->baseFrame;
    // TODO: init ref table

    base->prev = base;
    base->next = base;
    base->type = VM_STACK_FRAME_NATIVE; // Root frame must be a native frame
    base->thisClass = NULL;
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

jobject native_get_local_ref(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj) {
    // This cannot be called inside a checkpoint because the obj might be moved by GC
    assert(!thread_in_checkpoint(vmCurrentContext));

    return NULL;
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
