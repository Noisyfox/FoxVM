//
// Created by noisyfox on 2020/9/7.
//

#include "vm_method.h"
#include <string.h>

MethodInfo *method_find(JavaClassInfo *clazz, C_CSTR name, C_CSTR desc) {
    for (uint16_t i = 0; i < clazz->methodCount; i++) {
        MethodInfo *m = clazz->methods[i];
        if (strcmp(string_get_constant_utf8(m->name), name) == 0 && strcmp(string_get_constant_utf8(m->descriptor), desc) == 0) {
            return m;
        }
    }

    if (strcmp("<init>", name) == 0 || strcmp("<clinit>", name) == 0) {
        // Class and instance initializers are not inherited,
        // so don't bother checking superclasses/superinterfaces.
        return NULL;
    }

    if (clazz->superClass) {
        MethodInfo *m = method_find(clazz->superClass, name, desc);
        if (m) {
            return m;
        }
    }

    for (int i = 0; i < clazz->interfaceCount; i++) {
        JavaClassInfo *it = clazz->interfaces[i];
        MethodInfo *m = method_find(it, name, desc);
        if (m) {
            return m;
        }
    }

    return NULL;
}

C_CSTR method_get_return_type(C_CSTR desc) {
    // Find the end of the parameter desc
    while (*desc != ')') {
        desc++;
    }
    // Then skip the right brackets
    desc++;
    return desc;
}

C_CSTR method_get_next_parameter_type(C_CSTR* desc_cursor) {
    // The desc_cursor can be in one of the two states:
    // 1. points to the beginning of a method descriptor, which is the left brackets
    // 2. or, points to the end of previous parameter type desc
    // In both cases, we need to increase the cursor by 1 char so it points to the
    // beginning of the next parameter desc.
    (*desc_cursor)++;

    C_CSTR result = *desc_cursor;

    // Find the end of current desc
    switch (*result) {
        case TYPE_DESC_BYTE:
        case TYPE_DESC_CHAR:
        case TYPE_DESC_DOUBLE:
        case TYPE_DESC_FLOAT:
        case TYPE_DESC_INT:
        case TYPE_DESC_LONG:
        case TYPE_DESC_SHORT:
        case TYPE_DESC_BOOLEAN:
            // Single char desc, cursor already point to the end
            return result;
        case TYPE_DESC_ARRAY:
            // Recursively parsing the type desc of the array
            method_get_next_parameter_type(desc_cursor);
            // Now the desc_cursor should points to the end of a non-array type desc, which
            // is also the end of current array desc
            return result;
        case TYPE_DESC_REFERENCE:
            // A ref desc should end with ';'
            while (**desc_cursor != ';') {
                (*desc_cursor)++;
            }
            return result;
    }

    // Return NULL if it can't find any more parameters
    return NULL;
}

JAVA_INT method_get_parameter_count(C_CSTR desc) {
    JAVA_INT count = 0;
    while (method_get_next_parameter_type(&desc)) {
        count++;
    }
    return count;
}
