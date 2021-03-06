//
// Created by noisyfox on 2020/5/10.
//

#ifndef FOXVM_VM_BYTECODE_H
#define FOXVM_VM_BYTECODE_H

#include "vm_stack.h"
#include "vm_exception.h"
#include "jni.h"

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

// Comparison instructions
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

// Branch instructions
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

// tableswitch and lookupswitch are both implemented by C switch-case statement
JAVA_INT bc_switch_get_index(VMOperandStack *stack);
#define bc_switch() switch(bc_switch_get_index(OP_STACK))

// ldc instructions
#define bc_ldc_int(v)    stack_push_int(v)
#define bc_ldc_long(v)   stack_push_long(v)
#define bc_ldc_float(v)  stack_push_float(v)
#define bc_ldc_double(v) stack_push_double(v)
JAVA_OBJECT bc_ldc_class_obj(VM_PARAM_CURRENT_CONTEXT, JavaStackFrame *frame, C_CSTR class_name);
#define bc_ldc_class(class_name) do {JAVA_OBJECT obj = bc_ldc_class_obj(vmCurrentContext, &STACK_FRAME, class_name); stack_push_object(obj);} while(0)
JAVA_OBJECT bc_ldc_string_const(VM_PARAM_CURRENT_CONTEXT, JAVA_INT constant_index);
#define bc_ldc_string(constant_index) do { JAVA_OBJECT str = bc_ldc_string_const(vmCurrentContext, constant_index); stack_push_object(str);} while(0)

// new instruction
JAVA_OBJECT bc_create_instance(VM_PARAM_CURRENT_CONTEXT, JavaStackFrame *frame, JavaClassInfo *info);
#define bc_new(class_info) do {JAVA_OBJECT obj = bc_create_instance(vmCurrentContext, &STACK_FRAME, class_info); stack_push_object(obj);} while(0)

// invokeXXXX instructions
#define bc_invoke_special(fp)                          ((JavaMethodRetVoid)    fp)(vmCurrentContext)
#define bc_invoke_special_z(fp) do {JAVA_BOOLEAN ret = ((JavaMethodRetBoolean) fp)(vmCurrentContext); stack_push_int(ret);                } while(0)
#define bc_invoke_special_c(fp) do {JAVA_CHAR    ret = ((JavaMethodRetChar)    fp)(vmCurrentContext); stack_push_int((JAVA_UCHAR)ret);    } while(0)
#define bc_invoke_special_b(fp) do {JAVA_BYTE    ret = ((JavaMethodRetByte)    fp)(vmCurrentContext); stack_push_int(ret);                } while(0)
#define bc_invoke_special_s(fp) do {JAVA_SHORT   ret = ((JavaMethodRetShort)   fp)(vmCurrentContext); stack_push_int(ret);                } while(0)
#define bc_invoke_special_i(fp) do {JAVA_INT     ret = ((JavaMethodRetInt)     fp)(vmCurrentContext); stack_push_int(ret);                } while(0)
#define bc_invoke_special_f(fp) do {JAVA_FLOAT   ret = ((JavaMethodRetFloat)   fp)(vmCurrentContext); stack_push_float(ret);              } while(0)
#define bc_invoke_special_l(fp) do {JAVA_LONG    ret = ((JavaMethodRetLong)    fp)(vmCurrentContext); stack_push_long(ret);               } while(0)
#define bc_invoke_special_d(fp) do {JAVA_DOUBLE  ret = ((JavaMethodRetDouble)  fp)(vmCurrentContext); stack_push_double(ret);             } while(0)
#define bc_invoke_special_a(fp) do {JAVA_ARRAY   ret = ((JavaMethodRetArray)   fp)(vmCurrentContext); stack_push_object((JAVA_OBJECT)ret);} while(0)
#define bc_invoke_special_o(fp) do {JAVA_OBJECT  ret = ((JavaMethodRetObject)  fp)(vmCurrentContext); stack_push_object(ret);             } while(0)

