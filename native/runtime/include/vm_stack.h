//
// Created by noisyfox on 2020/4/24.
//

#ifndef FOXVM_VM_STACK_H
#define FOXVM_VM_STACK_H

#include "vm_base.h"
#include <assert.h>
#include <stdlib.h>

struct _ExceptionFrame;

static inline VMTypeCategory slot_type_category(VMStackSlotType type) {
    switch (type) {
        case VM_SLOT_OBJECT:
        case VM_SLOT_INT:
        case VM_SLOT_FLOAT:
            return VM_TYPE_CAT_1;
        case VM_SLOT_LONG:
        case VM_SLOT_DOUBLE:
            return VM_TYPE_CAT_2;
        case VM_SLOT_INVALID:
            assert(!"Invalid slot type");
    }

    assert(!"Unexpected slot type");
    exit(-1);
}

typedef struct {
    uint16_t maxStack; // Max depth of the operand stack
    VMStackSlot *slots; // Bottom of the stack
    VMStackSlot *top; // Current position of the stack
} VMOperandStack;

typedef struct {
    uint16_t maxLocals; // Local variable slot array length
    VMStackSlot *slots; // Header of the slot array
} VMLocals;

typedef enum {
    VM_STACK_FRAME_JAVA,
    VM_STACK_FRAME_NATIVE,
} StackFrameType;

typedef struct _VMStackFrame {
    struct _VMStackFrame *prev;
    struct _VMStackFrame *next;
    StackFrameType type;

    struct _ExceptionFrame *exceptionHandler;

    JAVA_CLASS thisClass; // Reference to current class
} VMStackFrame;

typedef struct _JavaStackFrame {
    VMStackFrame baseFrame;

    MethodInfo *currentMethod; // Might be null for internal system methods
    VMOperandStack operandStack;
    VMLocals locals;

    uint16_t currentLine; // Current line number in the source file
    int32_t currentLabel; // Current label in the instruction stream
} JavaStackFrame;

/** Current top of the call stack */
VMStackFrame *stack_frame_top(VM_PARAM_CURRENT_CONTEXT);

/** Push the given frame to the top of the current call stack */
void stack_frame_push(VM_PARAM_CURRENT_CONTEXT, VMStackFrame *frame);

/** Pop the top of the call stack frame */
void stack_frame_pop(VM_PARAM_CURRENT_CONTEXT);

/**
 * Pop all stack frames that are deeper than current one,
 * make current one the top frame. This is used at exception handler to
 * fix the stack frame.
 */
void stack_frame_pop_deeper(VM_PARAM_CURRENT_CONTEXT, VMStackFrame *frame);

/** Init the stack frame of current method */
void stack_frame_init_java(JavaStackFrame *frame, VMStackSlot *slot_base, uint16_t max_stack, uint16_t max_locals);

#define STACK_FRAME __stackFrame
#define STACK_SLOTS __slots
#define OP_STACK (&STACK_FRAME.operandStack)

#define stack_frame_start(method_info, max_stack, max_locals)                       \
    JavaStackFrame STACK_FRAME;                                                     \
    VMStackSlot STACK_SLOTS[(max_stack) + (max_locals)];                            \
    stack_frame_init_java(&STACK_FRAME, STACK_SLOTS, (max_stack), (max_locals));    \
    STACK_FRAME.baseFrame.thisClass = vmCurrentContext->callingClass;               \
    STACK_FRAME.currentMethod = method_info;                                        \
    stack_frame_push(vmCurrentContext, &STACK_FRAME.baseFrame)

#define stack_frame_start_zero(method_info)                                         \
    JavaStackFrame STACK_FRAME;                                                     \
    stack_frame_init_java(&STACK_FRAME, NULL, 0, 0);                                \
    STACK_FRAME.baseFrame.thisClass = vmCurrentContext->callingClass;               \
    STACK_FRAME.currentMethod = method_info;                                        \
    stack_frame_push(vmCurrentContext, &STACK_FRAME.baseFrame)

#define stack_frame_end() \
    stack_frame_pop(vmCurrentContext)

#define THIS_CLASS STACK_FRAME.baseFrame.thisClass

#define local_of(local) STACK_SLOTS[local]

#define stack_frame_iterate(thread, frame) \
    for (VMStackFrame *frame = stack_frame_top(thread); frame != (VMStackFrame *)&thread->frameRoot; frame = frame->prev)

