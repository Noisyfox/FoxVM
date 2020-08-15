//
// Created by noisyfox on 2020/5/10.
//

#ifndef FOXVM_VM_BYTECODE_H
#define FOXVM_VM_BYTECODE_H

#include "vm_stack.h"


/**
 * Instruction `aconst_null`.
 * Push the `null` object reference onto the operand stack.
 */
#define bc_aconst_null() stack_push_object(JAVA_NULL)

/**
 * Pop the top of the operand stack into given local variable
 */
#define bc_store(local, required_type) do {                                                             \
    switch (required_type) {                                                                            \
        case VM_SLOT_INVALID:                                                                           \
            assert(!"Invalid slot type");                                                               \
            break;                                                                                      \
        case VM_SLOT_OBJECT:                                                                            \
        case VM_SLOT_INT:                                                                               \
        case VM_SLOT_FLOAT:                                                                             \
            local_check_index(local);                                                                   \
            stack_pop_to(&__stackFrame.operandStack, &__stackFrame.locals.slots[local], required_type); \
            break;                                                                                      \
        case VM_SLOT_LONG:                                                                              \
        case VM_SLOT_DOUBLE:                                                                            \
            local_check_index(local);                                                                   \
            local_check_index(local + 1);                                                               \
            stack_pop_to(&__stackFrame.operandStack, &__stackFrame.locals.slots[local], required_type); \
            /* Long and Double use 2 local slots */                                                     \
            __stackFrame.locals.slots[local + 1].type = VM_SLOT_INVALID;                                \
            break;                                                                                      \
    }                                                                                                   \
} while(0)

// Variants of the store instructions
#define bc_istore(local) bc_store(local, VM_SLOT_INT)
#define bc_lstore(local) bc_store(local, VM_SLOT_LONG)
#define bc_fstore(local) bc_store(local, VM_SLOT_FLOAT)
#define bc_dstore(local) bc_store(local, VM_SLOT_DOUBLE)
#define bc_astore(local) bc_store(local, VM_SLOT_OBJECT)

/**
 * Push the given local variable on to the top of the operand stack
 */
#define bc_load(local, required_type) do {                                                          \
    local_check_index(local);                                                                       \
    stack_push_from(&__stackFrame.operandStack, &__stackFrame.locals.slots[local], required_type);  \
} while(0)

// Variants of the load instructions
#define bc_iload(local) bc_load(local, VM_SLOT_INT)
#define bc_lload(local) bc_load(local, VM_SLOT_LONG)
#define bc_fload(local) bc_load(local, VM_SLOT_FLOAT)
#define bc_dload(local) bc_load(local, VM_SLOT_DOUBLE)
#define bc_aload(local) bc_load(local, VM_SLOT_OBJECT)

/**
 * Push the given value onto the top of the operand stack, sign-extended to a JAVA_INT
 */
#define bc_bipush(i) stack_push_int(i)
#define bc_sipush(i) stack_push_int(i)

// FoxVM specific instructions

/** Record current source file line number. */
#define bc_line(line_number) __stackFrame.currentLine = (line_number)

#endif //FOXVM_VM_BYTECODE_H