JAVA_VOID bc_resolve_class(VM_PARAM_CURRENT_CONTEXT, JavaStackFrame *frame, JavaClassInfo *classInfo,
                           JAVA_CLASS *classRefOut);
#define bc_invoke_static_prepare(class_info)                                    \
    JAVA_CLASS classRef;                                                        \
    bc_resolve_class(vmCurrentContext, &STACK_FRAME, class_info, &classRef);    \
    vmCurrentContext->callingClass = classRef
#define bc_invoke_static(class_info,   fp) do {bc_invoke_static_prepare(class_info);                    ((JavaMethodRetVoid)    fp)(vmCurrentContext);                                     } while(0)
#define bc_invoke_static_z(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_BOOLEAN ret = ((JavaMethodRetBoolean) fp)(vmCurrentContext); stack_push_int(ret);                } while(0)
#define bc_invoke_static_c(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_CHAR    ret = ((JavaMethodRetChar)    fp)(vmCurrentContext); stack_push_int((JAVA_UCHAR)ret);    } while(0)
#define bc_invoke_static_b(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_BYTE    ret = ((JavaMethodRetByte)    fp)(vmCurrentContext); stack_push_int(ret);                } while(0)
#define bc_invoke_static_s(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_SHORT   ret = ((JavaMethodRetShort)   fp)(vmCurrentContext); stack_push_int(ret);                } while(0)
#define bc_invoke_static_i(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_INT     ret = ((JavaMethodRetInt)     fp)(vmCurrentContext); stack_push_int(ret);                } while(0)
#define bc_invoke_static_f(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_FLOAT   ret = ((JavaMethodRetFloat)   fp)(vmCurrentContext); stack_push_float(ret);              } while(0)
#define bc_invoke_static_l(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_LONG    ret = ((JavaMethodRetLong)    fp)(vmCurrentContext); stack_push_long(ret);               } while(0)
#define bc_invoke_static_d(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_DOUBLE  ret = ((JavaMethodRetDouble)  fp)(vmCurrentContext); stack_push_double(ret);             } while(0)
#define bc_invoke_static_a(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_ARRAY   ret = ((JavaMethodRetArray)   fp)(vmCurrentContext); stack_push_object((JAVA_OBJECT)ret);} while(0)
#define bc_invoke_static_o(class_info, fp) do {bc_invoke_static_prepare(class_info); JAVA_OBJECT  ret = ((JavaMethodRetObject)  fp)(vmCurrentContext); stack_push_object(ret);             } while(0)

void *bc_vtable_lookup(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, JAVA_INT argument_count, JavaClassInfo* clazz, uint16_t vtable_index);
#define bc_invoke_virtual(argument_count,   clazz, vtable_index) bc_invoke_special(  bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_z(argument_count, clazz, vtable_index) bc_invoke_special_z(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_c(argument_count, clazz, vtable_index) bc_invoke_special_c(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_b(argument_count, clazz, vtable_index) bc_invoke_special_b(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_s(argument_count, clazz, vtable_index) bc_invoke_special_s(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_i(argument_count, clazz, vtable_index) bc_invoke_special_i(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_f(argument_count, clazz, vtable_index) bc_invoke_special_f(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_l(argument_count, clazz, vtable_index) bc_invoke_special_l(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_d(argument_count, clazz, vtable_index) bc_invoke_special_d(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_a(argument_count, clazz, vtable_index) bc_invoke_special_a(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))
#define bc_invoke_virtual_o(argument_count, clazz, vtable_index) bc_invoke_special_o(bc_vtable_lookup(vmCurrentContext, OP_STACK, argument_count, clazz, vtable_index))

