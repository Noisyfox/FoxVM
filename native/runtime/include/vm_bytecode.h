//
// Created by noisyfox on 2020/5/10.
//

#ifndef FOXVM_VM_BYTECODE_H
#define FOXVM_VM_BYTECODE_H

#include "vm_stack.h"

/**
 * Nop instruction
 */
#define bc_nop() do {} while(0)

/**
 * Instruction `aconst_null`.
 * Push the `null` object reference onto the operand stack.
 */
#define bc_aconst_null() stack_push_object(JAVA_NULL)

/**
 * Push int constant onto the top of the operand stack
 */
#define bc_iconst_m1() stack_push_int(-1)
#define bc_iconst_0()  stack_push_int(0)
#define bc_iconst_1()  stack_push_int(1)
#define bc_iconst_2()  stack_push_int(2)
#define bc_iconst_3()  stack_push_int(3)
#define bc_iconst_4()  stack_push_int(4)
#define bc_iconst_5()  stack_push_int(5)

/**
 * Push long constant onto the top of the operand stack
 */
#define bc_lconst_0() stack_push_long(0)
#define bc_lconst_1() stack_push_long(1)

/**
 * Push float constant onto the top of the operand stack
 */
#define bc_fconst_0() stack_push_float(0.0f)
#define bc_fconst_1() stack_push_float(1.0f)
#define bc_fconst_2() stack_push_float(2.0f)

/**
 * Push double constant onto the top of the operand stack
 */
#define bc_dconst_0() stack_push_double(0.0)
#define bc_dconst_1() stack_push_double(1.0)

/**
 * Pop the top of the operand stack into given local variable
 */
#define bc_store(local, required_type) do {                                                             \
    switch (slot_type_category(required_type)) {                                                        \
        case VM_TYPE_CAT_1:                                                                             \
            local_check_index(local);                                                                   \
            stack_pop_to(&__stackFrame.operandStack, &__stackFrame.locals.slots[local], required_type); \
            break;                                                                                      \
        case VM_TYPE_CAT_2:                                                                             \
            local_check_index(local);                                                                   \
            local_check_index(local + 1);                                                               \
            stack_pop_to(&__stackFrame.operandStack, &__stackFrame.locals.slots[local], required_type); \
            /* Cat2 use 2 local slots */                                                                \
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

/**
 * Pop the top value from the operand stack.
 * The pop instruction must not be used unless value is a value of a
 * category 1 computational type
 */
#define bc_pop() stack_pop(&__stackFrame.operandStack)
/**
 * Pop the top one or two values from the operand stack.
 */
#define bc_pop2() stack_pop2(&__stackFrame.operandStack)

// Variants of the dup instructions
#define bc_dup()     stack_dup(&__stackFrame.operandStack)
#define bc_dup_x1()  stack_dup_x1(&__stackFrame.operandStack)
#define bc_dup_x2()  stack_dup_x2(&__stackFrame.operandStack)
#define bc_dup2()    stack_dup2(&__stackFrame.operandStack)
#define bc_dup2_x1() stack_dup2_x1(&__stackFrame.operandStack)
#define bc_dup2_x2() stack_dup2_x2(&__stackFrame.operandStack)

/** Swap the top two operand stack values */
#define bc_swap() stack_swap(&__stackFrame.operandStack)

// FoxVM specific instructions

/** Record current source file line number. */
#define bc_line(line_number) __stackFrame.currentLine = (line_number)

#endif //FOXVM_VM_BYTECODE_H
