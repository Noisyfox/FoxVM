//
// Created by noisyfox on 2020/9/13.
//

#include "vm_primitive.h"
#include <string.h>

static JAVA_VOID resolve_handler_primitive(JAVA_CLASS c) {
    c->interfaceCount = 0;
    c->interfaces = NULL;

    c->staticFieldCount = 0;
    c->staticFields = NULL;
    c->hasStaticReference = JAVA_FALSE;

    c->fieldCount = 0;
    c->fields = NULL;
    c->hasReference = JAVA_FALSE;
}

#define def_prim_class(desc)                                                \
JavaClassInfo g_classInfo_primitive_##desc = {                              \
    .accessFlags = CLASS_ACC_ABSTRACT | CLASS_ACC_FINAL | CLASS_ACC_PUBLIC, \
    .thisClass = #desc,                                                     \
    .signature = NULL,                                                      \
    .superClass = NULL,                                                     \
    .interfaceCount = 0,                                                    \
    .interfaces = NULL,                                                     \
    .fieldCount = 0,                                                        \
    .fields = NULL,                                                         \
    .methodCount = 0,                                                       \
    .methods = NULL,                                                        \
                                                                            \
    .resolveHandler = resolve_handler_primitive,                            \
    .classSize = sizeof(JavaClass),                                         \
    .instanceSize = 0,                                                      \
                                                                            \
    .preResolvedStaticFieldCount = 0,                                       \
    .preResolvedStaticFields = NULL,                                        \
    .preResolvedInstanceFieldCount = 0,                                     \
    .preResolvedInstanceFields = NULL,                                      \
                                                                            \
    .vtableCount = 0,                                                       \
    .vtable = NULL,                                                         \
    .ivtableCount = 0,                                                      \
    .ivtable = NULL,                                                        \
                                                                            \
    .clinit = NULL,                                                         \
    .finalizer = NULL,                                                      \
};                                                                          \
JAVA_CLASS g_class_primitive_##desc = NULL

def_prim_class(Z);
def_prim_class(B);
def_prim_class(C);
def_prim_class(S);
def_prim_class(I);
def_prim_class(J);
def_prim_class(F);
def_prim_class(D);
def_prim_class(V);

#undef def_prim_class

JAVA_CLASS primitive_of_name(C_CSTR name) {
    if (strcmp("boolean", name) == 0) {
        return g_class_primitive_Z;
    } else if (strcmp("char", name) == 0) {
        return g_class_primitive_C;
    } else if (strcmp("float", name) == 0) {
        return g_class_primitive_F;
    } else if (strcmp("double", name) == 0) {
        return g_class_primitive_D;
    } else if (strcmp("byte", name) == 0) {
        return g_class_primitive_B;
    } else if (strcmp("short", name) == 0) {
        return g_class_primitive_S;
    } else if (strcmp("int", name) == 0) {
        return g_class_primitive_I;
    } else if (strcmp("long", name) == 0) {
        return g_class_primitive_J;
    } else if (strcmp("void", name) == 0) {
        return g_class_primitive_V;
    }

    return (JAVA_CLASS) JAVA_NULL;
}