void *bc_ivtable_lookup(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, JAVA_INT argument_count, JavaClassInfo* interface_type, uint16_t method_index);
#define bc_invoke_interface(argument_count,   interface_type, method_index) bc_invoke_special(  bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_z(argument_count, interface_type, method_index) bc_invoke_special_z(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_c(argument_count, interface_type, method_index) bc_invoke_special_c(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_b(argument_count, interface_type, method_index) bc_invoke_special_b(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_s(argument_count, interface_type, method_index) bc_invoke_special_s(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_i(argument_count, interface_type, method_index) bc_invoke_special_i(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_f(argument_count, interface_type, method_index) bc_invoke_special_f(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_l(argument_count, interface_type, method_index) bc_invoke_special_l(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_d(argument_count, interface_type, method_index) bc_invoke_special_d(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_a(argument_count, interface_type, method_index) bc_invoke_special_a(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))
#define bc_invoke_interface_o(argument_count, interface_type, method_index) bc_invoke_special_o(bc_ivtable_lookup(vmCurrentContext, OP_STACK, argument_count, interface_type, method_index))

// field instructions
JAVA_VOID bc_putfield(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, JavaClassInfo* clazz, uint16_t field_index, JAVA_OBJECT *objRefOut, void *valueOut, BasicType fieldType);
#define bc_do_putfield(clazz, field_index, object_type, field_name, field_type)                                 \
    JAVA_OBJECT objectRef;                                                                                      \
    JAVA_##field_type value;                                                                                    \
    bc_putfield(vmCurrentContext, OP_STACK, clazz, field_index, &objectRef, &value, VM_TYPE_##field_type);      \
    ((object_type*)objectRef)->field_name = value
#define bc_putfield_z(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, BOOLEAN);} while(0)
#define bc_putfield_c(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, CHAR);   } while(0)
#define bc_putfield_b(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, BYTE);   } while(0)
#define bc_putfield_s(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, SHORT);  } while(0)
#define bc_putfield_i(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, INT);    } while(0)
#define bc_putfield_f(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, FLOAT);  } while(0)
#define bc_putfield_l(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, LONG);   } while(0)
#define bc_putfield_d(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, DOUBLE); } while(0)
// TODO: update card table for cross-gen reference
#define bc_putfield_a(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, ARRAY);  } while(0)
#define bc_putfield_o(clazz, field_index, object_type, field_name) do {bc_do_putfield(clazz, field_index, object_type, field_name, OBJECT); } while(0)

JAVA_OBJECT bc_getfield(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, JavaClassInfo* clazz, uint16_t field_index);
#define bc_do_getfield(clazz, field_index, object_type, field_name, field_type)                                 \
    JAVA_OBJECT objectRef = bc_getfield(vmCurrentContext, OP_STACK, clazz, field_index);                        \
    JAVA_##field_type value = ((object_type*)objectRef)->field_name
#define bc_getfield_z(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, BOOLEAN); stack_push_int(value);                } while(0)
#define bc_getfield_c(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, CHAR);    stack_push_int((JAVA_UCHAR)value);    } while(0)
#define bc_getfield_b(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, BYTE);    stack_push_int(value);                } while(0)
#define bc_getfield_s(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, SHORT);   stack_push_int(value);                } while(0)
#define bc_getfield_i(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, INT);     stack_push_int(value);                } while(0)
#define bc_getfield_f(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, FLOAT);   stack_push_float(value);              } while(0)
#define bc_getfield_l(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, LONG);    stack_push_long(value);               } while(0)
#define bc_getfield_d(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, DOUBLE);  stack_push_double(value);             } while(0)
#define bc_getfield_a(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, ARRAY);   stack_push_object((JAVA_OBJECT)value);} while(0)
#define bc_getfield_o(clazz, field_index, object_type, field_name) do {bc_do_getfield(clazz, field_index, object_type, field_name, OBJECT);  stack_push_object(value);             } while(0)

JAVA_VOID bc_putstatic(VM_PARAM_CURRENT_CONTEXT, JavaStackFrame *frame, JavaClassInfo *classInfo,
                       JAVA_CLASS *classRefOut, void *valueOut, BasicType fieldType);
#define bc_do_putstatic(class_info, class_type, field_name, field_type)                                 \
    JAVA_CLASS classRef;                                                                                \
    JAVA_##field_type value;                                                                            \
    bc_putstatic(vmCurrentContext, &STACK_FRAME, class_info, &classRef, &value, VM_TYPE_##field_type);  \
    ((class_type*)classRef)->field_name = value
#define bc_putstatic_z(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, BOOLEAN);} while(0)
#define bc_putstatic_c(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, CHAR);   } while(0)
#define bc_putstatic_b(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, BYTE);   } while(0)
#define bc_putstatic_s(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, SHORT);  } while(0)
#define bc_putstatic_i(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, INT);    } while(0)
#define bc_putstatic_f(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, FLOAT);  } while(0)
#define bc_putstatic_l(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, LONG);   } while(0)
#define bc_putstatic_d(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, DOUBLE); } while(0)
// TODO: update card table for cross-gen reference
#define bc_putstatic_a(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, ARRAY);  } while(0)
#define bc_putstatic_o(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, OBJECT); } while(0)

#define bc_do_getstatic(class_info, class_type, field_name, field_type)         \
    JAVA_CLASS classRef;                                                        \
    bc_resolve_class(vmCurrentContext, &STACK_FRAME, class_info, &classRef);    \
    JAVA_##field_type value = ((class_type*)classRef)->field_name
#define bc_getstatic_z(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, BOOLEAN); stack_push_int(value);                } while(0)
#define bc_getstatic_c(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, CHAR);    stack_push_int((JAVA_UCHAR)value);    } while(0)
#define bc_getstatic_b(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, BYTE);    stack_push_int(value);                } while(0)
#define bc_getstatic_s(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, SHORT);   stack_push_int(value);                } while(0)
#define bc_getstatic_i(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, INT);     stack_push_int(value);                } while(0)
#define bc_getstatic_f(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, FLOAT);   stack_push_float(value);              } while(0)
#define bc_getstatic_l(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, LONG);    stack_push_long(value);               } while(0)
#define bc_getstatic_d(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, DOUBLE);  stack_push_double(value);             } while(0)
#define bc_getstatic_a(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, ARRAY);   stack_push_object((JAVA_OBJECT)value);} while(0)
#define bc_getstatic_o(class_info, class_type, field_name) do {bc_do_getstatic(class_info, class_type, field_name, OBJECT);  stack_push_object(value);             } while(0)

// array related instructions
JAVA_ARRAY bc_new_array(VM_PARAM_CURRENT_CONTEXT, JavaStackFrame *frame, C_CSTR desc);
#define bc_newarray(desc) do {JAVA_ARRAY array = bc_new_array(vmCurrentContext, &STACK_FRAME, desc); stack_push_object((JAVA_OBJECT)array);} while(0)

JAVA_INT bc_array_length(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack);
#define bc_arraylength() do {JAVA_INT len = bc_array_length(vmCurrentContext, OP_STACK); stack_push_int(len);} while(0)

JAVA_VOID bc_array_load(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, void *valueOut, BasicType fieldType);
#define bc_do_array_load(field_type)                        \
    JAVA_##field_type value;                                \
    bc_array_load(vmCurrentContext, OP_STACK, &value, VM_TYPE_##field_type)
// For caload the char value is zero-extended to an int value, so here we need a unsigned cast,
// otherwise it will be sign-extended.
#define bc_caload() do {bc_do_array_load(CHAR);   stack_push_int((JAVA_UCHAR)value);   } while(0)
#define bc_baload() do {bc_do_array_load(BYTE);   stack_push_int(value);   } while(0)
#define bc_saload() do {bc_do_array_load(SHORT);  stack_push_int(value);   } while(0)
#define bc_iaload() do {bc_do_array_load(INT);    stack_push_int(value);   } while(0)
#define bc_faload() do {bc_do_array_load(FLOAT);  stack_push_float(value); } while(0)
#define bc_laload() do {bc_do_array_load(LONG);   stack_push_long(value);  } while(0)
#define bc_daload() do {bc_do_array_load(DOUBLE); stack_push_double(value);} while(0)
#define bc_aaload() do {bc_do_array_load(OBJECT); stack_push_object(value);} while(0)

JAVA_VOID bc_array_store(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, BasicType fieldType);
#define bc_castore() bc_array_store(vmCurrentContext, OP_STACK, VM_TYPE_CHAR)
#define bc_bastore() bc_array_store(vmCurrentContext, OP_STACK, VM_TYPE_BYTE)
#define bc_sastore() bc_array_store(vmCurrentContext, OP_STACK, VM_TYPE_SHORT)
#define bc_iastore() bc_array_store(vmCurrentContext, OP_STACK, VM_TYPE_INT)
#define bc_fastore() bc_array_store(vmCurrentContext, OP_STACK, VM_TYPE_FLOAT)
#define bc_lastore() bc_array_store(vmCurrentContext, OP_STACK, VM_TYPE_LONG)
#define bc_dastore() bc_array_store(vmCurrentContext, OP_STACK, VM_TYPE_DOUBLE)
#define bc_aastore() bc_array_store(vmCurrentContext, OP_STACK, VM_TYPE_OBJECT)

// Monitor instructions
JAVA_VOID bc_monitor_enter(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack);
#define bc_monitorenter() bc_monitor_enter(vmCurrentContext, OP_STACK)
JAVA_VOID bc_monitor_exit(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack);
#define bc_monitorexit() bc_monitor_exit(vmCurrentContext, OP_STACK)

// Return instructions
JAVA_VOID bc_read_stack_top(VMOperandStack *stack, void *valueOut, BasicType requiredType);
#define bc_return() stack_frame_end(); return
#define bc_do_return(field_type) do {                           \
    JAVA_##field_type value;                                    \
    bc_read_stack_top(OP_STACK, &value, VM_TYPE_##field_type);  \
    stack_frame_end();                                          \
    return value;                                               \
} while(0)
#define bc_zreturn() bc_do_return(BOOLEAN)
#define bc_creturn() bc_do_return(CHAR)
#define bc_breturn() bc_do_return(BYTE)
#define bc_sreturn() bc_do_return(SHORT)
#define bc_ireturn() bc_do_return(INT)
#define bc_freturn() bc_do_return(FLOAT)
#define bc_lreturn() bc_do_return(LONG)
#define bc_dreturn() bc_do_return(DOUBLE)
#define bc_areturn() bc_do_return(ARRAY)
#define bc_oreturn() bc_do_return(OBJECT)

// instanceof instructions
JAVA_BOOLEAN bc_instance_of(VMOperandStack *stack, JavaClassInfo *info);
JAVA_BOOLEAN bc_instance_of_a(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, C_CSTR desc);
#define bc_instanceof(class_info)   do {JAVA_BOOLEAN r = bc_instance_of(OP_STACK, class_info);                     stack_push_int(r);} while(0)
#define bc_instanceof_a(array_desc) do {JAVA_BOOLEAN r = bc_instance_of_a(vmCurrentContext, OP_STACK, array_desc); stack_push_int(r);} while(0)

// checkcast instructions
JAVA_VOID bc_cast(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, JavaClassInfo *info);
JAVA_VOID bc_cast_a(VM_PARAM_CURRENT_CONTEXT, VMOperandStack *stack, C_CSTR desc);
#define bc_checkcast(class_info)   bc_cast(vmCurrentContext, OP_STACK, class_info)
#define bc_checkcast_a(array_desc) bc_cast_a(vmCurrentContext, OP_STACK, array_desc)

// athrow instruction
#define bc_athrow() do {                                \
    JAVA_OBJECT _ex;                                    \
    bc_read_stack_top(OP_STACK, &_ex, VM_TYPE_OBJECT);  \
    exception_set(vmCurrentContext, _ex);               \
    exception_raise(vmCurrentContext);                  \
} while(0)

// FoxVM specific instructions
#define bc_prepare_arguments(argument_count) local_transfer_arguments(&STACK_FRAME, argument_count)

/** Record current source file line number. */
#define bc_line(line_number) STACK_FRAME.currentLine = (line_number)

/** Record current label. */
#define bc_label(label_number) STACK_FRAME.currentLabel = (label_number)

// JNI stack frame helpers
// Setup the native stack frame for the jni call
// Before it enters a native method, the VM automatically ensures that at least 16 local references can be created.
#define bc_native_prepare() native_stack_frame_start(16)
// Mark the current thread that has entered the native call frames, so GC can work on this thread if needed.
// After this is called, before the native frame ended, access of any object must be done via ref handler, because
// the object can be moved by the GC at any time.
// This affectively makes current thread entering the safe region until the call returns.
#define bc_native_start() native_enter_jni(vmCurrentContext);
// End the native stack frame then push the result to the java stack.
// This also switch the state of current thread back to normal.
#define bc_native_end_z(result) do {native_exit_jni(vmCurrentContext); stack_push_int(result);    native_stack_frame_end();} while(0)
#define bc_native_end_c(result) do {native_exit_jni(vmCurrentContext); stack_push_int((JAVA_UCHAR)result);    native_stack_frame_end();} while(0)
#define bc_native_end_b(result) do {native_exit_jni(vmCurrentContext); stack_push_int(result);    native_stack_frame_end();} while(0)
#define bc_native_end_s(result) do {native_exit_jni(vmCurrentContext); stack_push_int(result);    native_stack_frame_end();} while(0)
#define bc_native_end_i(result) do {native_exit_jni(vmCurrentContext); stack_push_int(result);    native_stack_frame_end();} while(0)
#define bc_native_end_f(result) do {native_exit_jni(vmCurrentContext); stack_push_float(result);  native_stack_frame_end();} while(0)
#define bc_native_end_l(result) do {native_exit_jni(vmCurrentContext); stack_push_long(result);   native_stack_frame_end();} while(0)
#define bc_native_end_d(result) do {native_exit_jni(vmCurrentContext); stack_push_double(result); native_stack_frame_end();} while(0)
// For reference return types, we need to convert the native handler to object reference
#define bc_native_end_a(result) bc_native_end_o(result)
#define bc_native_end_o(result) do {native_exit_jni(vmCurrentContext); JAVA_OBJECT obj = native_dereference(vmCurrentContext, result); stack_push_object(obj); native_stack_frame_end();} while(0)

// JNI argument passing helpers
#define bc_jni_arg_jboolean(local)       (local_of(local).data.i == 0 ? JNI_FALSE : JNI_TRUE)
#define bc_jni_arg_jchar(local)          ((jchar)local_of(local).data.i)
#define bc_jni_arg_jbyte(local)          ((jbyte)local_of(local).data.i)
#define bc_jni_arg_jshort(local)         ((jshort)local_of(local).data.i)
#define bc_jni_arg_jint(local)           ((jint)local_of(local).data.i)
#define bc_jni_arg_jfloat(local)         ((jfloat)local_of(local).data.f)
#define bc_jni_arg_jlong(local)          ((jlong)local_of(local).data.l)
#define bc_jni_arg_jdouble(local)        ((jdouble)local_of(local).data.d)
#define bc_jni_arg_jref(local, ref_type) ((ref_type)native_get_local_ref(vmCurrentContext, local_of(local).data.o))
#define bc_jni_arg_this_class()          ((jclass)native_get_local_ref(vmCurrentContext, (JAVA_OBJECT)THIS_CLASS))

// Called at the beginning of each instance method to check the validity of the objectref
// and also populate the STACK_FRAME.thisClass
JAVA_VOID bc_check_objref(VM_PARAM_CURRENT_CONTEXT, JavaStackFrame* frame);
#define bc_check_objectref()  bc_check_objref(vmCurrentContext, &STACK_FRAME)

void *bc_resolve_native(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method);

#endif //FOXVM_VM_BYTECODE_H
