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
    RefTable* refTable;
} NativeStackFrame;

#define native_stack_frame_start(ref_capacity)                          \
    NativeStackFrame __nativeFrame;                                     \
    RefTable __refTable;                                                \
    JAVA_OBJECT __nativeRefs[ref_capacity];                             \
    native_reftable_init(&__refTable, __nativeRefs, ref_capacity);      \
    native_frame_init(&__nativeFrame, &__refTable);                     \
    __nativeFrame.baseFrame.thisClass = vmCurrentContext->callingClass; \
    stack_frame_push(vmCurrentContext, &__nativeFrame.baseFrame);       \
    /* Native frame don't inherit exception handler from java world */  \
    __nativeFrame.baseFrame.exceptionHandler = NULL

#define native_stack_frame_end() \
    native_frame_pop(vmCurrentContext); \
    exception_raise_if_occurred(vmCurrentContext)

// Helper macros for using native handler in a non-jni native function

// Start a native scope. A local variable called `result` will be created
// which you can store a native handler to as the result of this scope.
#define native_scoped                   \
    native_stack_frame_start(10);       \
    JAVA_OBJECT javaResult = JAVA_NULL; \
    jobject result = NULL;              \
    do

// End the scope. The result handler will be dereferenced automatically
// if no exception occurred inside the scope, and is stored in the local
// variable `javaResult`.
#define native_scope_end()                                          \
    while (0);                                                      \
    native_end:                                                     \
    if (!exception_occurred(vmCurrentContext)) {                    \
        javaResult = native_dereference(vmCurrentContext, result);  \
    }                                                               \
    native_frame_pop(vmCurrentContext)

#define native_check_exception() \
    if (exception_occurred(vmCurrentContext)) goto native_end

#define native_handler_of(var, expression)                  \
    {                                                       \
        JAVA_OBJECT _tmp = (JAVA_OBJECT)(expression);       \
        native_check_exception();                           \
        var = native_get_local_ref(vmCurrentContext, _tmp); \
    }                                                       \
    native_check_exception()

#define native_handler_new(var, expression)                 \
    jobject var = NULL;                                     \
    native_handler_of(var, expression)

JAVA_BOOLEAN native_init();

/** Init the thread root stack frame */
void native_make_root_stack_frame(NativeStackFrame *root);

void native_reftable_init(RefTable *table, JAVA_OBJECT* storage, jint capacity);

void native_frame_init(NativeStackFrame *frame, RefTable* initialRefTable);
void native_frame_pop(VM_PARAM_CURRENT_CONTEXT);

JAVA_VOID native_thread_attach_jni(VM_PARAM_CURRENT_CONTEXT);

void* native_load_library(C_CSTR name);
void* native_find_symbol(void* handle, C_CSTR symbol);

void* native_bind_method(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method);

JAVA_VOID native_enter_jni(VM_PARAM_CURRENT_CONTEXT);
JAVA_VOID native_exit_jni(VM_PARAM_CURRENT_CONTEXT);

jobject native_get_local_ref(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj);
JAVA_OBJECT native_dereference(VM_PARAM_CURRENT_CONTEXT, jobject ref);

#endif //FOXVM_VM_NATIVE_H
