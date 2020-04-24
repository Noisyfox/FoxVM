//
// Created by noisyfox on 2020/4/24.
//

#include "vm_stack.h"
#include "vm_thread.h"
#include <assert.h>

void stack_frame_make_root(VMStackFrame *root) {
    stack_frame_init(root, NULL, 0, 0);

    root->prev = root;
    root->next = root;
}

inline VMStackFrame *stack_frame_top(VM_PARAM_CURRENT_CONTEXT) {
    return vmCurrentContext->frameRoot.prev;
}

void stack_frame_init(VMStackFrame *frame, VMStackSlot *slot_base, uint16_t max_stack, uint16_t max_locals) {
    frame->prev = NULL;
    frame->next = NULL;

    frame->operandStack = (VMOperandStack) {
            .maxStack = max_stack,
            .slots = slot_base,
            .top = slot_base,
    };
    frame->locals = (VMLocals) {
            .maxLocals = max_locals,
            .slots = slot_base + max_stack,
    };
}

void stack_frame_push(VM_PARAM_CURRENT_CONTEXT, VMStackFrame *frame) {
    assert(frame->next == NULL);
    assert(frame->prev == NULL);

    VMStackFrame *curr = stack_frame_top(vmCurrentContext);

    frame->next = curr->next;
    frame->prev = curr;

    frame->prev->next = frame;
    frame->next->prev = frame;
}

void stack_frame_pop(VM_PARAM_CURRENT_CONTEXT) {
    VMStackFrame *curr = stack_frame_top(vmCurrentContext);
    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;

    curr->prev = NULL;
    curr->next = NULL;
}
