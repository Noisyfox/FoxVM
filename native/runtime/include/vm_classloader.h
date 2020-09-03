//
// Created by noisyfox on 2020/8/27.
//

#ifndef FOXVM_VM_CLASSLOADER_H
#define FOXVM_VM_CLASSLOADER_H

#include "vm_base.h"

JAVA_BOOLEAN classloader_init(VM_PARAM_CURRENT_CONTEXT);

JAVA_CLASS classloader_get_class(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classloader, JavaClassInfo *classInfo);

JAVA_CLASS classloader_get_class_by_name(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classloader, C_CSTR className);

JAVA_CLASS classloader_get_class_init(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classloader, JavaClassInfo *classInfo);

JAVA_CLASS classloader_get_class_by_name_init(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classloader, C_CSTR className);

JAVA_BOOLEAN classloader_prepare_fields(VM_PARAM_CURRENT_CONTEXT, JAVA_CLASS thisClass);

#endif //FOXVM_VM_CLASSLOADER_H
