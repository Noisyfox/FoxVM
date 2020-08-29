//
// Created by noisyfox on 2020/5/10.
//

#include "vm_bytecode.h"
#include <math.h>
#include "vm_classloader.h"

#define JAVA_TYPE_i JAVA_INT
#define JAVA_TYPE_l JAVA_LONG
#define JAVA_TYPE_f JAVA_FLOAT
#define JAVA_TYPE_d JAVA_DOUBLE
#define JAVA_TYPE_b JAVA_BYTE
#define JAVA_TYPE_c JAVA_CHAR
#define JAVA_TYPE_s JAVA_SHORT
#define java_type_of(prefix) JAVA_TYPE_##prefix

#define SLOT_TYPE_a VM_SLOT_OBJECT
#define SLOT_TYPE_i VM_SLOT_INT
#define SLOT_TYPE_l VM_SLOT_LONG
#define SLOT_TYPE_f VM_SLOT_FLOAT
#define SLOT_TYPE_d VM_SLOT_DOUBLE
#define SLOT_TYPE_b VM_SLOT_INT
#define SLOT_TYPE_c VM_SLOT_INT
#define SLOT_TYPE_s VM_SLOT_INT
#define slot_type_of(prefix) SLOT_TYPE_##prefix

#define SLOT_VALUE_a o
#define SLOT_VALUE_i i
#define SLOT_VALUE_l l
#define SLOT_VALUE_f f
#define SLOT_VALUE_d d
#define SLOT_VALUE_b i
#define SLOT_VALUE_c i
#define SLOT_VALUE_s i
#define slot_value_of(prefix) SLOT_VALUE_##prefix

// ..., value1, value2 →
// ..., result
#define arithmetic_bi_operand(required_type)    \
    VMStackSlot *value2 = stack->top - 1;       \
    VMStackSlot *value1 = stack->top - 2;       \
                                                \
    assert(value1 >= stack->slots);             \
                                                \
    assert(value1->type == required_type);      \
    assert(value2->type == required_type);      \
                                                \
    /* Pop value2 */                            \
    stack->top = value2;                        \
    value2->type = VM_SLOT_INVALID
    // Store the result in the place of value1

// ..., value1, value2 →
// ...
#define arithmetic_bi_operand_nr(required_type) \
    VMStackSlot *value2 = stack->top - 1;       \
    VMStackSlot *value1 = stack->top - 2;       \
                                                \
    assert(value1 >= stack->slots);             \
                                                \
    assert(value1->type == required_type);      \
    assert(value2->type == required_type);      \
                                                \
    /* Pop value1 and value2 */                 \
    stack->top = value1;                        \
    value1->type = VM_SLOT_INVALID;             \
    value2->type = VM_SLOT_INVALID

// ..., value →
// ..., result
#define arithmetic_un_operand(required_type)    \
    VMStackSlot *value = stack->top - 1;        \
                                                \
    assert(value >= stack->slots);              \
                                                \
    assert(value->type == required_type)
// Store the result in the place of value

// ..., value →
// ...
#define arithmetic_un_operand_nr(required_type) \
    VMStackSlot *value = stack->top - 1;        \
                                                \
    assert(value >= stack->slots);              \
                                                \
    assert(value->type == required_type);       \
                                                \
    /* Pop value */                             \
    stack->top = value;                         \
    value->type = VM_SLOT_INVALID

#define def_arithmetic_bi_func(prefix, name, operation)                                     \
decl_arithmetic_func(prefix##name) {                                                        \
    arithmetic_bi_operand(slot_type_of(prefix));                                            \
                                                                                            \
    value1->data.slot_value_of(prefix)                                                      \
        = value1->data.slot_value_of(prefix) operation value2->data.slot_value_of(prefix);  \
}

#define def_arithmetic_un_func(prefix, name, operation)                                 \
decl_arithmetic_func(prefix##name) {                                                    \
    arithmetic_un_operand(slot_type_of(prefix));                                        \
                                                                                        \
    value->data.slot_value_of(prefix) = operation value->data.slot_value_of(prefix);    \
}

