//
// Created by noisyfox on 2020/8/27.
//

#include "vm_boot_classloader.h"
#include "vm_classloader.h"
#include "vm_thread.h"
#include "vm_gc.h"
#include <string.h>
#include "uthash.h"
#include <stdio.h>
#include "vm_bytecode.h"
#include "vm_array.h"
#include "vm_method.h"

extern JavaClassInfo *foxvm_class_infos_rt[];

// A fake object for locking
static JavaObjectBase g_bootstrapClassLockObj = {0};
static VMStackSlot g_bootstrapClassLock = {0};

// A fake object for array cache locking
static JavaObjectBase g_bootstrapArrayClassLockObj = {0};
static VMStackSlot g_bootstrapArrayClassLock = {0};

#define cached_class(var)                \
JavaClassInfo *g_classInfo_##var = NULL; \
JAVA_CLASS     g_class_##var = NULL

cached_class(java_lang_Object);
cached_class(java_lang_Class);
cached_class(java_lang_ClassLoader);
cached_class(java_lang_String);
cached_class(java_lang_Boolean);
cached_class(java_lang_Byte);
cached_class(java_lang_Character);
cached_class(java_lang_Short);
cached_class(java_lang_Integer);
cached_class(java_lang_Long);
cached_class(java_lang_Float);
cached_class(java_lang_Double);
cached_class(java_lang_Enum);
cached_class(java_lang_Cloneable);
cached_class(java_lang_Thread);
cached_class(java_lang_ThreadGroup);
cached_class(java_io_Serializable);
cached_class(java_lang_Runtime);

// Errors
cached_class(java_lang_Error);
cached_class(java_lang_NoClassDefFoundError);
cached_class(java_lang_NoSuchFieldError);
cached_class(java_lang_NoSuchMethodError);
cached_class(java_lang_IncompatibleClassChangeError);
cached_class(java_lang_UnsatisfiedLinkError);
cached_class(java_lang_InstantiationError);

// Exceptions
cached_class(java_lang_Throwable);
cached_class(java_lang_RuntimeException);
cached_class(java_lang_NullPointerException);
cached_class(java_lang_ArrayIndexOutOfBoundsException);
cached_class(java_lang_ClassNotFoundException);
cached_class(java_lang_NegativeArraySizeException);
cached_class(java_lang_InstantiationException);

#undef cached_class

static JAVA_VOID resolve_handler_primitive(JAVA_CLASS c) {
    c->interfaceCount = 0;
    c->interfaces = NULL;

    c->staticFieldCount = 0;
    c->staticFields = NULL;
    c->hasStaticReference = JAVA_FALSE;

    c->fieldCount = 0;
    c->fields = NULL;
    c->hasReference = JAVA_FALSE;
}

#define def_prim_class(desc)                                                \
JavaClassInfo g_classInfo_primitive_##desc = {                              \
    .accessFlags = CLASS_ACC_PUBLIC | CLASS_ACC_FINAL | CLASS_ACC_SUPER,    \
    .thisClass = #desc,                                                     \
    .signature = NULL,                                                      \
    .superClass = NULL,                                                     \
    .interfaceCount = 0,                                                    \
    .interfaces = NULL,                                                     \
    .fieldCount = 0,                                                        \
    .fields = NULL,                                                         \
    .methodCount = 0,                                                       \
    .methods = 0,                                                           \
                                                                            \
    .resolveHandler = resolve_handler_primitive,                            \
    .classSize = sizeof(JavaClass),                                         \
    .instanceSize = 0,                                                      \
                                                                            \
    .preResolvedStaticFieldCount = 0,                                       \
    .preResolvedStaticFields = NULL,                                        \
    .preResolvedInstanceFieldCount = 0,                                     \
    .preResolvedInstanceFields = NULL,                                      \
                                                                            \
    .clinit = NULL,                                                         \
    .finalizer = NULL,                                                      \
};                                                                          \
static JAVA_CLASS g_class_primitive_##desc = NULL

def_prim_class(Z);
def_prim_class(B);
def_prim_class(C);
def_prim_class(S);
def_prim_class(I);
def_prim_class(J);
def_prim_class(F);
def_prim_class(D);
def_prim_class(V);