#define stack_frame_operand_stack_iterate(frame, slot) \
    for (VMStackSlot *slot = frame->operandStack.slots; slot < frame->operandStack.top; slot++)

#define stack_frame_local_iterate(frame, slot) \
    for(VMStackSlot* slot = frame->locals.slots; slot < frame->locals.slots + frame->locals.maxLocals; slot++)

#define stack_check_overflow0(stack) \
    assert((stack)->top < (stack)->slots + (stack)->maxStack)

#define stack_check_overflow() \
    stack_check_overflow0(OP_STACK)

#define stack_push_object(obj) do {              \
    stack_check_overflow();                      \
    OP_STACK->top->type = VM_SLOT_OBJECT;        \
    OP_STACK->top->data.o = (obj);               \
    OP_STACK->top++;                             \
} while(0)

#define stack_push_int(val) do {                 \
    stack_check_overflow();                      \
    OP_STACK->top->type = VM_SLOT_INT;           \
    OP_STACK->top->data.i = (val);               \
    OP_STACK->top++;                             \
} while(0)

#define stack_push_long(val) do {                \
    stack_check_overflow();                      \
    OP_STACK->top->type = VM_SLOT_LONG;          \
    OP_STACK->top->data.l = (val);               \
    OP_STACK->top++;                             \
} while(0)

#define stack_push_float(val) do {               \
    stack_check_overflow();                      \
    OP_STACK->top->type = VM_SLOT_FLOAT;         \
    OP_STACK->top->data.f = (val);               \
    OP_STACK->top++;                             \
} while(0)

#define stack_push_double(val) do {              \
    stack_check_overflow();                      \
    OP_STACK->top->type = VM_SLOT_DOUBLE;        \
    OP_STACK->top->data.d = (val);               \
    OP_STACK->top++;                             \
} while(0)

#define slot_copy(to, from) do { \
    (to)->type = (from)->type;   \
    (to)->data = (from)->data;   \
} while(0)

static inline void slot_swap(VMStackSlot *a, VMStackSlot *b) {
    VMStackSlot tmp;

    slot_copy(&tmp, a);
    slot_copy(a, b);
    slot_copy(b, &tmp);
}

/**
 * Push the value from [from] slot on to the top of the given stack.
 * This also checks if the data type matches the [required_type].
 */
static inline void stack_push_from(VMOperandStack *stack, VMStackSlot *from, VMStackSlotType required_type) {
    stack_check_overflow0(stack);
    assert(from->type == required_type); // Check the type

    // Push the data
    slot_copy(stack->top, from);

    // Increase the top pointer
    stack->top++;
}

