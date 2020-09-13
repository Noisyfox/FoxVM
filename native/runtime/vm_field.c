//
// Created by noisyfox on 2020/9/3.
//

#include "vm_field.h"
#include "vm_string.h"
#include <string.h>

static inline JAVA_BOOLEAN field_matches(ResolvedField *field, C_CSTR name, C_CSTR desc) {
    FieldInfo *info = &field->info;
    if (strcmp(string_get_constant_utf8(info->name), name) == 0 && strcmp(string_get_constant_utf8(info->descriptor), desc) == 0) {
        return JAVA_TRUE;
    }

    return JAVA_FALSE;
}

ResolvedField *field_find(JAVA_CLASS clazz, C_CSTR name, C_CSTR desc) {
    for (uint32_t i = 0; i < clazz->fieldCount; i++) {
        ResolvedField *f = &clazz->fields[i];

        if (field_matches(f, name, desc)) {
            return f;
        }
    }

    for (uint16_t i = 0; i < clazz->staticFieldCount; i++) {
        ResolvedField *f = &clazz->staticFields[i];

        if (field_matches(f, name, desc)) {
            return f;
        }
    }

    for (int i = 0; i < clazz->interfaceCount; i++) {
        JAVA_CLASS it = clazz->interfaces[i];
        ResolvedField *f = field_find(it, name, desc);
        if (f) {
            return f;
        }
    }

    if (clazz->superClass) {
        return field_find(clazz->superClass, name, desc);
    }

    return NULL;
}
