//
// Created by noisyfox on 2018/10/20.
//

#include "vm_reference.h"

typedef struct {
    JAVA_OBJECT targetObject; // Not NULL!
} NativeReference;

JAVA_OBJECT ref_dereference(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref) {
    // Make sure not in checkpoint!

    return ((NativeReference *) ref)->targetObject;
}

void ref_update(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref, JAVA_OBJECT newAddress) {
    ((NativeReference *) ref)->targetObject = newAddress;
    newAddress->ref = ref;
}
