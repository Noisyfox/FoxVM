//
// Created by noisyfox on 2020/9/1.
//

#ifndef FOXVM_VM_STRING_H
#define FOXVM_VM_STRING_H

#include "vm_base.h"

JAVA_OBJECT string_get_constant(VM_PARAM_CURRENT_CONTEXT, JAVA_INT constant_index);

JAVA_OBJECT string_create_utf8(VM_PARAM_CURRENT_CONTEXT, C_CSTR utf8);

#endif //FOXVM_VM_STRING_H
