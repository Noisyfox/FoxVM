//
// Created by noisyfox on 2020/9/7.
//

#ifndef FOXVM_VM_METHOD_H
#define FOXVM_VM_METHOD_H

#include "vm_base.h"
#include "vm_string.h"

static inline JAVA_BOOLEAN method_is_static(MethodInfo *method) {
    return (method->accessFlags & METHOD_ACC_STATIC) == METHOD_ACC_STATIC;
}

static inline JAVA_BOOLEAN method_is_public(MethodInfo *method) {
    return (method->accessFlags & METHOD_ACC_PUBLIC) == METHOD_ACC_PUBLIC;
}

static inline JAVA_BOOLEAN method_is_object_initializer(MethodInfo *method) {
    return STRING_CONSTANT_INDEX_INIT == method->name;
}

static inline JAVA_BOOLEAN method_is_static_initializer(MethodInfo *method) {
    return STRING_CONSTANT_INDEX_CLINIT == method->name && method_is_static(method);
}

static inline JAVA_BOOLEAN method_is_initializer(MethodInfo *method) {
    return method_is_object_initializer(method) || method_is_static_initializer(method);
}

MethodInfo *method_find(JavaClassInfo *clazz, C_CSTR name, C_CSTR desc);

MethodInfo *method_vtable_get(JavaClassInfo *clazz, JAVA_INT vtable_index);

C_CSTR method_get_return_type(C_CSTR desc);
C_CSTR method_get_next_parameter_type(C_CSTR* desc_cursor);
JAVA_INT method_get_parameter_count(C_CSTR desc);

#endif //FOXVM_VM_METHOD_H
