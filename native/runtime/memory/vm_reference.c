//
// Created by noisyfox on 2018/10/24.
//

#include "vm_reference.h"

typedef struct {
    OPA_ptr_t targetObject; // Not NULL!
} NativeReference;


JAVA_REF ref_obtain(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj) {
    // TODO: make sure not in checkpoint
    OPA_ptr_t *ptr = &obj->ref;
    NativeReference *ref = OPA_load_ptr(ptr);

    if (ref) {// Reference already created
        return ref;
    }

    // TODO: Create new reference
    return NULL;
}

JAVA_REF ref_update(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj) {
    OPA_ptr_t *ptr = &obj->ref;
    NativeReference *ref = OPA_load_ptr(ptr);

    if (ref) {
        OPA_store_ptr(&ref->targetObject, obj);
    }

    return ref;
}

JAVA_OBJECT ref_dereference(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref) {
    // TODO: make sure not in checkpoint
    return OPA_load_ptr(&((NativeReference *) ref)->targetObject);
}