def_arithmetic_bi_func(i, add, +);
def_arithmetic_bi_func(l, add, +);
def_arithmetic_bi_func(f, add, +);
def_arithmetic_bi_func(d, add, +);

def_arithmetic_bi_func(i, sub, -);
def_arithmetic_bi_func(l, sub, -);
def_arithmetic_bi_func(f, sub, -);
def_arithmetic_bi_func(d, sub, -);

def_arithmetic_bi_func(i, mul, *);
def_arithmetic_bi_func(l, mul, *);
def_arithmetic_bi_func(f, mul, *);
def_arithmetic_bi_func(d, mul, *);

def_arithmetic_bi_func(i, div, /);
def_arithmetic_bi_func(l, div, /);
def_arithmetic_bi_func(f, div, /);
def_arithmetic_bi_func(d, div, /);

def_arithmetic_bi_func(i, rem, %);
def_arithmetic_bi_func(l, rem, %);
// % does not work with float and double
decl_arithmetic_func(frem) {
    arithmetic_bi_operand(VM_SLOT_FLOAT);

    // fmodf produce incorrect results for values close to zero.Converting the
    // operands to doubles and using fmod() returns the expected result. See
    // https://github.com/robovm/robovm/issues/89
    value1->data.f = fmod(value1->data.f, value2->data.f);
}
decl_arithmetic_func(drem) {
    arithmetic_bi_operand(VM_SLOT_DOUBLE);

    value1->data.d = fmod(value1->data.d, value2->data.d);
}

def_arithmetic_un_func(i, neg, -);
def_arithmetic_un_func(l, neg, -);
def_arithmetic_un_func(f, neg, -);
def_arithmetic_un_func(d, neg, -);

#define def_ishift_func(name, operation)                                                        \
decl_arithmetic_func(i##name) {                                                                 \
    arithmetic_bi_operand(VM_SLOT_INT);                                                         \
                                                                                                \
    value1->data.i = (JAVA_UINT)value1->data.i operation ((JAVA_UINT)value2->data.i & 0x1Fu);   \
}

#define def_lshift_func(name, operation)                                                        \
decl_arithmetic_func(l##name) {                                                                 \
    arithmetic_bi_operand(VM_SLOT_LONG);                                                        \
                                                                                                \
    value1->data.l = (JAVA_ULONG)value1->data.l operation ((JAVA_ULONG)value2->data.l & 0x3Fu); \
}

// Left shift
def_ishift_func(shl, <<);
def_lshift_func(shl, <<);
// Logical right shift
def_ishift_func(ushr, >>);
def_lshift_func(ushr, >>);
// C dose not have a signed right shift (arithmetic right shift) operator
decl_arithmetic_func(ishr) {
    arithmetic_bi_operand(VM_SLOT_INT);

    JAVA_UINT s = (JAVA_UINT)value2->data.i & 0x1Fu;
    if(s > 0) {
        JAVA_INT x = value1->data.i;
        if(x < 0) {
            value1->data.i = (JAVA_UINT)x >> s | ~(~((JAVA_UINT)0u) >> s);
        } else {
            // Same as logical shift
            value1->data.i = (JAVA_UINT)x >> s;
        }
    }
}
decl_arithmetic_func(lshr) {
    arithmetic_bi_operand(VM_SLOT_LONG);

    JAVA_ULONG s = (JAVA_ULONG)value2->data.l & 0x3Fu;
    if(s > 0) {
        JAVA_LONG x = value1->data.l;
        if(x < 0) {
            value1->data.l = (JAVA_ULONG)x >> s | ~(~((JAVA_ULONG)0u) >> s);
        } else {
            // Same as logical shift
            value1->data.l = (JAVA_ULONG)x >> s;
        }
    }
}

def_arithmetic_bi_func(i, and, &);
def_arithmetic_bi_func(l, and, &);

def_arithmetic_bi_func(i, or, |);
def_arithmetic_bi_func(l, or, |);

