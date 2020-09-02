//
// Created by noisyfox on 2020/9/2.
//

#ifndef FOXVM_VM_NATIVE_H
#define FOXVM_VM_NATIVE_H

#include "vm_base.h"

JAVA_BOOLEAN native_init();

void* native_load_library(C_CSTR name);
void* native_find_symbol(void* handle, C_CSTR symbol);

void* native_bind_method(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method);

#endif //FOXVM_VM_NATIVE_H
