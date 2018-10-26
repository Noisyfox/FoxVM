//
// Created by noisyfox on 2018/10/24.
//

#ifndef FOXVM_VM_REFERENCE_H
#define FOXVM_VM_REFERENCE_H

#include "vm_thread.h"

typedef void* JAVA_REF; // NativeReference

/**
 * Obtain a reference of given object. The object might be moved during
 * GC, so before entering the checkpoint, we use the reference to hold
 * the object pointer. During GC, if the object is moved then the pointer
 * hold by the reference will be updated too.
 *
 * Object pointer can not be used inside checkpoint. After exiting the
 * checkpoint, we could get the updated object pointer by using
 * ref_dereference() on this reference.
 */
JAVA_REF ref_obtain(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj);

/**
 * Only used by GC. Update associated reference after the object been moved.
 *
 * @return reference to the given object pointer, or NULL if the given object
 *          doesn't associated with any reference.
 */
JAVA_REF ref_update(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj);

/**
 * Only used by GC. Free the given reference.
 */
void ref_release(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref);

/**
 * Get object pointer from given reference.
 */
JAVA_OBJECT ref_dereference(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref);

#endif //FOXVM_VM_REFERENCE_H
