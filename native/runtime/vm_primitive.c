//
// Created by noisyfox on 2020/9/13.
//

#include "vm_primitive.h"
#include "classloader/vm_boot_classloader.h"
#include "vm_field.h"
#include "vm_exception.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

#define def_prim_class(desc)                                                    \
JavaClassInfo g_classInfo_primitive_##desc = {                                  \
    .accessFlags = CLASS_ACC_ABSTRACT | CLASS_ACC_FINAL | CLASS_ACC_PUBLIC,     \
    .modifierFlags = CLASS_ACC_ABSTRACT | CLASS_ACC_FINAL | CLASS_ACC_PUBLIC,   \
    .thisClass = #desc,                                                         \
    .signature = NULL,                                                          \
    .superClass = NULL,                                                         \
    .interfaceCount = 0,                                                        \
    .interfaces = NULL,                                                         \
    .fieldCount = 0,                                                            \
    .fields = NULL,                                                             \
    .methodCount = 0,                                                           \
    .methods = NULL,                                                            \
                                                                                \
    .resolveHandler = resolve_handler_primitive,                                \
    .classSize = sizeof(JavaClass),                                             \
    .instanceSize = 0,                                                          \
                                                                                \
    .preResolvedStaticFieldCount = 0,                                           \
    .preResolvedStaticFields = NULL,                                            \
    .preResolvedInstanceFieldCount = 0,                                         \
    .preResolvedInstanceFields = NULL,                                          \
                                                                                \
    .vtableCount = 0,                                                           \
    .vtable = NULL,                                                             \
    .ivtableCount = 0,                                                          \
    .ivtable = NULL,                                                            \
                                                                                \
    .clinit = NULL,                                                             \
    .finalizer = NULL,                                                          \
};                                                                              \
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

// Fields that stores the value in boxing classes
static ResolvedField *g_field_java_lang_Byte_value = NULL;
static ResolvedField *g_field_java_lang_Character_value = NULL;
static ResolvedField *g_field_java_lang_Double_value = NULL;
static ResolvedField *g_field_java_lang_Float_value = NULL;
static ResolvedField *g_field_java_lang_Integer_value = NULL;
static ResolvedField *g_field_java_lang_Long_value = NULL;
static ResolvedField *g_field_java_lang_Short_value = NULL;
static ResolvedField *g_field_java_lang_Boolean_value = NULL;

#define resolve_field(clazz, name, desc) do {                               \
    g_field_java_lang_##clazz##_##name =                                    \
        field_find(g_class_java_lang_##clazz, #name, desc);                 \
    if (!g_field_java_lang_##clazz##_##name) {                              \
        fprintf(stderr, "Primitive: unable to resolve field %s.%s:%s\n",    \
            g_class_java_lang_##clazz->info->thisClass, #name, desc);       \
        abort();                                                            \
    }                                                                       \
} while(0)

JAVA_VOID primitive_init(VM_PARAM_CURRENT_CONTEXT) {
    resolve_field(Byte,      value, "B");
    resolve_field(Character, value, "C");
    resolve_field(Double,    value, "D");
    resolve_field(Float,     value, "F");
    resolve_field(Integer,   value, "I");
    resolve_field(Long,      value, "J");
    resolve_field(Short,     value, "S");
    resolve_field(Boolean,   value, "Z");
}

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

#define def_unbox(return_type, class_name, suffix)                                                                      \
return_type primitive_unbox_##suffix(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o) {                                         \
    if (!o) {                                                                                                           \
        exception_set_NullPointerException(vmCurrentContext, "o");                                                      \
        return 0;                                                                                                       \
    }                                                                                                                   \
                                                                                                                        \
    if (obj_get_class(o) != g_class_java_lang_##class_name) {                                                           \
        exception_set_ClassCastException(vmCurrentContext, g_classInfo_java_lang_##class_name, obj_get_class(o)->info); \
        return 0;                                                                                                       \
    }                                                                                                                   \
                                                                                                                        \
    return *((return_type *) ptr_inc(o, g_field_java_lang_##class_name##_value->info.offset));                          \
}

def_unbox(JAVA_BYTE,    Byte,      b);
def_unbox(JAVA_CHAR,    Character, c);
def_unbox(JAVA_DOUBLE,  Double,    d);
def_unbox(JAVA_FLOAT,   Float,     f);
def_unbox(JAVA_INT,     Integer,   i);
def_unbox(JAVA_LONG,    Long,      j);
def_unbox(JAVA_SHORT,   Short,     s);
def_unbox(JAVA_BOOLEAN, Boolean,   z);
