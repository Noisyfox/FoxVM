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
 * Iinc instruction
 *
 * Increment local variable by [amount]
 */
#define bc_iinc(local, amount) do {                 \
    local_check_index(local);                       \
    assert(local_of(local).type == VM_SLOT_INT);    \
                                                    \
    local_of(local).data.i += amount;               \
} while(0)


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
#define bc_store(local, required_type) do {                            \
    switch (slot_type_category(required_type)) {                       \
        case VM_TYPE_CAT_1:                                            \
            local_check_index(local);                                  \
            stack_pop_to(OP_STACK, &local_of(local), required_type);   \
            break;                                                     \
        case VM_TYPE_CAT_2:                                            \
            local_check_index(local);                                  \
            local_check_index(local + 1);                              \
            stack_pop_to(OP_STACK, &local_of(local), required_type);   \
            /* Cat2 use 2 local slots */                               \
            local_of(local + 1).type = VM_SLOT_INVALID;                \
            break;                                                     \
    }                                                                  \
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
#define bc_load(local, required_type) do {                         \
    local_check_index(local);                                      \
    stack_push_from(OP_STACK, &local_of(local), required_type);    \
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
#define bc_pop() stack_pop(OP_STACK)
/**
 * Pop the top one or two values from the operand stack.
 */
#define bc_pop2() stack_pop2(OP_STACK)

// Variants of the dup instructions
#define bc_dup()     stack_dup(OP_STACK)
#define bc_dup_x1()  stack_dup_x1(OP_STACK)
#define bc_dup_x2()  stack_dup_x2(OP_STACK)
#define bc_dup2()    stack_dup2(OP_STACK)
#define bc_dup2_x1() stack_dup2_x1(OP_STACK)
#define bc_dup2_x2() stack_dup2_x2(OP_STACK)

/** Swap the top two operand stack values */
#define bc_swap() stack_swap(OP_STACK)

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

#define bc_iadd() bc_arithmetic_iadd(OP_STACK)
#define bc_ladd() bc_arithmetic_ladd(OP_STACK)
#define bc_fadd() bc_arithmetic_fadd(OP_STACK)
#define bc_dadd() bc_arithmetic_dadd(OP_STACK)

#define bc_isub() bc_arithmetic_isub(OP_STACK)
#define bc_lsub() bc_arithmetic_lsub(OP_STACK)
#define bc_fsub() bc_arithmetic_fsub(OP_STACK)
#define bc_dsub() bc_arithmetic_dsub(OP_STACK)

#define bc_imul() bc_arithmetic_imul(OP_STACK)
#define bc_lmul() bc_arithmetic_lmul(OP_STACK)
#define bc_fmul() bc_arithmetic_fmul(OP_STACK)
#define bc_dmul() bc_arithmetic_dmul(OP_STACK)

#define bc_idiv() bc_arithmetic_idiv(OP_STACK)
#define bc_ldiv() bc_arithmetic_ldiv(OP_STACK)
#define bc_fdiv() bc_arithmetic_fdiv(OP_STACK)
#define bc_ddiv() bc_arithmetic_ddiv(OP_STACK)

#define bc_irem() bc_arithmetic_irem(OP_STACK)
#define bc_lrem() bc_arithmetic_lrem(OP_STACK)
#define bc_frem() bc_arithmetic_frem(OP_STACK)
#define bc_drem() bc_arithmetic_drem(OP_STACK)

#define bc_ineg() bc_arithmetic_ineg(OP_STACK)
#define bc_lneg() bc_arithmetic_lneg(OP_STACK)
#define bc_fneg() bc_arithmetic_fneg(OP_STACK)
#define bc_dneg() bc_arithmetic_dneg(OP_STACK)

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

#define bc_ishl() bc_arithmetic_ishl(OP_STACK)
#define bc_lshl() bc_arithmetic_lshl(OP_STACK)

#define bc_ishr() bc_arithmetic_ishr(OP_STACK)
#define bc_lshr() bc_arithmetic_lshr(OP_STACK)

#define bc_iushr() bc_arithmetic_iushr(OP_STACK)
#define bc_lushr() bc_arithmetic_lushr(OP_STACK)

#define bc_iand() bc_arithmetic_iand(OP_STACK)
#define bc_land() bc_arithmetic_land(OP_STACK)

#define bc_ior() bc_arithmetic_ior(OP_STACK)
#define bc_lor() bc_arithmetic_lor(OP_STACK)

