//
// Created by noisyfox on 2020/5/10.
//

#include "vm_bytecode.h"
#include <math.h>

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

// ..., value →
// ..., result
#define arithmetic_un_operand(required_type)    \
    VMStackSlot *value = stack->top - 1;        \
                                                \
    assert(value >= stack->slots);              \
                                                \
    assert(value->type == required_type)
// Store the result in the place of value

#define def_arithmetic_bi_func(prefix, name, operation, required_type)          \
decl_arithmetic_func(prefix##name) {                                            \
    arithmetic_bi_operand(required_type);                                       \
                                                                                \
    value1->data.prefix = value1->data.prefix operation value2->data.prefix;    \
}

#define def_arithmetic_un_func(prefix, name, operation, required_type)          \
decl_arithmetic_func(prefix##name) {                                            \
    arithmetic_un_operand(required_type);                                       \
                                                                                \
    value->data.prefix = operation value->data.prefix;                          \
}

def_arithmetic_bi_func(i, add, +, VM_SLOT_INT);
def_arithmetic_bi_func(l, add, +, VM_SLOT_LONG);
def_arithmetic_bi_func(f, add, +, VM_SLOT_FLOAT);
def_arithmetic_bi_func(d, add, +, VM_SLOT_DOUBLE);

def_arithmetic_bi_func(i, sub, -, VM_SLOT_INT);
def_arithmetic_bi_func(l, sub, -, VM_SLOT_LONG);
def_arithmetic_bi_func(f, sub, -, VM_SLOT_FLOAT);
def_arithmetic_bi_func(d, sub, -, VM_SLOT_DOUBLE);

def_arithmetic_bi_func(i, mul, *, VM_SLOT_INT);
def_arithmetic_bi_func(l, mul, *, VM_SLOT_LONG);
def_arithmetic_bi_func(f, mul, *, VM_SLOT_FLOAT);
def_arithmetic_bi_func(d, mul, *, VM_SLOT_DOUBLE);

def_arithmetic_bi_func(i, div, /, VM_SLOT_INT);
def_arithmetic_bi_func(l, div, /, VM_SLOT_LONG);
def_arithmetic_bi_func(f, div, /, VM_SLOT_FLOAT);
def_arithmetic_bi_func(d, div, /, VM_SLOT_DOUBLE);

def_arithmetic_bi_func(i, rem, %, VM_SLOT_INT);
def_arithmetic_bi_func(l, rem, %, VM_SLOT_LONG);
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

def_arithmetic_un_func(i, neg, -, VM_SLOT_INT);
def_arithmetic_un_func(l, neg, -, VM_SLOT_LONG);
def_arithmetic_un_func(f, neg, -, VM_SLOT_FLOAT);
def_arithmetic_un_func(d, neg, -, VM_SLOT_DOUBLE);

def_arithmetic_bi_func(i, and, &, VM_SLOT_INT);
def_arithmetic_bi_func(l, and, &, VM_SLOT_LONG);

def_arithmetic_bi_func(i, or, |, VM_SLOT_INT);
def_arithmetic_bi_func(l, or, |, VM_SLOT_LONG);

def_arithmetic_bi_func(i, xor, ^, VM_SLOT_INT);
def_arithmetic_bi_func(l, xor, ^, VM_SLOT_LONG);
