//
// Created by noisyfox on 2020/9/7.
//

#ifndef FOXVM_VM_CLASS_H
#define FOXVM_VM_CLASS_H

#include "vm_base.h"

JAVA_BOOLEAN class_assignable_desc(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *s, C_CSTR t);

JAVA_BOOLEAN class_assignable(JavaClassInfo *s, JavaClassInfo *t);

JAVA_CLASS class_get_native_class(JAVA_OBJECT classObj);

#endif //FOXVM_VM_CLASS_H
