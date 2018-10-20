//
// Created by noisyfox on 2018/10/20.
//

#ifndef FOXVM_VM_REFERENCE_H
#define FOXVM_VM_REFERENCE_H

#include "vm_thread.h"

JAVA_REF ref_allocate(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj);

void ref_free(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref);

void ref_update(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref, JAVA_OBJECT newAddress);


JAVA_OBJECT ref_dereference(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref);

#endif //FOXVM_VM_REFERENCE_H
