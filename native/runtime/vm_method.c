//
// Created by noisyfox on 2020/9/7.
//

#include "vm_method.h"
#include "vm_string.h"
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