#undef def_prim_class

JavaClass* g_class_array_Z = NULL;
JavaClass* g_class_array_B = NULL;
JavaClass* g_class_array_C = NULL;
JavaClass* g_class_array_S = NULL;
JavaClass* g_class_array_I = NULL;
JavaClass* g_class_array_J = NULL;
JavaClass* g_class_array_F = NULL;
JavaClass* g_class_array_D = NULL;

static MethodInfo *java_lang_Class_init = NULL;

typedef struct {
    void *key;
    JAVA_CLASS clazz;
    UT_hash_handle hh;
} LoadedClassEntry;
static LoadedClassEntry *g_loadedClasses = NULL;

typedef struct {
    C_CSTR key;
    JAVA_CLASS clazz;
    UT_hash_handle hh;
} LoadedArrayClassEntry;
static LoadedArrayClassEntry *g_loadedArrayClasses = NULL;

// Interfaces that implemented by all arrays:
// java/lang/Cloneable and java/io/Serializable
static JavaClassInfo *g_array_interfaces[2] = {
        NULL, NULL
};
static JavaClass *g_array_interfaces_resolved[2] = {
        NULL, NULL
};

static JavaClassInfo g_classInfo_array_prototype;

// Methods of arrays
MethodInfo g_array_methodInfo_5Mclone_R9Pjava_lang6CObject = {
        .accessFlags = METHOD_ACC_PUBLIC | METHOD_ACC_FINAL,
        .name = "clone",
        .descriptor = "()Ljava/lang/Object;",
        .signature = NULL,
        .declaringClass = &g_classInfo_array_prototype,
        .code = g_array_5Mclone_R9Pjava_lang6CObject,
};
static MethodInfo* g_array_methods[] = {
        &g_array_methodInfo_5Mclone_R9Pjava_lang6CObject
};

// vtable of the array classes. It has the same length as the vtable of java/lang/Object,
// all methods inherent from it, except the clone() method.
#define VTABLE_COUNT_java_lang_object 11
static VTableItem g_array_vtable[VTABLE_COUNT_java_lang_object] = {0};

static JAVA_VOID resolve_handler_array(JAVA_CLASS c) {
    JavaArrayClass *clazz = (JavaArrayClass *) c;

    c->interfaceCount = 2;
    c->interfaces = g_array_interfaces_resolved;

    c->staticFieldCount = 0;
    c->staticFields = NULL;
    c->hasStaticReference = JAVA_FALSE;

    c->fieldCount = 0;
    c->fields = NULL;
    c->hasReference = JAVA_FALSE;

    // Copy the prototype to the classInfo field in [JavaArrayClass]
    memcpy(&clazz->classInfo, c->info, sizeof(JavaClassInfo));

    // Fix the info ref
    c->info = &clazz->classInfo;
}

// The prototype of all array class infos
static JavaClassInfo g_classInfo_array_prototype = {
        .accessFlags = CLASS_ACC_PUBLIC | CLASS_ACC_FINAL,
        .thisClass = "[",
        .signature = NULL,
        .superClass = NULL, // Assigned in [cl_bootstrap_init]
        .interfaceCount = 2,
        .interfaces = g_array_interfaces,
        .fieldCount = 0,
        .fields = NULL,
        .methodCount = 1,
        .methods = g_array_methods,

        .resolveHandler = resolve_handler_array,
        .classSize = sizeof(JavaArrayClass), // This is the most important part
        .instanceSize = 0, // This is not used for array types, use [array_size_of_type] instead

        .preResolvedStaticFieldCount = 0,
        .preResolvedStaticFields = NULL,
        .preResolvedInstanceFieldCount = 0,
        .preResolvedInstanceFields = NULL,

        .vtableCount = VTABLE_COUNT_java_lang_object,
        .vtable = g_array_vtable,

        .clinit = NULL,
        .finalizer = NULL,
};

