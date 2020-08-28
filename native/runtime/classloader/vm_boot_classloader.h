//
// Created by noisyfox on 2020/8/27.
//

#ifndef FOXVM_VM_BOOT_CLASSLOADER_H
#define FOXVM_VM_BOOT_CLASSLOADER_H

#include "vm_base.h"

JAVA_BOOLEAN cl_bootstrap_init(VM_PARAM_CURRENT_CONTEXT);

JAVA_CLASS cl_bootstrap_find_class_by_info(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *classInfo);

JAVA_CLASS cl_bootstrap_find_class(VM_PARAM_CURRENT_CONTEXT, C_CSTR className);

#endif //FOXVM_VM_BOOT_CLASSLOADER_H
