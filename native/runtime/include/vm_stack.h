//
// Created by noisyfox on 2020/4/24.
//

#ifndef FOXVM_VM_STACK_H
#define FOXVM_VM_STACK_H

#include "vm_base.h"
#include <assert.h>

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

    JAVA_CLASS thisClass; // Reference to current class
    VMOperandStack operandStack;
    VMLocals locals;

    uint16_t currentLine; // Current line number in the source file
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
    __stackFrame.thisClass = vmCurrentContext->callingClass;                \
    stack_frame_push(vmCurrentContext, &__stackFrame)

#define stack_frame_start_zero()                                            \
    VMStackFrame __stackFrame;                                              \
    stack_frame_init(&__stackFrame, NULL, 0, 0);                            \
    __stackFrame.thisClass = vmCurrentContext->callingClass;                \
    stack_frame_push(vmCurrentContext, &__stackFrame)

#define stack_frame_end() \
    stack_frame_pop(vmCurrentContext)

#define THIS_CLASS __stackFrame->thisClass

#define stack_frame_iterate(thread, frame) \
    for (VMStackFrame *frame = stack_frame_top(thread); frame != &thread->frameRoot; frame = frame->prev)

#define stack_frame_operand_stack_iterate(frame, slot) \
    for (VMStackSlot *slot = frame->operandStack.slots; slot < frame->operandStack.top; slot++)

#define stack_frame_local_iterate(frame, slot) \
    for(VMStackSlot* slot = frame->locals.slots; slot < frame->locals.slots + frame->locals.maxLocals; slot++)

#define stack_check_overflow() \
    assert(__stackFrame.operandStack.top < __stackFrame.operandStack.slots + __stackFrame.operandStack.maxStack)

#define stack_push_object(obj) do {                             \
    stack_check_overflow();                                     \
    __stackFrame.operandStack.top->type = VM_SLOT_OBJECT;       \
    __stackFrame.operandStack.top->data.o = (obj);              \
    __stackFrame.operandStack.top++;                            \
} while(0)

/**
 * Transfer arguments from caller's operand stack to current method's local slots
 */
static inline void local_transfer_arguments(VM_PARAM_CURRENT_CONTEXT, uint16_t argument_count) {
    VMStackFrame *currentFrame = stack_frame_top(vmCurrentContext);
    VMOperandStack *stackFrom = &currentFrame->prev->operandStack;
    VMLocals *localsTo = &currentFrame->locals;

    VMStackSlot *stackSlotFrom = stackFrom->top - argument_count;
    assert(stackSlotFrom >= stackFrom->slots);
    stackFrom->top = stackSlotFrom; // Pop all arguments from previous stack frame

    int localIndex = 0;
    while (stackSlotFrom < stackFrom->top) {
        switch (stackSlotFrom->type) {
            case VM_SLOT_INVALID:
                assert(!"Invalid slot type");
                break;
            case VM_SLOT_OBJECT:
                assert(localIndex < localsTo->maxLocals);
                localsTo->slots[localIndex].type = VM_SLOT_OBJECT;
                localsTo->slots[localIndex].data.o = stackSlotFrom->data.o;
                localIndex++;
                break;
            case VM_SLOT_INT:
                assert(localIndex < localsTo->maxLocals);
                localsTo->slots[localIndex].type = VM_SLOT_INT;
                localsTo->slots[localIndex].data.i = stackSlotFrom->data.i;
                localIndex++;
                break;
            case VM_SLOT_LONG:
                assert(localIndex + 1 < localsTo->maxLocals);
                localsTo->slots[localIndex].type = VM_SLOT_LONG;
                localsTo->slots[localIndex].data.l = stackSlotFrom->data.l;
                localIndex++;
                // Long uses 2 local slots
                localsTo->slots[localIndex].type = VM_SLOT_INVALID;
                localIndex++;
                break;
            case VM_SLOT_FLOAT:
                assert(localIndex < localsTo->maxLocals);
                localsTo->slots[localIndex].type = VM_SLOT_FLOAT;
                localsTo->slots[localIndex].data.f = stackSlotFrom->data.f;
                localIndex++;
                break;
            case VM_SLOT_DOUBLE:
                assert(localIndex + 1 < localsTo->maxLocals);
                localsTo->slots[localIndex].type = VM_SLOT_DOUBLE;
                localsTo->slots[localIndex].data.d = stackSlotFrom->data.d;
                localIndex++;
                // Double uses 2 local slots
                localsTo->slots[localIndex].type = VM_SLOT_INVALID;
                localIndex++;
                break;
        }

        stackSlotFrom++;
    }
}

#endif //FOXVM_VM_STACK_H
