//
// Created by noisyfox on 2018/10/20.
//

#ifndef FOXVM_VM_ARRAYS_H
#define FOXVM_VM_ARRAYS_H

#include "vm_base.h"

typedef enum {
    VM_ARRAY_OBJECT = 3,
    VM_ARRAY_BOOLEAN = 4,
    VM_ARRAY_CHAR,
    VM_ARRAY_FLOAT,
    VM_ARRAY_DOUBLE,
    VM_ARRAY_BYTE,
    VM_ARRAY_SHORT,
    VM_ARRAY_INT,
    VM_ARRAY_LONG
} VMArrayElementType;

struct _JavaArray {
    OPA_ptr_t ref;
    JAVA_CLASS clazz;
    void *monitor;

    int length;
    int dimensions;
    VMArrayElementType elementType;
};


#endif //FOXVM_VM_ARRAYS_H
