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

} RefTable;

typedef struct _NativeStackFrame {
    VMStackFrame baseFrame;

    // Reference table
} NativeStackFrame;


JAVA_BOOLEAN native_init();

/** Init the thread root stack frame */
void native_make_root_stack_frame(NativeStackFrame *root);

JAVA_VOID native_thread_attach_jni(VM_PARAM_CURRENT_CONTEXT);

void* native_load_library(C_CSTR name);
void* native_find_symbol(void* handle, C_CSTR symbol);

void* native_bind_method(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method);

#endif //FOXVM_VM_NATIVE_H
