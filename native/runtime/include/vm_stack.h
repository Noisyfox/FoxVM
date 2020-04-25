//
// Created by noisyfox on 2020/4/24.
//

#ifndef FOXVM_VM_STACK_H
#define FOXVM_VM_STACK_H

#include "vm_base.h"

typedef struct {
    uint16_t maxStack; // Max depth of the operand stack
    VMStackSlot *slots; // Bottom of the stack
    VMStackSlot *top; // Current position of the stack
} VMOperandStack;

typedef struct {
    uint16_t maxLocals; // Local variable slot array length
    VMStackSlot *slots; // Header of the slot array
} VMLocals;

typedef struct _VMStackFrame {
    struct _VMStackFrame *prev;
    struct _VMStackFrame *next;

    VMOperandStack operandStack;
    VMLocals locals;
} VMStackFrame;

/** Init the thread root stack frame */
void stack_frame_make_root(VMStackFrame *root);

/** Current top of the call stack */
VMStackFrame *stack_frame_top(VM_PARAM_CURRENT_CONTEXT);

/** Init the stack frame of current method */
void stack_frame_init(VMStackFrame *frame, VMStackSlot *slot_base, uint16_t max_stack, uint16_t max_locals);

/** Push the given frame to the top of the current call stack */
void stack_frame_push(VM_PARAM_CURRENT_CONTEXT, VMStackFrame *frame);

/** Pop the top of the call stack frame */
void stack_frame_pop(VM_PARAM_CURRENT_CONTEXT);

#define stack_frame_start(max_stack, max_locals)                            \
    VMStackFrame __stackFrame;                                              \
    VMStackSlot __slots[(max_stack) + (max_locals)];                        \
    stack_frame_init(&__stackFrame, __slots, (max_stack), (max_locals));    \
    stack_frame_push(vmCurrentContext, &__stackFrame)

#define stack_frame_end() \
    stack_frame_pop(vmCurrentContext)

#define stack_frame_iterate(thread, frame) \
    for (VMStackFrame *frame = stack_frame_top(thread); frame != &thread->frameRoot; frame = frame->prev)

#define stack_frame_operand_stack_iterate(frame, slot) \
    for (VMStackSlot *slot = frame->operandStack.slots; slot < frame->operandStack.top; slot++)

#define stack_frame_local_iterate(frame, slot) \
    for(VMStackSlot* slot = frame->locals.slots; slot < frame->locals.slots + frame->locals.maxLocals; slot++)

#endif //FOXVM_VM_STACK_H
