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

#define JAVA_NULL   ((JAVA_OBJECT) 0)
#define JAVA_FALSE  ((JAVA_BOOLEAN) 0)
#define JAVA_TRUE   ((JAVA_BOOLEAN) 1)

// Object prototype
typedef struct _JavaClass{
    struct _JavaClass* clazz;
    void* monitor;

    const char* className;

    const struct _JavaClass* parentClass;
    const int interfaceCount;
    const struct _JavaClass** parentInterfaces;
} JavaClass, *JAVA_CLASS;

typedef struct {
    JAVA_CLASS clazz;
    void* monitor;

} JavaObjectBase, *JAVA_OBJECT;

typedef struct {
    JAVA_CLASS clazz;
    void* monitor;

    int length;
} JavaArrayBase, *JAVA_ARRAY;

typedef enum {
    VM_SLOT_INVALID,
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

typedef struct {
    JAVA_LONG threadId;
    JAVA_OBJECT currentThread;

//    VMStackSlot* stack;
//    int sta

    JAVA_OBJECT exception;
} VMThreadContext;

#endif //FOXVM_VM_BASE_H