#define stack_peek_value(i) \
    VMStackSlot *value##i = stack->top - i; \
    assert(value##i >= stack->slots)

#define stack_assert_cat(i, c) \
    if (slot_type_category(value##i->type) != c) assert(!"value"#i" is not "#c)


/**
 * Empty the given stack. Typically used when handling exceptions.
 */
static inline void stack_clear(VMOperandStack *stack) {
    stack->top = stack->slots;
}

/**
 * Pop the top of the given stack, and store the data into given [to] slot.
 * This also checks if the data type matches the [required_type].
 */
static inline void stack_pop_to(VMOperandStack *stack, VMStackSlot *to, VMStackSlotType required_type) {
    stack_peek_value(1);
    assert(value1->type == required_type); // Check the type

    // Pop the data
    stack->top = value1;
    value1->type = VM_SLOT_INVALID;

    // Copy the slot to the target slot
    to->type = required_type;
    to->data = value1->data;
}

/**
 * Pop a category 1 computational type value from the top of the operand stack
 */
static inline void stack_pop(VMOperandStack *stack) {
    stack_peek_value(1);
    stack_assert_cat(1, VM_TYPE_CAT_1);

    // Pop the data
    stack->top = value1;
    value1->type = VM_SLOT_INVALID;
}

/**
 * If top of the stack are two category 1 computational type, then pop both of them.
 * If top of the stack is is a value of a category 2 computational type, then just pop it.
 */
static inline void stack_pop2(VMOperandStack *stack) {
    // Try the most top slot first
    stack_peek_value(1);
    switch (slot_type_category(value1->type)) {
        case VM_TYPE_CAT_1:
            // Do nothing for now, need to check the second one
            break;
        case VM_TYPE_CAT_2:
            // Pop the data
            stack->top = value1;
            value1->type = VM_SLOT_INVALID;
            return;
    }

    // Test the value2 type
    stack_peek_value(2);
    stack_assert_cat(2, VM_TYPE_CAT_1);
    // We have two category 1 computational type, pop both
    value1->type = VM_SLOT_INVALID;
    value2->type = VM_SLOT_INVALID;
    stack->top = value2;
}

/**
 * Duplicate the top operand stack value
 */
static inline void stack_dup(VMOperandStack *stack) {
    stack_peek_value(1);
    stack_assert_cat(1, VM_TYPE_CAT_1);

    // Push value1 to dup it
    stack_push_from(stack, value1, value1->type);
}

#define _stack_dup_x1() do {                        \
    /* First duplicate value1 */                    \
    stack_push_from(stack, value1, value1->type);   \
    /* Then copy value2 to value1 */                \
    slot_copy(value1, value2);                      \
    /* Then copy stack top to value2 */             \
    slot_copy(value2, stack->top - 1);              \
} while(0)

/**
 * Duplicate the top operand stack value and insert two values down
 */
static inline void stack_dup_x1(VMOperandStack *stack) {
    stack_peek_value(1);
    stack_assert_cat(1, VM_TYPE_CAT_1);
    stack_peek_value(2);
    stack_assert_cat(2, VM_TYPE_CAT_1);

    _stack_dup_x1();
}

#define _stack_dup_x2() do {                        \
    /* First duplicate value1 */                    \
    stack_push_from(stack, value1, value1->type);   \
    /* Then copy value2 to value1 */                \
    slot_copy(value1, value2);                      \
    /* Then copy value3 to value2 */                \
    slot_copy(value2, value3);                      \
    /* Then copy stack top to value3 */             \
    slot_copy(value3, stack->top - 1);              \
} while(0)

/**
 * Duplicate the top operand stack value and insert two or three values down
 */
static inline void stack_dup_x2(VMOperandStack *stack) {
    stack_peek_value(1);
    stack_assert_cat(1, VM_TYPE_CAT_1);
    stack_peek_value(2);

    switch (slot_type_category(value2->type)) {
        case VM_TYPE_CAT_1:
            // Do nothing for now, need to check the third one
            break;
        case VM_TYPE_CAT_2:
            // Form 2, same as dup_x1
            _stack_dup_x1();
            return;
    }

    // Test the value3 type
    stack_peek_value(3);
    stack_assert_cat(3, VM_TYPE_CAT_1);
    // Form 1
    _stack_dup_x2();
}

/**
 * Duplicate the top one or two operand stack values
 */
static inline void stack_dup2(VMOperandStack *stack) {
    stack_peek_value(1);

    switch (slot_type_category(value1->type)) {
        case VM_TYPE_CAT_1:
            // Do nothing for now, need to check the second one
            break;
        case VM_TYPE_CAT_2:
            // Form 2, same as dup
            stack_push_from(stack, value1, value1->type);
            return;
    }

    // Test the value2 type
    stack_peek_value(2);
    stack_assert_cat(2, VM_TYPE_CAT_1);
    // Form 1
    /* Push value2 first */
    stack_push_from(stack, value2, value2->type);
    /* Then push value1 */
    stack_push_from(stack, value1, value1->type);
}

#define _stack_dup2_x1() do {                       \
    /* First duplicate value2 */                    \
    stack_push_from(stack, value2, value2->type);   \
    /* Then duplicate value1 */                     \
    stack_push_from(stack, value1, value1->type);   \
    /* Then copy value3 to value1 */                \
    slot_copy(value1, value3);                      \
    /* Then copy value2 to value3 */                \
    slot_copy(value3, value2);                      \
    /* Then copy stack top to value2 */             \
    slot_copy(value2, stack->top - 1);              \
} while(0)

/**
 * Duplicate the top one or two operand stack values and insert two or three values down
 */
static inline void stack_dup2_x1(VMOperandStack *stack) {
    stack_peek_value(1);
    stack_peek_value(2);
    stack_assert_cat(2, VM_TYPE_CAT_1);

    switch (slot_type_category(value1->type)) {
        case VM_TYPE_CAT_1:
            // Do nothing for now, need to check the third one
            break;
        case VM_TYPE_CAT_2:
            // Form 2, same as dup_x1
            _stack_dup_x1();
            return;
    }

    // Test the value3 type
    stack_peek_value(3);
    stack_assert_cat(3, VM_TYPE_CAT_1);
    // Form 1
    _stack_dup2_x1();
}

