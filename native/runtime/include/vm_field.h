//
// Created by noisyfox on 2020/9/3.
//

#ifndef FOXVM_VM_FIELD_H
#define FOXVM_VM_FIELD_H

#include "vm_base.h"

static inline JAVA_BOOLEAN field_is_static(ResolvedField *field) {
    return (field->info.accessFlags & FIELD_ACC_STATIC) == FIELD_ACC_STATIC;
}

ResolvedField *field_find(JAVA_CLASS clazz, C_CSTR name, C_CSTR desc);

#endif //FOXVM_VM_FIELD_H