def_arithmetic_bi_func(i, xor, ^);
def_arithmetic_bi_func(l, xor, ^);

// Type conversion operations
#define def_conv_func(from_prefix, to_prefix)                                   \
decl_arithmetic_func(from_prefix##2##to_prefix) {                               \
    arithmetic_un_operand(slot_type_of(from_prefix));                           \
                                                                                \
    java_type_of(to_prefix) converted = value->data.slot_value_of(from_prefix); \
    value->type = slot_type_of(to_prefix);                                      \
    value->data.slot_value_of(to_prefix) = converted;                           \
}

def_conv_func(i, l);
def_conv_func(i, f);
def_conv_func(i, d);

def_conv_func(l, i);
def_conv_func(l, f);
def_conv_func(l, d);

def_conv_func(f, i);
def_conv_func(f, l);
def_conv_func(f, d);

def_conv_func(d, i);
def_conv_func(d, l);
def_conv_func(d, f);

def_conv_func(i, b);
def_conv_func(i, c);
def_conv_func(i, s);

// Comparison operations
#define CMP_RESULT_g 1
#define CMP_RESULT_l -1
#define cmp_result_of(r) CMP_RESULT_##r

#define IS_NAN_f isnanf
#define IS_NAN_d isnan
#define is_nan(prefix, v)  IS_NAN_##prefix(v)

decl_arithmetic_func(lcmp) {
    arithmetic_bi_operand(VM_SLOT_LONG);

    if (value1->data.l == value2->data.l) {
        value1->data.i = 0;
    } else if (value1->data.l > value2->data.l) {
        value1->data.i = cmp_result_of(g);
    } else {
        value1->data.i = cmp_result_of(l);
    }
    value1->type = VM_SLOT_INT;
}

#define def_cmp_func(prefix, suffix)                                \
decl_arithmetic_func(prefix##cmp##suffix) {                         \
    arithmetic_bi_operand(slot_type_of(prefix));                    \
                                                                    \
    java_type_of(prefix) v1 = value1->data.slot_value_of(prefix);   \
    java_type_of(prefix) v2 = value2->data.slot_value_of(prefix);   \
                                                                    \
    if(is_nan(prefix, v1) || is_nan(prefix, v2)) {                  \
        value1->data.i = cmp_result_of(suffix);                     \
    } else if(v1 == v2){                                            \
        value1->data.i = 0;                                         \
    } else if(v1 > v2){                                             \
        value1->data.i = cmp_result_of(g);                          \
    } else {                                                        \
        value1->data.i = cmp_result_of(l);                          \
    }                                                               \
    value1->type = VM_SLOT_INT;                                     \
}

def_cmp_func(f, l);
def_cmp_func(f, g);
def_cmp_func(d, l);
def_cmp_func(d, g);

// Branch operations
#define CMP_eq ==
#define CMP_ne !=
#define CMP_lt <
#define CMP_le <=
#define CMP_gt >
#define CMP_ge >=
#define comparator_of(c) CMP_##c

#define def_branch_if(suffix)                                               \
JAVA_BOOLEAN bc_branch_if##suffix(VMOperandStack *stack) {                  \
    arithmetic_un_operand_nr(VM_SLOT_INT);                                  \
                                                                            \
    return value->data.i comparator_of(suffix) 0 ? JAVA_TRUE : JAVA_FALSE;  \
}
def_branch_if(eq);
def_branch_if(ne);
def_branch_if(lt);
def_branch_if(le);
def_branch_if(gt);
def_branch_if(ge);

#define def_branch_ifcmp(prefix, suffix)                                    \
JAVA_BOOLEAN bc_branch_if_##prefix##cmp##suffix(VMOperandStack *stack) {    \
    arithmetic_bi_operand_nr(slot_type_of(prefix));                         \
                                                                            \
    return value1->data.slot_value_of(prefix)                               \
        comparator_of(suffix) value2->data.slot_value_of(prefix)            \
        ? JAVA_TRUE : JAVA_FALSE;                                           \
}
def_branch_ifcmp(i, eq);
def_branch_ifcmp(i, ne);
def_branch_ifcmp(i, lt);
def_branch_ifcmp(i, le);
def_branch_ifcmp(i, gt);
def_branch_ifcmp(i, ge);
def_branch_ifcmp(a, eq);
def_branch_ifcmp(a, ne);

