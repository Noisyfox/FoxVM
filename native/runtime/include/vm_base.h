//
// Created by noisyfox on 2018/10/1.
//
// Base type defs that will be used everywhere.
//

#ifndef FOXVM_VM_BASE_H
#define FOXVM_VM_BASE_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>

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

typedef struct _VMThreadContext VMThreadContext;

#define vmCurrentContext __vmCurrentThreadContext
#define VM_PARAM_CURRENT_CONTEXT VMThreadContext *vmCurrentContext

// Method prototype for different return types
typedef JAVA_VOID       (*JavaMethodRetVoid)     (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_BOOLEAN    (*JavaMethodRetBoolean)  (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_CHAR       (*JavaMethodRetChar)     (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_BYTE       (*JavaMethodRetByte)     (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_SHORT      (*JavaMethodRetShort)    (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_INT        (*JavaMethodRetInt)      (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_LONG       (*JavaMethodRetFloat)    (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_FLOAT      (*JavaMethodRetLong)     (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_DOUBLE     (*JavaMethodRetDouble)   (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_ARRAY      (*JavaMethodRetArray)    (VM_PARAM_CURRENT_CONTEXT);
typedef JAVA_OBJECT     (*JavaMethodRetObject)   (VM_PARAM_CURRENT_CONTEXT);

// Additional Java primary types
typedef uint8_t     JAVA_UBYTE;
typedef uint16_t    JAVA_USHORT;
typedef uint32_t    JAVA_UINT;
typedef uint64_t    JAVA_ULONG;

// CString types, NULL terminated
typedef char*       C_STR;
typedef const char* C_CSTR;

#define JAVA_INT_MIN ((JAVA_INT)(((JAVA_UINT)1) << (sizeof(JAVA_INT)*8-1)))     // 0x80000000 == smallest jint
#define JAVA_INT_MAX ((JAVA_INT)((JAVA_UINT)(JAVA_INT_MIN) - 1))                // 0x7FFFFFFF == largest jint
#define JAVA_LONG_MIN ((JAVA_LONG)(((JAVA_ULONG)1) << (sizeof(JAVA_LONG)*8-1))) // 0x8000000000000000 == smallest jlong
#define JAVA_LONG_MAX ((JAVA_LONG)((JAVA_ULONG)(JAVA_LONG_MIN) - 1))            // 0x7FFFFFFFFFFFFFFF == largest jlong
#define JAVA_FLOAT_NAN (0.0f/0.0f)
#define JAVA_FLOAT_INF (1.0f/0.0f)
#define JAVA_FLOAT_NEG_INF (-1.0f/0.0f)
#define JAVA_DOUBLE_NAN (0.0/0.0)
#define JAVA_DOUBLE_INF (1.0/0.0)
#define JAVA_DOUBLE_NEG_INF (-1.0/0.0)

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

//*********************************************************************************************************
// JVM .class file related structs & types that represent a class file, with some extra info
// that can be statically resolved.
//*********************************************************************************************************

typedef struct _JavaClassInfo JavaClassInfo;

typedef enum {
    FIELD_ACC_PUBLIC = 0x0001,
    FIELD_ACC_PRIVATE = 0x0002,
    FIELD_ACC_PROTECTED = 0x0004,
    FIELD_ACC_STATIC = 0x0008,
    FIELD_ACC_FINAL = 0x0010,
    FIELD_ACC_VOLATILE = 0x0040,
    FIELD_ACC_TRANSIENT = 0x0080,
    FIELD_ACC_SYNTHETIC = 0x1000,
    FIELD_ACC_ENUM = 0x4000,
} FieldAccFlag;

/** Java .class field_info */
typedef struct {
    uint16_t accessFlags;
    C_CSTR name; // UTF-8 string of the field name
    C_CSTR descriptor; // Type descriptor of this field
    C_CSTR signature; // Signature of this field if it is generic type, or `NULL`

    // TODO: Add field attributes
} FieldInfo;

typedef struct {
    uint16_t fieldIndex; // The index of `JavaClassInfo.fields`

    JAVA_BOOLEAN isReference;
} PreResolvedStaticFieldInfo;

typedef struct {
    JavaClassInfo *declaringClass; // The class that declares this field, NULL means this class
    uint16_t fieldIndex; // The index of `JavaClassInfo.fields`

    JAVA_BOOLEAN isReference;
} PreResolvedInstanceFieldInfo;

typedef enum {
    METHOD_ACC_PUBLIC = 0x0001,
    METHOD_ACC_PRIVATE = 0x0002,
    METHOD_ACC_PROTECTED = 0x0004,
    METHOD_ACC_STATIC = 0x0008,
    METHOD_ACC_FINAL = 0x0010,
    METHOD_ACC_SYNCHRONIZED = 0x0020,
    METHOD_ACC_BRIDGE = 0x0040,
    METHOD_ACC_VARARGS = 0x0080,
    METHOD_ACC_NATIVE = 0x0100,
    METHOD_ACC_ABSTRACT = 0x0400,
    METHOD_ACC_STRICTFP = 0x0800,
    METHOD_ACC_SYNTHETIC = 0x1000,
} MethodAccFlag;

/** Java .class method_info */
typedef struct {
    uint16_t accessFlags;
    C_CSTR name; // UTF-8 string of the method name
    C_CSTR descriptor; // Type descriptor of this method
    C_CSTR signature; // Signature of this method if it contains generic type, or `NULL`

    void *code; // Pointer to the function
} MethodInfo;

/** A reference to a static field that used by this class */
typedef struct {
    JavaClassInfo *declaringClass; // The class that declares this field, NULL means this class
    uint16_t fieldIndex; // The index of `JavaClassInfo.preResolvedStaticFields`
} PreResolvedStaticFieldReference;

typedef enum {
    CLASS_ACC_PUBLIC = 0x0001,
    CLASS_ACC_FINAL = 0x0010,
    CLASS_ACC_SUPER = 0x0020,
    CLASS_ACC_INTERFACE = 0x0200,
    CLASS_ACC_ABSTRACT = 0x0400,
    CLASS_ACC_SYNTHETIC = 0x1000,
    CLASS_ACC_ANNOTATION = 0x2000,
    CLASS_ACC_ENUM = 0x4000,
} ClassAccFlag;

typedef JAVA_VOID (*ClassResolveHandler)(JAVA_CLASS c);

/** Stores info of a Java .class file */
struct _JavaClassInfo {
    uint16_t accessFlags;
    C_CSTR thisClass; // UTF-8 string of the fully qualified name of this class
    C_CSTR signature; // Signature of this class if it is generic type, or `NULL`
    JavaClassInfo *superClass; // The parent class
    uint16_t interfaceCount; // Number of direct interfaces
    JavaClassInfo **interfaces; // Array of direct interfaces

    // Fields of this class, fields from super class / interfaces not included.
    uint16_t fieldCount; // Number of fields of this class
    FieldInfo *fields;

    // Methods of this class, methods from super class / interfaces not included.
    uint16_t methodCount; // Number of methods of this class
    MethodInfo *methods;

    //*****************************************************************************************************
    // Info resolved by .class to c translator
    //*****************************************************************************************************
    ClassResolveHandler resolveHandler; // Function that handles class instance pre-init
    size_t classSize; // Size of the class struct
    size_t instanceSize; // Size of the object struct

    uint16_t preResolvedStaticFieldCount; // All static fields from this class ONLY
    PreResolvedStaticFieldInfo *preResolvedStaticFields;

    uint16_t preResolvedInstanceFieldCount; // Include ALL instance fields from super classes
    PreResolvedInstanceFieldInfo *preResolvedInstanceFields;

    uint16_t preResolvedStaticFieldRefCount; // All static field references used by current class
    PreResolvedStaticFieldReference *preResolvedStaticFieldReferences;

    // Reference to the finalizer method, or NULL if this class does not declare a finalizer.
    // If this class does not declare a finalizer but one of it's superclasses declares one, this
    // reference will point to that method.
    // The only exception will be the finalizer defined in java/lang/Object.
    JavaMethodRetVoid finalizer;
};

//*********************************************************************************************************
// Resolved runtime type info, contain data resolved by class loader
//*********************************************************************************************************

typedef struct {
    PreResolvedStaticFieldInfo *info;

    // GC scanning information
    size_t offset;
} ResolvedStaticField;

typedef struct {
    PreResolvedInstanceFieldInfo *info;

    // GC scanning information
    size_t offset; // Offset to where the field data is stored
} ResolvedInstanceField;

/** The reference to a static field, resolved from a symbolic reference. */
typedef struct {
    JAVA_CLASS clazz;
    uint16_t fieldIndex; // The index of `JavaClass.staticFields`
} ResolvedStaticFieldReference;

typedef enum {
    CLASS_STATE_ALLOCATED = 0,
    CLASS_STATE_REGISTERED,
    CLASS_STATE_RESOLVED,
    CLASS_STATE_INITIALIZING,
    CLASS_STATE_INITIALIZED,
    CLASS_STATE_ERROR
} ClassState;

struct _JavaClass {
    JAVA_CLASS clazz; // Always NULL for class instance
    void *monitor;

    ClassState state;
    JavaClassInfo *info;

    JAVA_OBJECT classLoader;
    // The real instance of a java/lang/Class
    JAVA_OBJECT classInstance;

    // Resolved data
    JAVA_CLASS superClass;
    int interfaceCount;
    JAVA_CLASS *interfaces;

    // Static fields of this class, fields from super class / interfaces not included.
    uint16_t staticFieldCount; // Number of resolved static fields of this class
    ResolvedStaticField *staticFields;
    JAVA_BOOLEAN hasStaticReference;

    uint32_t fieldCount; // Number of resolved instance fields, includes ALL fields from super classes and interfaces
    ResolvedInstanceField *fields;
    JAVA_BOOLEAN hasReference;

    uint32_t staticFieldRefCount; // Number of static field references used by this class
    ResolvedStaticFieldReference *staticFieldReferences;
};

// Object prototype
struct _JavaObject {
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

typedef enum {
    VM_TYPE_CAT_1,
    VM_TYPE_CAT_2,
} VMTypeCategory;

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
 *      1       0
 * +--------+--------+
 * | pinned | marked |
 * +--------+--------+
 */
#define OBJECT_FLAG_MASK ((uintptr_t)0x3) // lower 2 bits
#define OBJECT_FLAG_GC_MARKED ((uintptr_t)0x1)
#define OBJECT_FLAG_GC_PINNED ((uintptr_t)0x2)

static inline JAVA_CLASS obj_get_class(JAVA_OBJECT obj) {
    // The class instance is allocated with a alignment of `DATA_ALIGNMENT`
    // so the lower 2 bits (32-bit arch) or 3 bits (64-bit arch) of the pointer can be used as other
    // flags. When we want the real address of the class instance,
    // we need to zero out the lower bits
    return (void *) (((uintptr_t) obj->clazz) & (~OBJECT_FLAG_MASK));
}

static inline void obj_set_flags(JAVA_OBJECT obj, uintptr_t flags) {
    flags &= OBJECT_FLAG_MASK;
    obj->clazz = (void *) (((uintptr_t) obj->clazz) | flags);
}

static inline void obj_clear_flags(JAVA_OBJECT obj, uintptr_t flags) {
    flags &= OBJECT_FLAG_MASK;
    flags = ~flags;
    obj->clazz = (void *) (((uintptr_t) obj->clazz) & flags);
}

static inline void obj_reset_flags_masked(JAVA_OBJECT obj, uintptr_t mask, uintptr_t flags) {
    mask &= OBJECT_FLAG_MASK;
    flags &= mask;
    obj->clazz = (void *) ((((uintptr_t) obj->clazz) & (~mask)) | flags);
}

static inline void obj_reset_flags(JAVA_OBJECT obj, uintptr_t flags) {
    obj_reset_flags_masked(obj, OBJECT_FLAG_MASK, flags);
}

static inline uintptr_t obj_get_flags(JAVA_OBJECT obj) {
    uintptr_t ref = (uintptr_t) obj->clazz;

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

#define obj_is_marked(obj) obj_test_flags_and((obj), OBJECT_FLAG_GC_MARKED)
#define obj_set_marked(obj) obj_set_flags((obj), OBJECT_FLAG_GC_MARKED)
#define obj_clear_marked(obj) obj_clear_flags((obj), OBJECT_FLAG_GC_MARKED)
#define obj_is_pinned(obj) obj_test_flags_and((obj), OBJECT_FLAG_GC_PINNED)
#define obj_set_pinned(obj) obj_set_flags((obj), OBJECT_FLAG_GC_PINNED)
#define obj_clear_pinned(obj) obj_clear_flags((obj), OBJECT_FLAG_GC_PINNED)

// The minimum size of any managed object
#define MIN_OBJECT_SIZE ((size_t)sizeof(JavaObjectBase))


#endif //FOXVM_VM_BASE_H
