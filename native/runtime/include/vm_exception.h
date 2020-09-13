//
// Created by noisyfox on 2018/9/23.
//

#ifndef FOXVM_VM_EXCEPTION_H
#define FOXVM_VM_EXCEPTION_H

#include <setjmp.h>
#include "vm_stack.h"
#include "vm_thread.h"

typedef struct _ExceptionFrame {
    struct _ExceptionFrame *previous;

    jmp_buf jmpTarget;
} ExceptionFrame;

static inline JAVA_VOID exception_frame_push(VMStackFrame *frame, ExceptionFrame* exceptionFrame) {
    exceptionFrame->previous = frame->exceptionHandler;
    frame->exceptionHandler = exceptionFrame;
}

static inline JAVA_VOID exception_frame_pop(VMStackFrame *frame) {
    frame->exceptionHandler = frame->exceptionHandler->previous;
}

static inline JAVA_BOOLEAN exception_occurred(VM_PARAM_CURRENT_CONTEXT) {
    return vmCurrentContext->exception != JAVA_NULL;
}

static inline JAVA_OBJECT exception_clear(VM_PARAM_CURRENT_CONTEXT) {
    JAVA_OBJECT ex = vmCurrentContext->exception;
    vmCurrentContext->exception = JAVA_NULL;
    return ex;
}

JAVA_BOOLEAN exception_matches(JAVA_OBJECT ex, int32_t current_sp, int32_t start, int32_t end, JavaClassInfo* type);

JAVA_VOID exception_set(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT ex);
JAVA_VOID exception_raise(VM_PARAM_CURRENT_CONTEXT);

static inline JAVA_VOID exception_raise_if_occurred(VM_PARAM_CURRENT_CONTEXT) {
    if (exception_occurred(vmCurrentContext)) {
        exception_raise(vmCurrentContext);
    }
}

JAVA_OBJECT exception_new(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *exClass, C_CSTR message);
JAVA_OBJECT exception_newf(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *exClass, C_CSTR format, ...);

JAVA_VOID exception_set_new(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *exClass, C_CSTR message);
JAVA_VOID exception_set_newf(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *exClass, C_CSTR format, ...);

JAVA_VOID exception_set_NoSuchFieldError(VM_PARAM_CURRENT_CONTEXT, C_CSTR message);
JAVA_VOID exception_set_IncompatibleClassChangeError(VM_PARAM_CURRENT_CONTEXT, C_CSTR message);
JAVA_VOID exception_set_UnsatisfiedLinkError(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method, C_CSTR message);

JAVA_VOID exception_set_NullPointerException(VM_PARAM_CURRENT_CONTEXT, C_CSTR variableName);
JAVA_VOID exception_set_ArrayStoreException(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *arrayType, JavaClassInfo *elementType);
JAVA_VOID exception_set_IllegalArgumentException(VM_PARAM_CURRENT_CONTEXT, C_CSTR message);

#define EX_VAL 1

#define exception_frame_start()                                             \
    ExceptionFrame __exceptionFrame;                                        \
    exception_frame_push(&STACK_FRAME.baseFrame, &__exceptionFrame);        \
    if (setjmp(__exceptionFrame.jmpTarget) == EX_VAL) {                     \
        stack_frame_pop_deeper(vmCurrentContext, &STACK_FRAME.baseFrame);   \
        JAVA_OBJECT ex = exception_clear(vmCurrentContext);                 \
        assert(ex);                                                         \
        stack_clear(OP_STACK);                                              \
        stack_push_object(ex)

#define exception_new_block(start, end, handler, type) \
        if(exception_matches(ex, STACK_FRAME.currentLabel, start, end, type)) goto handler

#define exception_not_handled()                             \
        /* Exception not handled */                         \
        exception_frame_pop(&STACK_FRAME.baseFrame);        \
        /* TODO: Release lock for synchronized method */    \

#define exception_block_end()                               \
        /* Rethrow the same exception again */              \
        bc_athrow();                                        \
    }

/**
 * Capture all exception, then re-set the exception and return current
 * function with given result.
 *
 * This is used when a native method calls some Java method and want to
 * handle the exception in a different fashion.
 */
#define exception_suppressed(result) do {       \
    exception_not_handled() {                   \
        exception_set(vmCurrentContext, ex);    \
        stack_frame_end();                      \
        return result;                          \
    }                                           \
} while(0)

#define exception_suppressedv() do {              \
    exception_not_handled() {                   \
        exception_set(vmCurrentContext, ex);    \
        stack_frame_end();                      \
        return;                                 \
    }                                           \
} while(0)

#endif //FOXVM_VM_EXCEPTION_H