JAVA_BOOLEAN bc_branch_ifnull(VMOperandStack *stack) {
    arithmetic_un_operand_nr(VM_SLOT_OBJECT);

    return value->data.o == JAVA_NULL ? JAVA_TRUE : JAVA_FALSE;
}

JAVA_INT bc_switch_get_index(VMOperandStack *stack) {
    arithmetic_un_operand_nr(VM_SLOT_INT);

    return value->data.i;
}

JAVA_VOID bc_check_objref(VMStackFrame *frame) {
    // objectref is always in the local0
    assert(frame->locals.maxLocals > 0);
    VMStackSlot *objSlot = &frame->locals.slots[0];
    assert(objSlot->type == VM_SLOT_OBJECT);

    JAVA_OBJECT obj = objSlot->data.o;
    assert(obj != JAVA_NULL); // TODO: throw NullPointerException instead

    frame->thisClass = obj->clazz;
    assert(frame->thisClass != (JAVA_CLASS) JAVA_NULL);
}

JAVA_VOID bc_putfield(VMOperandStack *stack, JAVA_OBJECT *objRefOut, void *valueOut, BasicType fieldType) {
    VMStackSlot *value = stack->top - 1;
    VMStackSlot *objectRef = stack->top - 2;

    assert(objectRef >= stack->slots);

    assert(objectRef->type == VM_SLOT_OBJECT);
    assert(objectRef->data.o != JAVA_NULL); // TODO: throw NullPointerException instead

    *objRefOut = objectRef->data.o;

    switch (fieldType) {
        case VM_TYPE_BOOLEAN:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_BOOLEAN *) valueOut) = value->data.i;
            break;
        case VM_TYPE_CHAR:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_CHAR *) valueOut) = value->data.i;
            break;
        case VM_TYPE_FLOAT:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_FLOAT *) valueOut) = value->data.f;
            break;
        case VM_TYPE_DOUBLE:
            assert(value->type == VM_SLOT_DOUBLE);
            *((JAVA_DOUBLE *) valueOut) = value->data.d;
            break;
        case VM_TYPE_BYTE:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_BYTE *) valueOut) = value->data.i;
            break;
        case VM_TYPE_SHORT:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_SHORT *) valueOut) = value->data.i;
            break;
        case VM_TYPE_INT:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_INT *) valueOut) = value->data.i;
            break;
        case VM_TYPE_LONG:
            assert(value->type == VM_SLOT_LONG);
            *((JAVA_LONG *) valueOut) = value->data.l;
            break;
        case VM_TYPE_OBJECT:
            assert(value->type == VM_SLOT_OBJECT);
            *((JAVA_OBJECT *) valueOut) = value->data.o;
            break;
        case VM_TYPE_ARRAY:
            assert(value->type == VM_SLOT_OBJECT);
            *((JAVA_ARRAY *) valueOut) = (JAVA_ARRAY) value->data.o;
            break;
        case VM_TYPE_VOID:
        case VM_TYPE_ILLEGAL:
            assert(!"Unexpected value type");
    }

    // Pop
    stack->top = objectRef;
    value->type = VM_SLOT_INVALID;
    objectRef->type = VM_SLOT_INVALID;
}

JAVA_VOID bc_getfield(VMOperandStack *stack, JAVA_OBJECT *objRefOut) {
    VMStackSlot *objectRef = stack->top - 1;

    assert(objectRef >= stack->slots);

    assert(objectRef->type == VM_SLOT_OBJECT);
    assert(objectRef->data.o != JAVA_NULL); // TODO: throw NullPointerException instead

    *objRefOut = objectRef->data.o;

    // Pop
    stack->top = objectRef;
    objectRef->type = VM_SLOT_INVALID;
}