#define bc_ixor() bc_arithmetic_ixor(OP_STACK)
#define bc_lxor() bc_arithmetic_lxor(OP_STACK)

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

#define bc_i2l() bc_arithmetic_i2l(OP_STACK)
#define bc_i2f() bc_arithmetic_i2f(OP_STACK)
#define bc_i2d() bc_arithmetic_i2d(OP_STACK)
#define bc_l2i() bc_arithmetic_l2i(OP_STACK)
#define bc_l2f() bc_arithmetic_l2f(OP_STACK)
#define bc_l2d() bc_arithmetic_l2d(OP_STACK)
#define bc_f2i() bc_arithmetic_f2i(OP_STACK)
#define bc_f2l() bc_arithmetic_f2l(OP_STACK)
#define bc_f2d() bc_arithmetic_f2d(OP_STACK)
#define bc_d2i() bc_arithmetic_d2i(OP_STACK)
#define bc_d2l() bc_arithmetic_d2l(OP_STACK)
#define bc_d2f() bc_arithmetic_d2f(OP_STACK)
#define bc_i2b() bc_arithmetic_i2b(OP_STACK)
#define bc_i2c() bc_arithmetic_i2c(OP_STACK)
#define bc_i2s() bc_arithmetic_i2s(OP_STACK)

// Comparison operations
decl_arithmetic_func(lcmp);
decl_arithmetic_func(fcmpl);
decl_arithmetic_func(fcmpg);
decl_arithmetic_func(dcmpl);
decl_arithmetic_func(dcmpg);

#define bc_lcmp() bc_arithmetic_lcmp(OP_STACK)
#define bc_fcmpl() bc_arithmetic_fcmpl(OP_STACK)
#define bc_fcmpg() bc_arithmetic_fcmpg(OP_STACK)
#define bc_dcmpl() bc_arithmetic_dcmpl(OP_STACK)
#define bc_dcmpg() bc_arithmetic_dcmpg(OP_STACK)

// Branch operations
#define decl_branch_func(name) JAVA_BOOLEAN bc_branch_##name(VMOperandStack *stack)

decl_branch_func(ifeq);
decl_branch_func(ifne);
decl_branch_func(iflt);
decl_branch_func(ifle);
decl_branch_func(ifgt);
decl_branch_func(ifge);

decl_branch_func(if_icmpeq);
decl_branch_func(if_icmpne);
decl_branch_func(if_icmplt);
decl_branch_func(if_icmple);
decl_branch_func(if_icmpgt);
decl_branch_func(if_icmpge);

decl_branch_func(if_acmpeq);
decl_branch_func(if_acmpne);

decl_branch_func(ifnull);

#define BC_BRANCH_IMPL(name, label) if (bc_branch_##name(OP_STACK)) goto label

#define bc_ifeq(label) BC_BRANCH_IMPL(ifeq, label)
#define bc_ifne(label) BC_BRANCH_IMPL(ifne, label)
#define bc_iflt(label) BC_BRANCH_IMPL(iflt, label)
#define bc_ifle(label) BC_BRANCH_IMPL(ifle, label)
#define bc_ifgt(label) BC_BRANCH_IMPL(ifgt, label)
#define bc_ifge(label) BC_BRANCH_IMPL(ifge, label)

#define bc_if_icmpeq(label) BC_BRANCH_IMPL(if_icmpeq, label)
#define bc_if_icmpne(label) BC_BRANCH_IMPL(if_icmpne, label)
#define bc_if_icmplt(label) BC_BRANCH_IMPL(if_icmplt, label)
#define bc_if_icmple(label) BC_BRANCH_IMPL(if_icmple, label)
#define bc_if_icmpgt(label) BC_BRANCH_IMPL(if_icmpgt, label)
#define bc_if_icmpge(label) BC_BRANCH_IMPL(if_icmpge, label)

#define bc_if_acmpeq(label) BC_BRANCH_IMPL(if_acmpeq, label)
#define bc_if_acmpne(label) BC_BRANCH_IMPL(if_acmpne, label)

#define bc_ifnull(label)    if (bc_branch_ifnull(OP_STACK))  goto label
#define bc_ifnonnull(label) if (!bc_branch_ifnull(OP_STACK)) goto label

#define bc_goto(label)  goto label

// FoxVM specific instructions

/** Record current source file line number. */
#define bc_line(line_number) STACK_FRAME.currentLine = (line_number)

#endif //FOXVM_VM_BYTECODE_H
