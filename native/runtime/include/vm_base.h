//
// Created by noisyfox on 2018/10/1.
//

#ifndef FOXVM_VM_BASE_H
#define FOXVM_VM_BASE_H

#include "config.h"
#include <stdint.h>
#include "opa_primitives.h"

// Java primary type def
typedef void        JAVA_VOID;
typedef int8_t      JAVA_BOOLEAN;
typedef int16_t     JAVA_CHAR;
typedef int8_t      JAVA_BYTE;
typedef int16_t     JAVA_SHORT;
typedef int32_t     JAVA_INT;
typedef int64_t     JAVA_LONG;
typedef float       JAVA_FLOAT;
typedef double      JAVA_DOUBLE;

#define JAVA_NULL   ((JAVA_OBJECT) 0)
#define JAVA_FALSE  ((JAVA_BOOLEAN) 0)
#define JAVA_TRUE   ((JAVA_BOOLEAN) 1)

typedef struct _JavaClass   JavaClass,      *JAVA_CLASS;
typedef struct _JavaObject  JavaObjectBase, *JAVA_OBJECT;
typedef struct _JavaArray   JavaArrayBase,  *JAVA_ARRAY;

// Object prototype
struct _JavaClass {
    OPA_ptr_t ref;
    JAVA_CLASS clazz;
    void *monitor;

    const char *className;

    JAVA_CLASS parentClass;
    int interfaceCount;
    JAVA_CLASS *parentInterfaces;
};

struct _JavaObject {
    OPA_ptr_t ref;
    JAVA_CLASS clazz;
    void *monitor;
};

typedef enum {
    VM_SLOT_INVALID = 0,
    VM_SLOT_OBJECT,
    VM_SLOT_INT,
    VM_SLOT_LONG,
    VM_SLOT_FLOAT,
    VM_SLOT_DOUBLE
} VMStackSlotType;

typedef union {
    JAVA_OBJECT o;
    JAVA_INT    i;
    JAVA_LONG   l;
    JAVA_FLOAT  f;
    JAVA_DOUBLE d;
} VMStackSlotData;

typedef struct {
    VMStackSlotType type;
    VMStackSlotData data;
} VMStackSlot;

#endif //FOXVM_VM_BASE_H