/** Find class info by class name */
static JavaClassInfo *cl_bootstrap_class_info_lookup(C_CSTR className) {
    JavaClassInfo **cursor = foxvm_class_infos_rt;
    JavaClassInfo *currentInfo = *cursor;
    while (currentInfo != NULL) {
        // Compare the class name
        if (strcmp(className, currentInfo->thisClass) == 0) {
            return currentInfo;
        }

        cursor++;
        currentInfo = *cursor;
    }

    return NULL;
}

static JavaClassInfo *cl_bootstrap_class_info_lookup_by_descriptor(C_CSTR desc) {
    assert(desc[0] == TYPE_DESC_REFERENCE);
    size_t len = strlen(desc);
    assert(desc[len - 1] == ';');

    desc++; // Skip the first L
    len -= 2; // Then also remove the last ;

    JavaClassInfo **cursor = foxvm_class_infos_rt;
    JavaClassInfo *currentInfo = *cursor;
    while (currentInfo != NULL) {
        // Compare the class name
        if (strncmp(desc, currentInfo->thisClass, len) == 0) {
            return currentInfo;
        }

        cursor++;
        currentInfo = *cursor;
    }

    return NULL;
}

#define load_class_info(var, className) do {                                                            \
    var = cl_bootstrap_class_info_lookup(className);                                                    \
    if (!var) {                                                                                         \
        fprintf(stderr, "Bootstrap Classloader: unable to find class info for class %s\n", className);  \
        return JAVA_FALSE;                                                                              \
    }                                                                                                   \
} while(0)

#define load_class(var, classInfo) do {                                                                 \
    var = cl_bootstrap_find_class_by_info(vmCurrentContext, classInfo);                                 \
    if (!var) {                                                                                         \
        fprintf(stderr, "Bootstrap Classloader: unable to load class %s\n", classInfo->thisClass);      \
        return JAVA_FALSE;                                                                              \
    }                                                                                                   \
} while(0)