JAVA_VOID bc_putstatic(VM_PARAM_CURRENT_CONTEXT, VMStackFrame *frame, JavaClassInfo *classInfo,
                       JAVA_CLASS *classRefOut, void *valueOut, BasicType fieldType) {
    // jvms8 §5.5 Initialization
    // A class or interface C may be initialized only as a result of:
    // • The execution of any one of the Java Virtual Machine instructions new,
    //   getstatic, putstatic, or invokestatic that references C (§new, §getstatic, §putstatic,
    //   §invokestatic). These instructions reference a class or interface directly or
    //   indirectly through either a field reference or a method reference.
    //
    //   Upon execution of a getstatic, putstatic, or invokestatic instruction, the class or
    //   interface that declared the resolved field or method is initialized if it has not been
    //   initialized already.
    JAVA_CLASS clazz = classloader_get_class(vmCurrentContext, frame->thisClass->classLoader, classInfo);
    assert(clazz != (JAVA_CLASS) JAVA_NULL);
    *classRefOut = clazz;

    VMOperandStack *stack = &frame->operandStack;
    VMStackSlot *value = stack->top - 1;

    assert(value >= stack->slots);

    switch (fieldType) {
        case VM_TYPE_BOOLEAN:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_BOOLEAN *) valueOut) = value->data.i;
            break;
        case VM_TYPE_CHAR:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_CHAR *) valueOut) = value->data.i;
            break;
        case VM_TYPE_FLOAT:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_FLOAT *) valueOut) = value->data.f;
            break;
        case VM_TYPE_DOUBLE:
            assert(value->type == VM_SLOT_DOUBLE);
            *((JAVA_DOUBLE *) valueOut) = value->data.d;
            break;
        case VM_TYPE_BYTE:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_BYTE *) valueOut) = value->data.i;
            break;
        case VM_TYPE_SHORT:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_SHORT *) valueOut) = value->data.i;
            break;
        case VM_TYPE_INT:
            assert(value->type == VM_SLOT_INT);
            *((JAVA_INT *) valueOut) = value->data.i;
            break;
        case VM_TYPE_LONG:
            assert(value->type == VM_SLOT_LONG);
            *((JAVA_LONG *) valueOut) = value->data.l;
            break;
        case VM_TYPE_OBJECT:
            assert(value->type == VM_SLOT_OBJECT);
            *((JAVA_OBJECT *) valueOut) = value->data.o;
            break;
        case VM_TYPE_ARRAY:
            assert(value->type == VM_SLOT_OBJECT);
            *((JAVA_ARRAY *) valueOut) = (JAVA_ARRAY) value->data.o;
            break;
        case VM_TYPE_VOID:
        case VM_TYPE_ILLEGAL:
            assert(!"Unexpected value type");
    }

    // Pop
    stack->top = value;
    value->type = VM_SLOT_INVALID;
}

JAVA_VOID bc_getstatic(VM_PARAM_CURRENT_CONTEXT, VMStackFrame *frame, JavaClassInfo *classInfo,
                       JAVA_CLASS *classRefOut) {
    // jvms8 §5.5 Initialization
    // A class or interface C may be initialized only as a result of:
    // • The execution of any one of the Java Virtual Machine instructions new,
    //   getstatic, putstatic, or invokestatic that references C (§new, §getstatic, §putstatic,
    //   §invokestatic). These instructions reference a class or interface directly or
    //   indirectly through either a field reference or a method reference.
    //
    //   Upon execution of a getstatic, putstatic, or invokestatic instruction, the class or
    //   interface that declared the resolved field or method is initialized if it has not been
    //   initialized already.

    JAVA_CLASS clazz = classloader_get_class(vmCurrentContext, frame->thisClass->classLoader, classInfo);
    assert(clazz != (JAVA_CLASS) JAVA_NULL);
    *classRefOut = clazz;
}
