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

extern JavaClassInfo *foxvm_class_infos_rt[];

// A fake object for locking
static JavaObjectBase g_bootstrapClassLockObj = {0};
static VMStackSlot g_bootstrapClassLock = {0};

#define cached_class(var)                       \
static JavaClassInfo *g_classInfo_##var = NULL; \
static JAVA_CLASS     g_class_##var = NULL

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

cached_class(java_lang_ClassNotFoundException);
cached_class(java_lang_NoClassDefFoundError);
cached_class(java_lang_Error);

#undef cached_class

static JAVA_VOID resolve_handler_primitive(JAVA_CLASS c) {
    JavaClass *clazz = (JavaClass *) c;

    clazz->interfaceCount = 0;
    clazz->interfaces = NULL;

    clazz->staticFieldCount = 0;
    clazz->staticFields = NULL;
    clazz->hasStaticReference = JAVA_FALSE;

    clazz->fieldCount = 0;
    clazz->fields = NULL;
    clazz->hasReference = JAVA_FALSE;
}

#define def_prim_class(desc)                                                \
static JavaClassInfo g_classInfo_primitive_##desc = {                       \
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
    .preResolvedStaticFieldRefCount = 0,                                    \
    .preResolvedStaticFieldReferences = NULL,                               \
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

static MethodInfo *java_lang_Class_init = NULL;

typedef struct {
    void *key;
    JAVA_CLASS clazz;
    UT_hash_handle hh;
} LoadedClassEntry;
static LoadedClassEntry *g_loadedClasses = NULL;

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

static void cl_bootstrap_init_class_object(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classObject, JAVA_CLASS clazz) {
    stack_frame_start(-1, 2, 0);

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

static JAVA_CLASS cl_bootstrap_createPrimitiveClass(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *classInfo) {
    printf("Bootstrap Classloader: creating primitive class %s\n", classInfo->thisClass);

    LoadedClassEntry *entry = heap_alloc_uncollectable(vmCurrentContext, sizeof(LoadedClassEntry));
    if (!entry) {
        fprintf(stderr, "Bootstrap Classloader: unable to alloc LoadedClassEntry for class %s\n", classInfo->thisClass);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }

    JAVA_CLASS thisClass;
    JAVA_OBJECT classObject;
    if (cl_bootstrap_create_class(vmCurrentContext, classInfo, &thisClass, &classObject) != JAVA_TRUE) {
        heap_free_uncollectable(vmCurrentContext, entry);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }
    thisClass->isPrimitive = JAVA_TRUE;
    // We then register the class
    entry->key = classInfo;
    entry->clazz = thisClass;
    HASH_ADD_PTR(g_loadedClasses, key, entry);
    thisClass->state = CLASS_STATE_REGISTERED;

    // Primitive class does not need monitor

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

    // Make sure the class is initilized
    if (classloader_get_class(vmCurrentContext, JAVA_NULL, g_classInfo_java_lang_Class) == (JAVA_CLASS) JAVA_NULL) {
        fprintf(stderr, "Bootstrap Classloader: unable to initialize class java.lang.Class\n");
        return JAVA_FALSE;
    }
    // We here get the init method of the java/lang/Class
    for (uint32_t i = 0; i < g_classInfo_java_lang_Class->methodCount; i++) {
        MethodInfo *method = &g_classInfo_java_lang_Class->methods[i];
        if (strcmp(method->name, "<init>") == 0) {
            java_lang_Class_init = method;
            break;
        }
    }
    if (!java_lang_Class_init) {
        fprintf(stderr, "Bootstrap Classloader: unable to find method java.lang.Class.<init>\n");
        return JAVA_FALSE;
    }
    // Fix some fields that depends on the java/lang/Object and java/lang/Class
    LoadedClassEntry *cursor;
    for (cursor = g_loadedClasses; cursor != NULL; cursor = cursor->hh.next) {
        JAVA_CLASS c = cursor->clazz;
        c->classInstance->clazz = g_class_java_lang_Class;

        cl_bootstrap_init_class_object(vmCurrentContext, c->classInstance, c);
    }
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

    cache_class(java_lang_ClassNotFoundException, "java/lang/ClassNotFoundException");
    cache_class(java_lang_NoClassDefFoundError, "java/lang/NoClassDefFoundError");
    cache_class(java_lang_Error, "java/lang/Error");

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

    // We first create the holder for this class. This is done before allocating the actual class
    // because this could trigger a GC and we don't want the newly allocated class got freed before
    // it's registered.
    LoadedClassEntry *entry = heap_alloc_uncollectable(vmCurrentContext, sizeof(LoadedClassEntry));
    if (!entry) {
        monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
        fprintf(stderr, "Bootstrap Classloader: unable to alloc LoadedClassEntry for class %s\n", classInfo->thisClass);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }

    // First we check if superclasses and superinterfaces are loaded
    JAVA_CLASS superclass = (JAVA_CLASS) JAVA_NULL;
    if (classInfo->superClass) {
        superclass = cl_bootstrap_find_class_by_info(vmCurrentContext, classInfo->superClass);
        if (!superclass || superclass->state < CLASS_STATE_RESOLVED) {
            monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
            heap_free_uncollectable(vmCurrentContext, entry);
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
        heap_free_uncollectable(vmCurrentContext, entry);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }
    thisClass->isPrimitive = JAVA_FALSE;

    // We then register the class
    entry->key = classInfo;
    entry->clazz = thisClass;
    HASH_ADD_PTR(g_loadedClasses, key, entry);
    thisClass->state = CLASS_STATE_REGISTERED;

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

    // Init the java/lang/Class instance
    if (java_lang_Class_init) {
        cl_bootstrap_init_class_object(vmCurrentContext, classObject, thisClass);
    }

    // Mark current class as resolved
    thisClass->state = CLASS_STATE_RESOLVED;

    monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
    return thisClass;
}

JAVA_CLASS cl_bootstrap_find_class(VM_PARAM_CURRENT_CONTEXT, C_CSTR className) {
    if (className[0] == '[') {

    }

    // Find class info by class name
    JavaClassInfo *info = cl_bootstrap_class_info_lookup(className);
    if (info) {
        return cl_bootstrap_find_class_by_info(vmCurrentContext, info);
    }

    return (JAVA_CLASS) JAVA_NULL;
}
