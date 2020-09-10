//
// Created by noisyfox on 2018/9/23.
//

#ifndef FOXVM_VM_EXCEPTION_H
#define FOXVM_VM_EXCEPTION_H

#include <setjmp.h>
#include "vm_stack.h"

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

JAVA_BOOLEAN exception_matches(JAVA_OBJECT ex, int32_t current_sp, int32_t start, int32_t end, JavaClassInfo* type);

#define EX_VAL 1

#define exception_frame_start()                                         \
    ExceptionFrame __exceptionFrame;                                    \
    exception_frame_push(&STACK_FRAME.baseFrame, &__exceptionFrame);    \
    if (setjmp(__exceptionFrame.jmpTarget) == EX_VAL) {                 \
        JAVA_OBJECT ex = vmCurrentContext->exception;                   \
        vmCurrentContext->exception = JAVA_NULL;                        \
        assert(ex);                                                     \
        stack_clear(OP_STACK);                                          \
        stack_push_object(ex)

#define exception_new_block(start, end, handler, type) \
        if(exception_matches(ex, STACK_FRAME.currentLabel, start, end, type)) goto handler

#define exception_block_end()                               \
        /* Exception not handled */                         \
        exception_frame_pop(&STACK_FRAME.baseFrame);        \
        /* TODO: Release lock for synchronized method */    \
        /* Rethrow the same exception again */              \
        bc_athrow();                                        \
    }

#endif //FOXVM_VM_EXCEPTION_H
