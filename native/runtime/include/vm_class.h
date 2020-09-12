//
// Created by noisyfox on 2020/9/7.
//

#ifndef FOXVM_VM_CLASS_H
#define FOXVM_VM_CLASS_H

#include "vm_base.h"

JAVA_BOOLEAN class_assignable_desc(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *s, C_CSTR t);

JAVA_BOOLEAN class_assignable(JavaClassInfo *s, JavaClassInfo *t);

JAVA_CLASS class_get_native_class(JAVA_OBJECT classObj);

/**
 * Returns a human-readable equivalent of 'descriptor'. So "I" would be "int",
 * "[[I" would be "int[][]", "[Ljava/lang/String;" would be
 * "java.lang.String[]", and so forth.
 *
 * Modified from Android PrettyDescriptor() in art\runtime\utils.cc
 *
 * The returned string is allocated using `heap_alloc_uncollectable()` and it's
 * the caller's responsibility to release it.
 */
C_CSTR class_pretty_descriptor(C_CSTR descriptor);

#endif //FOXVM_VM_CLASS_H
