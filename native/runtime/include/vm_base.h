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

// Additional Java primary types
typedef uint8_t     JAVA_UBYTE;
typedef uint16_t    JAVA_USHORT;
typedef uint32_t    JAVA_UINT;
typedef uint64_t    JAVA_ULONG;

#define JAVA_INT_MIN ((JAVA_INT)1 << (sizeof(JAVA_INT)*8-1))        // 0x80000000 == smallest jint
#define JAVA_INT_MAX ((JAVA_INT)((JAVA_UINT)(JAVA_INT_MIN) - 1))    // 0x7FFFFFFF == largest jint

typedef enum {
    VM_TYPE_BOOLEAN = 4,
    VM_TYPE_CHAR    = 5,
    VM_TYPE_FLOAT   = 6,
    VM_TYPE_DOUBLE  = 7,
    VM_TYPE_BYTE    = 8,
    VM_TYPE_SHORT   = 9,
    VM_TYPE_INT     = 10,
    VM_TYPE_LONG    = 11,
    VM_TYPE_OBJECT  = 12,
    VM_TYPE_ARRAY   = 13,
    VM_TYPE_VOID    = 14,
    VM_TYPE_ILLEGAL = 99
} BasicType;

/** Element size of array of specified element type */
static inline size_t type_size(BasicType t) {
    switch (t) {
        case VM_TYPE_BOOLEAN:   return sizeof(JAVA_BOOLEAN);
        case VM_TYPE_CHAR:      return sizeof(JAVA_CHAR);
        case VM_TYPE_FLOAT:     return sizeof(JAVA_FLOAT);
        case VM_TYPE_DOUBLE:    return sizeof(JAVA_DOUBLE);
        case VM_TYPE_BYTE:      return sizeof(JAVA_BYTE);
        case VM_TYPE_SHORT:     return sizeof(JAVA_SHORT);
        case VM_TYPE_INT:       return sizeof(JAVA_INT);
        case VM_TYPE_LONG:      return sizeof(JAVA_LONG);
        case VM_TYPE_OBJECT:    return sizeof(JAVA_OBJECT);
        case VM_TYPE_ARRAY:     return sizeof(JAVA_ARRAY);
    }

    return 0;
}

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

static inline JAVA_BOOLEAN is_java_primitive(BasicType t) {
    return VM_TYPE_BOOLEAN <= t && t <= VM_TYPE_LONG ? JAVA_TRUE : JAVA_FALSE;
}

/// Convert a TypeDescriptor from a classfile signature to a BasicType
static inline BasicType type_from_descriptor(TypeDescriptor d) {
    switch (d) {
        case TYPE_DESC_BYTE:        return VM_TYPE_BYTE;
        case TYPE_DESC_CHAR:        return VM_TYPE_CHAR;
        case TYPE_DESC_DOUBLE:      return VM_TYPE_DOUBLE;
        case TYPE_DESC_FLOAT:       return VM_TYPE_FLOAT;
        case TYPE_DESC_INT:         return VM_TYPE_INT;
        case TYPE_DESC_LONG:        return VM_TYPE_LONG;
        case TYPE_DESC_SHORT:       return VM_TYPE_SHORT;
        case TYPE_DESC_BOOLEAN:     return VM_TYPE_BOOLEAN;
        case TYPE_DESC_VOID:        return VM_TYPE_VOID;
        case TYPE_DESC_REFERENCE:   return VM_TYPE_OBJECT;
        case TYPE_DESC_ARRAY:       return VM_TYPE_ARRAY;
    }
    return VM_TYPE_ILLEGAL;
}

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

/**
 * Types that are stored as VM_SLOT_INT in a VMStackSlot.
 */
static inline JAVA_BOOLEAN is_subword_type(BasicType t) {
    return (t == VM_TYPE_BOOLEAN ||
            t == VM_TYPE_CHAR ||
            t == VM_TYPE_BYTE ||
            t == VM_TYPE_SHORT) ? JAVA_TRUE : JAVA_FALSE;
}

/*
 *  11          8 7       6 5 4 3   0
 * +-------------+---------+---+----+
 * | young gen   | gc mark | unused |
 * | survive age | bits    |        |
 * +-------------+---------+---+----+
 */
#define OBJECT_FLAG_MASK ((uintptr_t)0x3) // lower 2 bits
#define OBJECT_FLAG_AGE_MASK ((uintptr_t)0xF00)
#define OBJECT_FLAG_AGE_SHIFT 8
#define OBJECT_FLAG_GC_MARK_MASK ((uintptr_t)0xC0)
#define OBJECT_FLAG_GC_MARK_0 ((uintptr_t)0x80)
#define OBJECT_FLAG_GC_MARK_1 ((uintptr_t)0x40)

static inline JAVA_CLASS obj_get_class(JAVA_OBJECT obj) {
    // The class instance is allocated with a alignment of `DATA_ALIGNMENT`
    // so the lower 2 bits (32-bit arch) or 3 bits (64-bit arch) of the pointer can be used as other
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

// The minimum size of any managed object
#define MIN_OBJECT_SIZE ((size_t)sizeof(JavaObjectBase))


#endif //FOXVM_VM_BASE_H