/**
 * Duplicate the top one or two operand stack values and insert two, three, or four values down
 */
static inline void stack_dup2_x2(VMOperandStack *stack) {
    stack_peek_value(1);
    stack_peek_value(2);

    switch (slot_type_category(value1->type)) {
        case VM_TYPE_CAT_1: {
            // Test the value2 type
            stack_assert_cat(2, VM_TYPE_CAT_1);

            stack_peek_value(3);
            switch (slot_type_category(value3->type)) {
                case VM_TYPE_CAT_1: {
                    // Test the value4 type
                    stack_peek_value(4);
                    stack_assert_cat(4, VM_TYPE_CAT_1);
                    // Form 1
                    /* First duplicate value2 */
                    stack_push_from(stack, value2, value2->type);
                    /* Then duplicate value1 */
                    stack_push_from(stack, value1, value1->type);
                    /* Then copy value3 to value1 */
                    slot_copy(value1, value3);
                    /* Then copy value4 to value2 */
                    slot_copy(value2, value4);
                    /* Then copy stack top to value3 */
                    slot_copy(value3, stack->top - 1);
                    /* Then copy stack second to value4 */
                    slot_copy(value4, stack->top - 2);
                    break;
                }
                case VM_TYPE_CAT_2:
                    // Form 3, same as dup2_x1
                    _stack_dup2_x1();
                    return;
            }
            break;
        }
        case VM_TYPE_CAT_2:
            switch (slot_type_category(value2->type)) {
                case VM_TYPE_CAT_1: {
                    // Test the value3 type
                    stack_peek_value(3);
                    stack_assert_cat(3, VM_TYPE_CAT_1);

                    // Form 2, same as dup_x2
                    _stack_dup_x2();
                    return;
                }
                case VM_TYPE_CAT_2:
                    // Form 4, same as dup_x1
                    _stack_dup_x1();
                    return;
            }
            break;
    }
}

/**
 * Swap the top two operand stack values
 */
static inline void stack_swap(VMOperandStack *stack) {
    stack_peek_value(1);
    stack_assert_cat(1, VM_TYPE_CAT_1);
    stack_peek_value(2);
    stack_assert_cat(2, VM_TYPE_CAT_1);

    slot_swap(value1, value2);
}

#undef _stack_dup2_x1
#undef _stack_dup_x2
#undef _stack_dup_x1
#undef stack_peek_value

#define local_check_index(local) \
    assert(local >= 0 && local < STACK_FRAME.locals.maxLocals)

/**
 * Transfer arguments from caller's operand stack to current method's local slots
 */
static inline void local_transfer_arguments(JavaStackFrame* frame, uint16_t argument_count) {
    JavaStackFrame *parentFrame = (JavaStackFrame *) frame->baseFrame.prev;
    assert(parentFrame->baseFrame.type == VM_STACK_FRAME_JAVA);

    VMOperandStack *stackFrom = &parentFrame->operandStack;
    VMLocals *localsTo = &frame->locals;

    VMStackSlot *stackSlotFromLimit = stackFrom->top;
    VMStackSlot *stackSlotFrom = stackSlotFromLimit - argument_count;
    assert(stackSlotFrom >= stackFrom->slots);
    stackFrom->top = stackSlotFrom; // Pop all arguments from previous stack frame

    int localIndex = 0;
    while (stackSlotFrom < stackSlotFromLimit) {
        switch (slot_type_category(stackSlotFrom->type)) {
            case VM_TYPE_CAT_1:
                assert(localIndex < localsTo->maxLocals);
                localsTo->slots[localIndex].type = stackSlotFrom->type;
                localsTo->slots[localIndex].data = stackSlotFrom->data;
                localIndex++;
                break;
            case VM_TYPE_CAT_2:
                assert(localIndex + 1 < localsTo->maxLocals);
                localsTo->slots[localIndex].type = stackSlotFrom->type;
                localsTo->slots[localIndex].data = stackSlotFrom->data;
                localIndex++;
                // Cat2 use 2 local slots
                localsTo->slots[localIndex].type = VM_SLOT_INVALID;
                localIndex++;
                break;
        }
        stackSlotFrom->type = VM_SLOT_INVALID;

        stackSlotFrom++;
    }
}

#endif //FOXVM_VM_STACK_H
