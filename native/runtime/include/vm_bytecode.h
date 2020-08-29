//
// Created by noisyfox on 2020/5/10.
//

#ifndef FOXVM_VM_BYTECODE_H
#define FOXVM_VM_BYTECODE_H

#include "vm_stack.h"
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

// invokeXXXX instructions
#define bc_invoke_special(fp)                          ((JavaMethodRetVoid)    fp)(vmCurrentContext)
#define bc_invoke_special_z(fp) do {JAVA_BOOLEAN ret = ((JavaMethodRetBoolean) fp)(vmCurrentContext); stack_push_int(ret);                 }while(0)
#define bc_invoke_special_c(fp) do {JAVA_CHAR    ret = ((JavaMethodRetChar)    fp)(vmCurrentContext); stack_push_int(ret);                 }while(0)
#define bc_invoke_special_b(fp) do {JAVA_BYTE    ret = ((JavaMethodRetByte)    fp)(vmCurrentContext); stack_push_int(ret);                 }while(0)
#define bc_invoke_special_s(fp) do {JAVA_SHORT   ret = ((JavaMethodRetShort)   fp)(vmCurrentContext); stack_push_int(ret);                 }while(0)
#define bc_invoke_special_i(fp) do {JAVA_INT     ret = ((JavaMethodRetInt)     fp)(vmCurrentContext); stack_push_int(ret);                 }while(0)
#define bc_invoke_special_f(fp) do {JAVA_FLOAT   ret = ((JavaMethodRetFloat)   fp)(vmCurrentContext); stack_push_float(ret);               }while(0)
#define bc_invoke_special_j(fp) do {JAVA_LONG    ret = ((JavaMethodRetLong)    fp)(vmCurrentContext); stack_push_long(ret);                }while(0)
#define bc_invoke_special_d(fp) do {JAVA_DOUBLE  ret = ((JavaMethodRetDouble)  fp)(vmCurrentContext); stack_push_double(ret);              }while(0)
#define bc_invoke_special_a(fp) do {JAVA_ARRAY   ret = ((JavaMethodRetArray)   fp)(vmCurrentContext); stack_push_object((JAVA_OBJECT)ret); }while(0)
#define bc_invoke_special_o(fp) do {JAVA_OBJECT  ret = ((JavaMethodRetObject)  fp)(vmCurrentContext); stack_push_object(ret);              }while(0)

