//
// Created by noisyfox on 2020/9/7.
//

#ifndef FOXVM_VM_METHOD_H
#define FOXVM_VM_METHOD_H

#include "vm_base.h"

MethodInfo *method_find(JavaClassInfo *clazz, C_CSTR name, C_CSTR desc);

#endif //FOXVM_VM_METHOD_H
