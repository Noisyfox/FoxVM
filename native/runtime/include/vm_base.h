//
// Created by noisyfox on 2018/10/1.
//
// Base type defs that will be used everywhere.
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

typedef enum {
    ACC_PUBLIC = 0x0001,
    ACC_PRIVATE = 0x0002,
    ACC_PROTECTED = 0x0004,
    ACC_STATIC = 0x0008,
    ACC_FINAL = 0x0010,
    ACC_VOLATILE = 0x0040,
    ACC_TRANSIENT = 0x0080,
    ACC_SYNTHETIC = 0x1000,
    ACC_ENUM = 0x4000,
} FieldAccFlag;

typedef enum {
    TYPE_DESC_BYTE = 'B',
    TYPE_DESC_CHAR = 'C',
    TYPE_DESC_DOUBLE = 'D',
    TYPE_DESC_FLOAT = 'F',
    TYPE_DESC_INT = 'I',
    TYPE_DESC_LONG = 'J',
    TYPE_DESC_SHORT = 'S',
    TYPE_DESC_BOOLEAN = 'Z',
    TYPE_DESC_VOID = 'V',
    TYPE_DESC_REFERENCE = 'L',
    TYPE_DESC_ARRAY = '[',
} TypeDescriptor;

typedef struct {
    uint16_t accessFlags;
    const char *name;
    const char *descriptor;

    // TODO: field attributes

    // GC scanning information
    size_t offset;
    JAVA_BOOLEAN isReference;
} FieldDesc;

static inline JAVA_BOOLEAN field_is_static(FieldDesc *f) {
    return (f->accessFlags & ACC_STATIC) == ACC_STATIC ? JAVA_TRUE : JAVA_FALSE;
}

typedef struct {
    uint16_t fieldCount;
    FieldDesc *fields;
} FieldTable;

struct _JavaClass {
    OPA_ptr_t clazz; // Always NULL for class instance
    void *monitor;

    size_t classSize;
    size_t instanceSize;

    const char *className;

    JAVA_OBJECT classLoader;

    JAVA_CLASS parentClass;
    int interfaceCount;
    JAVA_CLASS *parentInterfaces;

    FieldTable *fieldTable; // Fields of parent classes are not included here.
};

// Object prototype
struct _JavaObject {
    OPA_ptr_t clazz;
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

/*
 *  11          8 7       6 5 4 3   0
 * +-------------+---------+---+----+
 * | young gen   | gc mark | unused |
 * | survive age | bits    |        |
 * +-------------+---------+---+----+
 */
#define OBJECT_FLAG_MASK ((uintptr_t)0xFFF)
#define OBJECT_FLAG_AGE_MASK ((uintptr_t)0xF00)
#define OBJECT_FLAG_AGE_SHIFT 8
#define OBJECT_FLAG_GC_MARK_MASK ((uintptr_t)0xC0)
#define OBJECT_FLAG_GC_MARK_0 ((uintptr_t)0x80)
#define OBJECT_FLAG_GC_MARK_1 ((uintptr_t)0x40)

static inline JAVA_CLASS obj_get_class(JAVA_OBJECT obj) {
    // The class instance is allocated with a alignment of 4096
    // so the lower 12 bits of the pointer can be used as other
    // flags. When we want the real address of the class instance,
    // we need to zero out the lower bits

    uintptr_t ref = (uintptr_t) OPA_load_ptr(&obj->clazz);
    ref = ref & (~OBJECT_FLAG_MASK);

    return (void *) ref;
}

static inline void obj_set_flags(JAVA_OBJECT obj, uintptr_t flags) {
    flags &= OBJECT_FLAG_MASK;

    void *orig_ref = OPA_load_ptr(&obj->clazz);

    while (1) {
        uintptr_t new_ref = ((uintptr_t) orig_ref) | flags;
        void *prev = OPA_cas_ptr(&obj->clazz, orig_ref, (void *) new_ref);
        if (prev == orig_ref) {
            break;
        } else {
            orig_ref = prev;
        }
    }
}

static inline void obj_clear_flags(JAVA_OBJECT obj, uintptr_t flags) {
    flags &= OBJECT_FLAG_MASK;
    flags = ~flags;

    void *orig_ref = OPA_load_ptr(&obj->clazz);

    while (1) {
        uintptr_t new_ref = ((uintptr_t) orig_ref) & flags;
        void *prev = OPA_cas_ptr(&obj->clazz, orig_ref, (void *) new_ref);
        if (prev == orig_ref) {
            break;
        } else {
            orig_ref = prev;
        }
    }
}

static inline void obj_reset_flags_masked(JAVA_OBJECT obj, uintptr_t mask, uintptr_t flags) {
    mask &= OBJECT_FLAG_MASK;
    flags &= mask;

    void *orig_ref = OPA_load_ptr(&obj->clazz);

    while (1) {
        uintptr_t new_ref = (((uintptr_t) orig_ref) & (~mask)) | flags;
        void *prev = OPA_cas_ptr(&obj->clazz, orig_ref, (void *) new_ref);
        if (prev == orig_ref) {
            break;
        } else {
            orig_ref = prev;
        }
    }
}

static inline void obj_reset_flags(JAVA_OBJECT obj, uintptr_t flags) {
    obj_reset_flags_masked(obj, OBJECT_FLAG_MASK, flags);
}

static inline uintptr_t obj_get_flags(JAVA_OBJECT obj) {
    uintptr_t ref = (uintptr_t) OPA_load_ptr(&obj->clazz);

    return ref & OBJECT_FLAG_MASK;
}

static inline JAVA_BOOLEAN obj_test_flags_and(JAVA_OBJECT obj, uintptr_t flags) {
    flags &= OBJECT_FLAG_MASK;

    uintptr_t current_flags = obj_get_flags(obj);

    return (flags & current_flags) == flags ? JAVA_TRUE : JAVA_FALSE;
}

static inline JAVA_BOOLEAN obj_test_flags_or(JAVA_OBJECT obj, uintptr_t flags) {
    flags &= OBJECT_FLAG_MASK;

    uintptr_t current_flags = obj_get_flags(obj);

    return (flags & current_flags) != 0 ? JAVA_TRUE : JAVA_FALSE;
}

typedef struct _VMThreadContext VMThreadContext;

#define vmCurrentContext __vmCurrentThreadContext
#define VM_PARAM_CURRENT_CONTEXT VMThreadContext *vmCurrentContext


#endif //FOXVM_VM_BASE_H