#define cache_class(var, className) do {            \
    load_class_info(g_classInfo_##var, className);  \
    load_class(g_class_##var, g_classInfo_##var);   \
} while(0)

#define cache_array_class(desc) do {                                                        \
    g_class_array_##desc = cl_bootstrap_find_class(vmCurrentContext, "["#desc);             \
    if (!g_class_array_##desc) {                                                            \
        fprintf(stderr, "Bootstrap Classloader: unable to load array class [%s\n", #desc);  \
        return JAVA_FALSE;                                                                  \
    }                                                                                       \
} while(0)

static void cl_bootstrap_init_class_object(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classObject, JAVA_CLASS clazz) {
    stack_frame_start(NULL, 2, 0);

    stack_push_object(classObject);
    stack_push_object((JAVA_OBJECT) clazz);

    // Simulate a invoke special
    bc_invoke_special(java_lang_Class_init->code);

    stack_frame_end();
}

static JAVA_BOOLEAN cl_bootstrap_create_class(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *classInfo,
                                              JAVA_CLASS *classRefOut, JAVA_OBJECT *classObjOut) {
    // The required size can store two structures: one for the JAVA_CLASS corresponding to the
    // classInfo, and an object instance of the java/lang/Class.
    size_t requiredSize = align_size_up(classInfo->classSize, SIZE_ALIGNMENT) +
                          align_size_up(g_classInfo_java_lang_Class->instanceSize, SIZE_ALIGNMENT);
    JAVA_CLASS thisClass = heap_alloc_loh(vmCurrentContext, requiredSize);
    if (!thisClass) {
        fprintf(stderr, "Bootstrap Classloader: unable to alloc class %s\n", classInfo->thisClass);
        return JAVA_FALSE;
    }
    thisClass->info = classInfo;

    JAVA_OBJECT classObject = ptr_inc(thisClass, align_size_up(classInfo->classSize, SIZE_ALIGNMENT));

    // this could be null when the java/lang/Class is not inited. Will fix after java/lang/Class is loaded.
    classObject->clazz = g_class_java_lang_Class;
    thisClass->classInstance = classObject;

    *classRefOut = thisClass;
    *classObjOut = classObject;

    return JAVA_TRUE;
}

// Register non-array class to the bootstrap class registry
static JAVA_BOOLEAN cl_bootstrap_register_class(VM_PARAM_CURRENT_CONTEXT, JAVA_CLASS clazz) {
    LoadedClassEntry *entry = heap_alloc_uncollectable(sizeof(LoadedClassEntry));
    if (!entry) {
        fprintf(stderr, "Bootstrap Classloader: unable to alloc LoadedClassEntry for class %s\n",
                clazz->info->thisClass);
        // TODO: throw OOM exception
        return JAVA_FALSE;
    }
    entry->key = clazz->info;
    entry->clazz = clazz;
    HASH_ADD_PTR(g_loadedClasses, key, entry);
    clazz->state = CLASS_STATE_REGISTERED;

    return JAVA_TRUE;
}

static JAVA_CLASS cl_bootstrap_createPrimitiveClass(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *classInfo) {
    printf("Bootstrap Classloader: creating primitive class %s\n", classInfo->thisClass);

    JAVA_CLASS thisClass;
    JAVA_OBJECT classObject;
    if (cl_bootstrap_create_class(vmCurrentContext, classInfo, &thisClass, &classObject) != JAVA_TRUE) {
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }
    thisClass->isPrimitive = JAVA_TRUE;
    thisClass->classLoader = JAVA_NULL;

    // We then register the class
    if(!cl_bootstrap_register_class(vmCurrentContext, thisClass)) {
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }

    // Then we pre-init the class
    VMStackSlot thisSlot;
    thisSlot.data.o = (JAVA_OBJECT) thisClass;
    thisSlot.type = VM_SLOT_OBJECT;
    if (monitor_create(vmCurrentContext, &thisSlot) != thrd_success) {
        thisClass->state = CLASS_STATE_ERROR;
        fprintf(stderr, "Bootstrap Classloader: unable to create monitor for class %s\n", classInfo->thisClass);
        // TODO: throw exception
        return (JAVA_CLASS) JAVA_NULL;
    }
    classInfo->resolveHandler(thisClass);
    thisClass->superClass = NULL;

    // Init the java/lang/Class instance
    if (java_lang_Class_init) {
        cl_bootstrap_init_class_object(vmCurrentContext, classObject, thisClass);
    }

    // Mark current class as resolved
    thisClass->state = CLASS_STATE_RESOLVED;

    return thisClass;
}

#define create_prim_class(desc) do {                                                                                                \
    g_class_primitive_##desc = cl_bootstrap_createPrimitiveClass(vmCurrentContext, &g_classInfo_primitive_##desc);                  \
    if (!g_class_primitive_##desc) {                                                                                                \
        fprintf(stderr, "Bootstrap Classloader: unable to create primitive class %s\n", g_classInfo_primitive_##desc.thisClass);    \
        return JAVA_FALSE;                                                                                                          \
    }                                                                                                                               \
} while(0)

JAVA_BOOLEAN cl_bootstrap_init(VM_PARAM_CURRENT_CONTEXT) {
    g_bootstrapClassLock.data.o = &g_bootstrapClassLockObj;
    g_bootstrapClassLock.type = VM_SLOT_OBJECT;
    if (monitor_create(vmCurrentContext, &g_bootstrapClassLock) != thrd_success) {
        return JAVA_FALSE;
    }
    g_bootstrapArrayClassLock.data.o = &g_bootstrapArrayClassLockObj;
    g_bootstrapArrayClassLock.type = VM_SLOT_OBJECT;
    if (monitor_create(vmCurrentContext, &g_bootstrapArrayClassLock) != thrd_success) {
        return JAVA_FALSE;
    }
    // Find essential class infos
    // This info is required by class creation, so we need to load it first
    load_class_info(g_classInfo_java_lang_Class, "java/lang/Class");

    // Init essential classes
    cache_class(java_lang_Object, "java/lang/Object");
    load_class(g_class_java_lang_Class, g_classInfo_java_lang_Class);

    // Create primitive classes
    create_prim_class(Z);
    create_prim_class(B);
    create_prim_class(C);
    create_prim_class(S);
    create_prim_class(I);
    create_prim_class(J);
    create_prim_class(F);
    create_prim_class(D);
    create_prim_class(V);

    // Classes that required by array types
    cache_class(java_lang_Cloneable, "java/lang/Cloneable");
    cache_class(java_io_Serializable, "java/io/Serializable");
    g_array_interfaces[0] = g_classInfo_java_lang_Cloneable;
    g_array_interfaces[1] = g_classInfo_java_io_Serializable;
    g_array_interfaces_resolved[0] = g_class_java_lang_Cloneable;
    g_array_interfaces_resolved[1] = g_class_java_io_Serializable;
    g_classInfo_array_prototype.superClass = g_classInfo_java_lang_Object;
    // Init vtable for array class
    assert(VTABLE_COUNT_java_lang_object == g_classInfo_java_lang_Object->vtableCount);
    memcpy(g_array_vtable, g_classInfo_java_lang_Object->vtable, sizeof(g_array_vtable));
    JAVA_BOOLEAN arrayCloneUpdated = JAVA_FALSE;
    for (int i = 0; i < VTABLE_COUNT_java_lang_object; i++) {
        VTableItem *item = &g_array_vtable[i];
        assert(item->declaringClass == NULL);
        // Get the name of the method
        MethodInfo *method = g_classInfo_java_lang_Object->methods[item->methodIndex];
        C_CSTR methodName = method->name;
        if (strcmp("clone", methodName) == 0) {
            assert(arrayCloneUpdated == JAVA_FALSE);
            // Update the clone method
            // make sure to match [g_array_methods]
            item->methodIndex = 0;
            item->code = g_array_5Mclone_R9Pjava_lang6CObject;
            arrayCloneUpdated = JAVA_TRUE;
        } else {
            // Not [clone], so we update the declaringClass instead
            item->declaringClass = g_classInfo_java_lang_Object;
        }
    }
    assert(arrayCloneUpdated == JAVA_TRUE);

    // Then we create the array classes for primitive types
    cache_array_class(Z);
    cache_array_class(B);
    cache_array_class(C);
    cache_array_class(S);
    cache_array_class(I);
    cache_array_class(J);
    cache_array_class(F);
    cache_array_class(D);

    // Make sure the class is initialized
    if (classloader_get_class_init(vmCurrentContext, JAVA_NULL, g_classInfo_java_lang_Class) == (JAVA_CLASS) JAVA_NULL) {
        fprintf(stderr, "Bootstrap Classloader: unable to initialize class java.lang.Class\n");
        return JAVA_FALSE;
    }
    // We here get the init method of the java/lang/Class
    java_lang_Class_init = method_find(g_classInfo_java_lang_Class, "<init>", "(Ljava/lang/Object;)V");
    if (!java_lang_Class_init) {
        fprintf(stderr, "Bootstrap Classloader: unable to find method java.lang.Class.<init>(Ljava/lang/Object;)V\n");
        return JAVA_FALSE;
    }
    // Fix some fields that depends on the java/lang/Object and java/lang/Class
    #define fix_class(c) do {                                                   \
        c->classInstance->clazz = g_class_java_lang_Class;                      \
        cl_bootstrap_init_class_object(vmCurrentContext, c->classInstance, c);  \
    } while(0)
    {
        LoadedClassEntry *cursor;
        for (cursor = g_loadedClasses; cursor != NULL; cursor = cursor->hh.next) {
            JAVA_CLASS c = cursor->clazz;
            fix_class(c);
        }
    }
    {
        LoadedArrayClassEntry *cursor;
        for (cursor = g_loadedArrayClasses; cursor != NULL; cursor = cursor->hh.next) {
            JAVA_CLASS c = cursor->clazz;
            fix_class(c);
        }
    }
    fix_class(g_class_primitive_Z);
    fix_class(g_class_primitive_B);
    fix_class(g_class_primitive_C);
    fix_class(g_class_primitive_S);
    fix_class(g_class_primitive_I);
    fix_class(g_class_primitive_J);
    fix_class(g_class_primitive_F);
    fix_class(g_class_primitive_D);
    #undef fix_class

    cache_class(java_lang_ClassLoader, "java/lang/ClassLoader");
    cache_class(java_lang_String, "java/lang/String");
    cache_class(java_lang_Boolean, "java/lang/Boolean");
    cache_class(java_lang_Byte, "java/lang/Byte");
    cache_class(java_lang_Character, "java/lang/Character");
    cache_class(java_lang_Short, "java/lang/Short");
    cache_class(java_lang_Integer, "java/lang/Integer");
    cache_class(java_lang_Long, "java/lang/Long");
    cache_class(java_lang_Float, "java/lang/Float");
    cache_class(java_lang_Double, "java/lang/Double");
    cache_class(java_lang_Enum, "java/lang/Enum");
    cache_class(java_lang_Thread, "java/lang/Thread");
    cache_class(java_lang_ThreadGroup, "java/lang/ThreadGroup");
    cache_class(java_lang_Runtime, "java/lang/Runtime");

    cache_class(java_lang_Error, "java/lang/Error");
    cache_class(java_lang_NoClassDefFoundError, "java/lang/NoClassDefFoundError");
    cache_class(java_lang_NoSuchFieldError, "java/lang/NoSuchFieldError");
    cache_class(java_lang_NoSuchMethodError, "java/lang/NoSuchMethodError");
    cache_class(java_lang_IncompatibleClassChangeError, "java/lang/IncompatibleClassChangeError");
    cache_class(java_lang_UnsatisfiedLinkError, "java/lang/UnsatisfiedLinkError");
    cache_class(java_lang_InstantiationError, "java/lang/InstantiationError");

    cache_class(java_lang_Throwable, "java/lang/Throwable");
    cache_class(java_lang_RuntimeException, "java/lang/RuntimeException");
    cache_class(java_lang_NullPointerException, "java/lang/NullPointerException");
    cache_class(java_lang_ArrayIndexOutOfBoundsException, "java/lang/ArrayIndexOutOfBoundsException");
    cache_class(java_lang_ClassNotFoundException, "java/lang/ClassNotFoundException");
    cache_class(java_lang_NegativeArraySizeException, "java/lang/NegativeArraySizeException");
    cache_class(java_lang_InstantiationException, "java/lang/InstantiationException");

    return JAVA_TRUE;
}