// field instructions
JAVA_VOID bc_putfield(VMOperandStack *stack, JAVA_OBJECT *objRefOut, void *valueOut, BasicType fieldType);
#define bc_do_putfield(object_type, field_name, field_type)             \
    JAVA_OBJECT objectRef;                                              \
    JAVA_##field_type value;                                            \
    bc_putfield(OP_STACK, &objectRef, &value, VM_TYPE_##field_type);    \
    ((object_type*)objectRef)->field_name = value
#define bc_putfield_z(object_type, field_name) do {bc_do_putfield(object_type, field_name, BOOLEAN);} while(0)
#define bc_putfield_c(object_type, field_name) do {bc_do_putfield(object_type, field_name, CHAR);   } while(0)
#define bc_putfield_b(object_type, field_name) do {bc_do_putfield(object_type, field_name, BYTE);   } while(0)
#define bc_putfield_s(object_type, field_name) do {bc_do_putfield(object_type, field_name, SHORT);  } while(0)
#define bc_putfield_i(object_type, field_name) do {bc_do_putfield(object_type, field_name, INT);    } while(0)
#define bc_putfield_f(object_type, field_name) do {bc_do_putfield(object_type, field_name, FLOAT);  } while(0)
#define bc_putfield_j(object_type, field_name) do {bc_do_putfield(object_type, field_name, LONG);   } while(0)
#define bc_putfield_d(object_type, field_name) do {bc_do_putfield(object_type, field_name, DOUBLE); } while(0)
// TODO: update card table for cross-gen reference
#define bc_putfield_a(object_type, field_name) do {bc_do_putfield(object_type, field_name, ARRAY);  } while(0)
#define bc_putfield_o(object_type, field_name) do {bc_do_putfield(object_type, field_name, OBJECT); } while(0)

JAVA_VOID bc_getfield(VMOperandStack *stack, JAVA_OBJECT *objRefOut);
#define bc_do_getfield(object_type, field_name, field_type)             \
    JAVA_OBJECT objectRef;                                              \
    bc_getfield(OP_STACK, &objectRef);                                  \
    JAVA_##field_type value = ((object_type*)objectRef)->field_name
#define bc_getfield_z(object_type, field_name) do {bc_do_getfield(object_type, field_name, BOOLEAN); stack_push_int(value);                } while(0)
#define bc_getfield_c(object_type, field_name) do {bc_do_getfield(object_type, field_name, CHAR);    stack_push_int(value);                } while(0)
#define bc_getfield_b(object_type, field_name) do {bc_do_getfield(object_type, field_name, BYTE);    stack_push_int(value);                } while(0)
#define bc_getfield_s(object_type, field_name) do {bc_do_getfield(object_type, field_name, SHORT);   stack_push_int(value);                } while(0)
#define bc_getfield_i(object_type, field_name) do {bc_do_getfield(object_type, field_name, INT);     stack_push_int(value);                } while(0)
#define bc_getfield_f(object_type, field_name) do {bc_do_getfield(object_type, field_name, FLOAT);   stack_push_float(value);              } while(0)
#define bc_getfield_j(object_type, field_name) do {bc_do_getfield(object_type, field_name, LONG);    stack_push_long(value);               } while(0)
#define bc_getfield_d(object_type, field_name) do {bc_do_getfield(object_type, field_name, DOUBLE);  stack_push_double(value);             } while(0)
#define bc_getfield_a(object_type, field_name) do {bc_do_getfield(object_type, field_name, ARRAY);   stack_push_object((JAVA_OBJECT)value);} while(0)
#define bc_getfield_o(object_type, field_name) do {bc_do_getfield(object_type, field_name, OBJECT);  stack_push_object(value);             } while(0)

JAVA_VOID bc_putstatic(VM_PARAM_CURRENT_CONTEXT, VMStackFrame *frame, JavaClassInfo *classInfo,
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
#define bc_putstatic_j(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, LONG);   } while(0)
#define bc_putstatic_d(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, DOUBLE); } while(0)
// TODO: update card table for cross-gen reference
#define bc_putstatic_a(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, ARRAY);  } while(0)
#define bc_putstatic_o(class_info, class_type, field_name) do {bc_do_putstatic(class_info, class_type, field_name, OBJECT); } while(0)

// FoxVM specific instructions
#define bc_prepare_arguments(argument_count) local_transfer_arguments(&STACK_FRAME, argument_count)

/** Record current source file line number. */
#define bc_line(line_number) STACK_FRAME.currentLine = (line_number)

/** Record current label. */
#define bc_label(label_number) STACK_FRAME.currentLabel = (label_number)

// JNI argument passing helpers
#define bc_jni_arg_jboolean(local)       (local_of(local).data.i == 0 ? JNI_FALSE : JNI_TRUE)
#define bc_jni_arg_jchar(local)          ((jchar)local_of(local).data.i)
#define bc_jni_arg_jbyte(local)          ((jbyte)local_of(local).data.i)
#define bc_jni_arg_jshort(local)         ((jshort)local_of(local).data.i)
#define bc_jni_arg_jint(local)           ((jint)local_of(local).data.i)
#define bc_jni_arg_jfloat(local)         ((jfloat)local_of(local).data.f)
#define bc_jni_arg_jlong(local)          ((jlong)local_of(local).data.l)
#define bc_jni_arg_jdouble(local)        ((jdouble)local_of(local).data.d)
#define bc_jni_arg_jref(local, ref_type) ((ref_type)local_of(local).data.o)

// Called at the beginning of each instance method to check the validity of the objectref
// and also populate the STACK_FRAME.thisClass
JAVA_VOID bc_check_objref(VMStackFrame* frame);
#define bc_check_objectref()  bc_check_objref(&STACK_FRAME)

#endif //FOXVM_VM_BYTECODE_H
