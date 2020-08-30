//
// Created by noisyfox on 2018/10/20.
//

#ifndef FOXVM_VM_ARRAYS_H
#define FOXVM_VM_ARRAYS_H

#include "vm_base.h"
#include <assert.h>

//*********************************************************************************************************
// Classes of array of primary type
//*********************************************************************************************************
extern JavaClass* g_class_array_Z;
extern JavaClass* g_class_array_B;
extern JavaClass* g_class_array_C;
extern JavaClass* g_class_array_S;
extern JavaClass* g_class_array_I;
extern JavaClass* g_class_array_J;
extern JavaClass* g_class_array_F;
extern JavaClass* g_class_array_D;

typedef struct _JavaArrayClass {
    JavaClass baseClass;

    // We store the class info together with the class so we don't
    // need to worry about memory memory management.
    JavaClassInfo classInfo;
    JAVA_CLASS componentType;
} JavaArrayClass;

struct _JavaArray {
    JavaObjectBase baseObject;

    JAVA_INT length;
};

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

static inline BasicType array_type_of(C_CSTR desc) {
    assert(desc[0] == TYPE_DESC_ARRAY);
    BasicType t = type_from_descriptor(desc[1]);
    assert(t != VM_TYPE_ILLEGAL);

    return t;
}

#endif //FOXVM_VM_ARRAYS_H