JAVA_CLASS cl_bootstrap_get_loaded_class(JavaClassInfo *classInfo) {
    LoadedClassEntry *entry;
    HASH_FIND_PTR(g_loadedClasses, &classInfo, entry);

    return entry ? entry->clazz : (JAVA_CLASS) JAVA_NULL;
}

JAVA_CLASS cl_bootstrap_find_class_by_info(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *classInfo) {
    int ret = monitor_enter(vmCurrentContext, &g_bootstrapClassLock);
    if (ret != thrd_success) {
        return (JAVA_CLASS) JAVA_NULL;
    }

    // First see if the class is already loaded
    JAVA_CLASS clazz = cl_bootstrap_get_loaded_class(classInfo);
    if (clazz != (JAVA_CLASS) JAVA_NULL) {
        monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
        return clazz;
    }

    // Start loading class
    printf("Bootstrap Classloader: loading class %s\n", classInfo->thisClass);

    // First we check if superclasses and superinterfaces are loaded
    JAVA_CLASS superclass = (JAVA_CLASS) JAVA_NULL;
    if (classInfo->superClass) {
        superclass = cl_bootstrap_find_class_by_info(vmCurrentContext, classInfo->superClass);
        if (!superclass || superclass->state < CLASS_STATE_RESOLVED) {
            monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
            fprintf(stderr, "Bootstrap Classloader: unable to load superclass %s\n", classInfo->superClass->thisClass);
            // TODO: throw exception
            return (JAVA_CLASS) JAVA_NULL;
        }
    }

    // We need space for storing the references to interfaces, so we delay interface resolving after
    // allocating the space for the class.
    JAVA_CLASS thisClass;
    JAVA_OBJECT classObject;
    if (cl_bootstrap_create_class(vmCurrentContext, classInfo, &thisClass, &classObject) != JAVA_TRUE) {
        monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }
    thisClass->isPrimitive = JAVA_FALSE;
    thisClass->classLoader = JAVA_NULL;

    // We then register the class
    if(!cl_bootstrap_register_class(vmCurrentContext, thisClass)) {
        monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }

    // Then we pre-init the class
    VMStackSlot thisSlot;
    thisSlot.data.o = (JAVA_OBJECT) thisClass;
    thisSlot.type = VM_SLOT_OBJECT;
    if (monitor_create(vmCurrentContext, &thisSlot) != thrd_success) {
        thisClass->state = CLASS_STATE_ERROR;
        monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
        fprintf(stderr, "Bootstrap Classloader: unable to create monitor for class %s\n", classInfo->thisClass);
        // TODO: throw exception
        return (JAVA_CLASS) JAVA_NULL;
    }
    classInfo->resolveHandler(thisClass);
    thisClass->superClass = superclass;
    // Then resolve direct superinterfaces
    thisClass->interfaceCount = classInfo->interfaceCount;
    if (classInfo->interfaceCount > 0) {
        for (int i = 0; i < classInfo->interfaceCount; i++) {
            JAVA_CLASS it = cl_bootstrap_find_class_by_info(vmCurrentContext, classInfo->interfaces[i]);
            if (!it || it->state < CLASS_STATE_RESOLVED) {
                thisClass->state = CLASS_STATE_ERROR;
                monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
                fprintf(stderr, "Bootstrap Classloader: unable to load superinterface %s\n",
                        classInfo->interfaces[i]->thisClass);
                // TODO: throw exception
                return (JAVA_CLASS) JAVA_NULL;
            }
            thisClass->interfaces[i] = it;
        }
    }

    if(classloader_prepare_fields(vmCurrentContext, thisClass) != JAVA_TRUE) {
        thisClass->state = CLASS_STATE_ERROR;
        monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
        // TODO: throw exception
        return (JAVA_CLASS) JAVA_NULL;
    }

    // Init the java/lang/Class instance
    if (java_lang_Class_init) {
        cl_bootstrap_init_class_object(vmCurrentContext, classObject, thisClass);
    }

    // Mark current class as resolved
    thisClass->state = CLASS_STATE_RESOLVED;

    monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
    return thisClass;
}

