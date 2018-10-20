//
// Created by noisyfox on 2018/10/1.
//

#ifndef FOXVM_VM_BASE_H
#define FOXVM_VM_BASE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Java primary type def
typedef void        JAVA_VOID;
typedef int         JAVA_BOOLEAN;
typedef int         JAVA_CHAR;
typedef int         JAVA_BYTE;
typedef int         JAVA_SHORT;
typedef int         JAVA_INT;
typedef long long   JAVA_LONG;
typedef float       JAVA_FLOAT;
typedef double      JAVA_DOUBLE;
typedef void*       JAVA_REF;

#define JAVA_NULL   ((JAVA_REF) 0)
#define JAVA_FALSE  ((JAVA_BOOLEAN) 0)
#define JAVA_TRUE   ((JAVA_BOOLEAN) 1)

typedef struct _JavaClass JavaClass, *JAVA_CLASS;
typedef struct _JavaObject JavaObjectBase, *JAVA_OBJECT;
typedef struct _JavaArray JavaArrayBase, *JAVA_ARRAY;

// Object prototype
struct _JavaClass {
    JAVA_REF ref; // ref of current instance

    JAVA_REF clazz;
    void *monitor;

    const char *className;

    JAVA_REF parentClass;
    int interfaceCount;
    JAVA_REF *parentInterfaces;
};

struct _JavaObject{
    JAVA_REF ref; // ref of current instance

    JAVA_REF clazz;
    void* monitor;

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
    JAVA_REF    o;
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
