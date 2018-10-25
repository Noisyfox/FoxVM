//
// Created by noisyfox on 2018/10/24.
//

#include "vm_reference.h"
#include <stdint.h>

typedef struct _NativeReference     NativeReference;
typedef struct _ReferenceRowMeta    ReferenceRowMeta;
typedef struct _ReferenceRow        ReferenceRow;
typedef struct _ReferenceBlockMeta  ReferenceBlockMeta;
typedef struct _ReferenceBlock      ReferenceBlock;

struct _ReferenceBlock {
    union {
        struct _ReferenceBlockMeta {
            uint64_t bitmap;
        } meta;

        struct _ReferenceRow {
            union {
                struct _ReferenceRowMeta {
                    uint64_t bitmap;
                } meta;

                struct _NativeReference {
                    OPA_ptr_t targetObject; // Not NULL!
                } ref;
            } refs[64];
        } row;
    } rows[64];
};


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