static JAVA_CLASS cl_bootstrap_find_class_by_descriptor(VM_PARAM_CURRENT_CONTEXT, C_CSTR desc) {
    switch (desc[0]) {
        case TYPE_DESC_BOOLEAN: return g_class_primitive_Z;
        case TYPE_DESC_BYTE:    return g_class_primitive_B;
        case TYPE_DESC_CHAR:    return g_class_primitive_C;
        case TYPE_DESC_SHORT:   return g_class_primitive_S;
        case TYPE_DESC_INT:     return g_class_primitive_I;
        case TYPE_DESC_LONG:    return g_class_primitive_J;
        case TYPE_DESC_FLOAT:   return g_class_primitive_F;
        case TYPE_DESC_DOUBLE:  return g_class_primitive_D;
        case TYPE_DESC_VOID:    return g_class_primitive_V;
        case TYPE_DESC_ARRAY:   return cl_bootstrap_find_class(vmCurrentContext, desc);
    }
    // Find the class info by the descriptor
    JavaClassInfo *info = cl_bootstrap_class_info_lookup_by_descriptor(desc);
    if (info) {
        return cl_bootstrap_find_class_by_info(vmCurrentContext, info);
    }

    return (JAVA_CLASS) JAVA_NULL;
}

static JAVA_CLASS cl_bootstrap_find_array_class(VM_PARAM_CURRENT_CONTEXT, C_CSTR desc) {
    assert(desc[0] == TYPE_DESC_ARRAY);

    int ret = monitor_enter(vmCurrentContext, &g_bootstrapArrayClassLock);
    if (ret != thrd_success) {
        return (JAVA_CLASS) JAVA_NULL;
    }

    // Find class in dynamic created array cache
    {
        LoadedArrayClassEntry *entry;
        HASH_FIND_STR(g_loadedArrayClasses, desc, entry);
        if (entry) {
            JAVA_CLASS c = entry->clazz;
            monitor_exit(vmCurrentContext, &g_bootstrapArrayClassLock);
            return c;
        }
    }

    // Need to create one
    printf("Bootstrap Classloader: creating array class %s\n", desc);
    JAVA_CLASS componentType = cl_bootstrap_find_class_by_descriptor(vmCurrentContext, &desc[1]);
    if (componentType == g_class_primitive_V) {
        componentType = (JAVA_CLASS) JAVA_NULL;
    }
    if (componentType == (JAVA_CLASS) JAVA_NULL) {
        monitor_exit(vmCurrentContext, &g_bootstrapArrayClassLock);
        fprintf(stderr, "Bootstrap Classloader: unable to create array %s: component type %s not found\n",
                desc, &desc[1]);
        // TODO: throw exception
        return NULL;
    }

    // Create a new JavaArrayClass for this class, using the prototype info
    JAVA_CLASS thisClass;
    JAVA_OBJECT classObject;
    if (cl_bootstrap_create_class(vmCurrentContext, &g_classInfo_array_prototype, &thisClass, &classObject) !=
        JAVA_TRUE) {
        monitor_exit(vmCurrentContext, &g_bootstrapArrayClassLock);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }
    thisClass->isPrimitive = JAVA_FALSE;
    thisClass->classLoader = JAVA_NULL;

    JavaArrayClass *arrayClass = (JavaArrayClass *) thisClass;
    arrayClass->componentType = componentType;

    // Make a copy of the class desc string
    C_CSTR desc_dup = strdup(desc);
    if (!desc_dup) {
        monitor_exit(vmCurrentContext, &g_bootstrapArrayClassLock);
        fprintf(stderr, "Bootstrap Classloader: unable to duplicate class descriptor %s\n", desc);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }

    {
        // We then register the class
        LoadedArrayClassEntry *entry = heap_alloc_uncollectable(sizeof(LoadedArrayClassEntry));
        if (!entry) {
            monitor_exit(vmCurrentContext, &g_bootstrapArrayClassLock);
            free((void *) desc_dup);
            fprintf(stderr, "Bootstrap Classloader: unable to alloc LoadedArrayClassEntry for array class %s\n", desc);
            // TODO: throw OOM exception
            return (JAVA_CLASS) JAVA_NULL;
        }
        entry->key = desc_dup; // <- here we keep the reference to the duplicated string, so it won't leak
        entry->clazz = thisClass;
        HASH_ADD_STR(g_loadedArrayClasses, key, entry);
        thisClass->state = CLASS_STATE_REGISTERED;
    }

    // Then we pre-init the class
    VMStackSlot thisSlot;
    thisSlot.data.o = (JAVA_OBJECT) thisClass;
    thisSlot.type = VM_SLOT_OBJECT;
    if (monitor_create(vmCurrentContext, &thisSlot) != thrd_success) {
        thisClass->state = CLASS_STATE_ERROR;
        monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
        fprintf(stderr, "Bootstrap Classloader: unable to create monitor for array class %s\n", desc);
        // TODO: throw exception
        return (JAVA_CLASS) JAVA_NULL;
    }
    g_classInfo_array_prototype.resolveHandler(thisClass);
    // The we assign the class name
    thisClass->info->thisClass = desc_dup;

    // Init the java/lang/Class instance
    if (java_lang_Class_init) {
        cl_bootstrap_init_class_object(vmCurrentContext, classObject, thisClass);
    }

    // Mark current class as resolved
    thisClass->state = CLASS_STATE_RESOLVED;

    monitor_exit(vmCurrentContext, &g_bootstrapArrayClassLock);
    return thisClass;
}

JAVA_CLASS cl_bootstrap_find_class(VM_PARAM_CURRENT_CONTEXT, C_CSTR className) {
    if (className[0] == TYPE_DESC_ARRAY) {
        return cl_bootstrap_find_array_class(vmCurrentContext, className);
    }

    // Find class info by class name
    JavaClassInfo *info = cl_bootstrap_class_info_lookup(className);
    if (info) {
        return cl_bootstrap_find_class_by_info(vmCurrentContext, info);
    }

    return (JAVA_CLASS) JAVA_NULL;
}
