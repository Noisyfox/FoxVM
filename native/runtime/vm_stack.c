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

    frame->thisClass = NULL;
    frame->currentLine = 0;

    /**
     * Locals are stored at the beginning of the slot_base,
     * so each local slot can be accessed directly by slot_base[n].
     */
    frame->locals = (VMLocals) {
            .maxLocals = max_locals,
            .slots = slot_base,
    };

    frame->operandStack = (VMOperandStack) {
            .maxStack = max_stack,
            .slots = slot_base + max_locals,
            .top = slot_base + max_locals,
    };

    // Init local slots to empty
    stack_frame_local_iterate(frame, slot) {
        slot->type = VM_SLOT_INVALID;
    }
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
