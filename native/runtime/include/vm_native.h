//
// Created by noisyfox on 2020/9/2.
//

#ifndef FOXVM_VM_NATIVE_H
#define FOXVM_VM_NATIVE_H

#include "vm_base.h"
#include "vm_stack.h"
#include "jni.h"

typedef struct _RefTable {
    struct _RefTable* next;
    jint capacity;
    jint top;
    JAVA_OBJECT* objects;
} RefTable;

typedef struct _NativeStackFrame {
    VMStackFrame baseFrame;

    // Whether this frame is created by `PushLocalFrame` jni call.
    // If true, then the root reftable will be freed when this frame
    // is popped, as well as this frame itself.
    JAVA_BOOLEAN isJniLocalFrame;

    // Reference table
    RefTable refTable;
} NativeStackFrame;

#define native_stack_frame_start(ref_capacity)                          \
    NativeStackFrame __nativeFrame;                                     \
    JAVA_OBJECT __nativeRefs[ref_capacity];                             \
    native_frame_init(&__nativeFrame, __nativeRefs, ref_capacity);      \
    __nativeFrame.baseFrame.thisClass = vmCurrentContext->callingClass; \
    stack_frame_push(vmCurrentContext, &__nativeFrame.baseFrame)

#define native_stack_frame_end() \
    stack_frame_pop(vmCurrentContext)

JAVA_BOOLEAN native_init();

/** Init the thread root stack frame */
void native_make_root_stack_frame(NativeStackFrame *root);

void native_frame_init(NativeStackFrame *frame, JAVA_OBJECT* refs, jint capacity);

JAVA_VOID native_thread_attach_jni(VM_PARAM_CURRENT_CONTEXT);

void* native_load_library(C_CSTR name);
void* native_find_symbol(void* handle, C_CSTR symbol);

void* native_bind_method(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method);

JAVA_VOID native_enter_jni(VM_PARAM_CURRENT_CONTEXT);
JAVA_VOID native_exit_jni(VM_PARAM_CURRENT_CONTEXT);

jobject native_get_local_ref(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj);
JAVA_OBJECT native_dereference(VM_PARAM_CURRENT_CONTEXT, jobject ref);

#endif //FOXVM_VM_NATIVE_H
