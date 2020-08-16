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

// Variants of the arithmetic instructions
#define decl_arithmetic_func(n) void bc_arithmetic_##n(VMOperandStack *stack)
#define decl_arithmetic_func_prefix_all(n) \
decl_arithmetic_func(i##n);\
decl_arithmetic_func(l##n);\
decl_arithmetic_func(f##n);\
decl_arithmetic_func(d##n)

decl_arithmetic_func_prefix_all(add);
decl_arithmetic_func_prefix_all(sub);
decl_arithmetic_func_prefix_all(mul);
decl_arithmetic_func_prefix_all(div);
decl_arithmetic_func_prefix_all(rem);
decl_arithmetic_func_prefix_all(neg);

#define bc_iadd() bc_arithmetic_iadd(&__stackFrame.operandStack)
#define bc_ladd() bc_arithmetic_ladd(&__stackFrame.operandStack)
#define bc_fadd() bc_arithmetic_fadd(&__stackFrame.operandStack)
#define bc_dadd() bc_arithmetic_dadd(&__stackFrame.operandStack)

#define bc_isub() bc_arithmetic_isub(&__stackFrame.operandStack)
#define bc_lsub() bc_arithmetic_lsub(&__stackFrame.operandStack)
#define bc_fsub() bc_arithmetic_fsub(&__stackFrame.operandStack)
#define bc_dsub() bc_arithmetic_dsub(&__stackFrame.operandStack)

#define bc_imul() bc_arithmetic_imul(&__stackFrame.operandStack)
#define bc_lmul() bc_arithmetic_lmul(&__stackFrame.operandStack)
#define bc_fmul() bc_arithmetic_fmul(&__stackFrame.operandStack)
#define bc_dmul() bc_arithmetic_dmul(&__stackFrame.operandStack)

#define bc_idiv() bc_arithmetic_idiv(&__stackFrame.operandStack)
#define bc_ldiv() bc_arithmetic_ldiv(&__stackFrame.operandStack)
#define bc_fdiv() bc_arithmetic_fdiv(&__stackFrame.operandStack)
#define bc_ddiv() bc_arithmetic_ddiv(&__stackFrame.operandStack)

#define bc_irem() bc_arithmetic_irem(&__stackFrame.operandStack)
#define bc_lrem() bc_arithmetic_lrem(&__stackFrame.operandStack)
#define bc_frem() bc_arithmetic_frem(&__stackFrame.operandStack)
#define bc_drem() bc_arithmetic_drem(&__stackFrame.operandStack)

#define bc_ineg() bc_arithmetic_ineg(&__stackFrame.operandStack)
#define bc_lneg() bc_arithmetic_lneg(&__stackFrame.operandStack)
#define bc_fneg() bc_arithmetic_fneg(&__stackFrame.operandStack)
#define bc_dneg() bc_arithmetic_dneg(&__stackFrame.operandStack)

// Variants of the bitwise instructions
#define decl_bitwise_func_prefix_all(n) \
decl_arithmetic_func(i##n);\
decl_arithmetic_func(l##n)

decl_bitwise_func_prefix_all(shl);
decl_bitwise_func_prefix_all(shr);
decl_bitwise_func_prefix_all(ushr);
decl_bitwise_func_prefix_all(and);
decl_bitwise_func_prefix_all(or);
decl_bitwise_func_prefix_all(xor);

#define bc_ishl() bc_arithmetic_ishl(&__stackFrame.operandStack)
#define bc_lshl() bc_arithmetic_lshl(&__stackFrame.operandStack)

#define bc_ishr() bc_arithmetic_ishr(&__stackFrame.operandStack)
#define bc_lshr() bc_arithmetic_lshr(&__stackFrame.operandStack)

#define bc_iushr() bc_arithmetic_iushr(&__stackFrame.operandStack)
#define bc_lushr() bc_arithmetic_lushr(&__stackFrame.operandStack)

#define bc_iand() bc_arithmetic_iand(&__stackFrame.operandStack)
#define bc_land() bc_arithmetic_land(&__stackFrame.operandStack)

#define bc_ior() bc_arithmetic_ior(&__stackFrame.operandStack)
#define bc_lor() bc_arithmetic_lor(&__stackFrame.operandStack)

#define bc_ixor() bc_arithmetic_ixor(&__stackFrame.operandStack)
#define bc_lxor() bc_arithmetic_lxor(&__stackFrame.operandStack)

// Type conversion instructions
decl_arithmetic_func(i2l);
decl_arithmetic_func(i2f);
decl_arithmetic_func(i2d);
decl_arithmetic_func(l2i);
decl_arithmetic_func(l2f);
decl_arithmetic_func(l2d);
decl_arithmetic_func(f2i);
decl_arithmetic_func(f2l);
decl_arithmetic_func(f2d);
decl_arithmetic_func(d2i);
decl_arithmetic_func(d2l);
decl_arithmetic_func(d2f);
decl_arithmetic_func(i2b);
decl_arithmetic_func(i2c);
decl_arithmetic_func(i2s);

#define bc_i2l() bc_arithmetic_i2l(&__stackFrame.operandStack)
#define bc_i2f() bc_arithmetic_i2f(&__stackFrame.operandStack)
#define bc_i2d() bc_arithmetic_i2d(&__stackFrame.operandStack)
#define bc_l2i() bc_arithmetic_l2i(&__stackFrame.operandStack)
#define bc_l2f() bc_arithmetic_l2f(&__stackFrame.operandStack)
#define bc_l2d() bc_arithmetic_l2d(&__stackFrame.operandStack)
#define bc_f2i() bc_arithmetic_f2i(&__stackFrame.operandStack)
#define bc_f2l() bc_arithmetic_f2l(&__stackFrame.operandStack)
#define bc_f2d() bc_arithmetic_f2d(&__stackFrame.operandStack)
#define bc_d2i() bc_arithmetic_d2i(&__stackFrame.operandStack)
#define bc_d2l() bc_arithmetic_d2l(&__stackFrame.operandStack)
#define bc_d2f() bc_arithmetic_d2f(&__stackFrame.operandStack)
#define bc_i2b() bc_arithmetic_i2b(&__stackFrame.operandStack)
#define bc_i2c() bc_arithmetic_i2c(&__stackFrame.operandStack)
#define bc_i2s() bc_arithmetic_i2s(&__stackFrame.operandStack)

// FoxVM specific instructions

/** Record current source file line number. */
#define bc_line(line_number) __stackFrame.currentLine = (line_number)

#endif //FOXVM_VM_BYTECODE_H
