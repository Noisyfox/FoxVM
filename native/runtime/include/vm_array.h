//
// Created by noisyfox on 2018/10/20.
//

#ifndef FOXVM_VM_ARRAYS_H
#define FOXVM_VM_ARRAYS_H

#include "vm_base.h"

struct _JavaArray {
    JAVA_CLASS clazz; // The class contains the info of the element type
    void *monitor;

    JAVA_INT length;
};

void array_init();

/** Returns the address of the first element. */
void *array_base(JAVA_ARRAY a, BasicType t);

/** Return the maximum length of an array of BasicType. */
size_t array_max_length(BasicType t);

/** Return the size in byte of an array of BasicType of given length. */
size_t array_size_of_type(BasicType t, size_t length);

/** Return the minimum size in byte of an array of BasicType. */
static inline size_t array_min_size_of_type(BasicType t) {
    return array_size_of_type(t, 0);
}

/** Return the maximum size in byte of an array of BasicType. */
static inline size_t array_max_size_of_type(BasicType t) {
    return array_size_of_type(t, array_max_length(t));
}

#endif //FOXVM_VM_ARRAYS_H
