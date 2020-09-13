//
// Created by noisyfox on 2020/9/13.
//

#include "vm_primitive.h"
#include <string.h>

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
