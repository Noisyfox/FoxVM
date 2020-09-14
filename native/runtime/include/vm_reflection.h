//
// Created by noisyfox on 2020/9/14.
//

#ifndef FOXVM_VM_REFLECTION_H
#define FOXVM_VM_REFLECTION_H

#include "vm_base.h"

JAVA_VOID reflection_init(VM_PARAM_CURRENT_CONTEXT);

JAVA_OBJECT reflection_method_new_constructor(VM_PARAM_CURRENT_CONTEXT, JAVA_CLASS cls, JAVA_INT index);

#endif //FOXVM_VM_REFLECTION_H
