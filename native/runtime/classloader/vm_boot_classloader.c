//
// Created by noisyfox on 2020/8/27.
//

#include "vm_boot_classloader.h"
#include "vm_thread.h"
#include "vm_gc.h"
#include <string.h>
#include "uthash.h"
#include <stdio.h>

extern JavaClassInfo *foxvm_class_infos_rt[];

// A fake object for locking
static JavaObjectBase g_bootstrapClassLockObj = {0};
static VMStackSlot g_bootstrapClassLock = {0};

static JavaClassInfo *g_classInfo_java_lang_Object = NULL;
static JavaClassInfo *g_classInfo_java_lang_Class = NULL;
static JavaClassInfo *g_classInfo_java_lang_String = NULL;
static JAVA_CLASS g_class_java_lang_Object = NULL;
static JAVA_CLASS g_class_java_lang_Class = NULL;
static JAVA_CLASS g_class_java_lang_String = NULL;

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

JAVA_BOOLEAN cl_bootstrap_init(VM_PARAM_CURRENT_CONTEXT) {
    g_bootstrapClassLock.data.o = &g_bootstrapClassLockObj;
    g_bootstrapClassLock.type = VM_SLOT_OBJECT;
    if (monitor_create(vmCurrentContext, &g_bootstrapClassLock) != thrd_success) {
        return JAVA_FALSE;
    }

    // Find essential class infos
    load_class_info(g_classInfo_java_lang_Object, "java/lang/Object");
    load_class_info(g_classInfo_java_lang_Class, "java/lang/Class");
    load_class_info(g_classInfo_java_lang_String, "java/lang/String");

    // Init essential classes
    load_class(g_class_java_lang_Object, g_classInfo_java_lang_Object);
    load_class(g_class_java_lang_Class, g_classInfo_java_lang_Class);
    // Fix some fields that depends on the java/lang/Object and java/lang/Class
    LoadedClassEntry *cursor;
    for (cursor = g_loadedClasses; cursor != NULL; cursor = cursor->hh.next) {
        JAVA_CLASS c = cursor->clazz;
        c->classInstance->clazz = g_class_java_lang_Class;
    }
    load_class(g_class_java_lang_String, g_classInfo_java_lang_String);

    return JAVA_TRUE;
}

JAVA_CLASS cl_bootstrap_get_loaded_class(JavaClassInfo *classInfo) {
    LoadedClassEntry *entry;
    HASH_FIND_PTR(g_loadedClasses, &classInfo, entry);

    return entry ? entry->clazz : (JAVA_CLASS) JAVA_NULL;
}

JAVA_CLASS cl_bootstrap_find_class_by_info(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *classInfo) {
    monitor_enter(vmCurrentContext, &g_bootstrapClassLock);
    // First see if the class is already loaded
    JAVA_CLASS clazz = cl_bootstrap_get_loaded_class(classInfo);
    if (clazz != (JAVA_CLASS)JAVA_NULL) {
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
    // allocating the space for the class. The required size can store two structures: one for the
    // JAVA_CLASS corresponding to the classInfo, and an object instance of the java/lang/Class.
    size_t requiredSize = align_size_up(classInfo->classSize, SIZE_ALIGNMENT) +
                          align_size_up(g_classInfo_java_lang_Class->instanceSize, SIZE_ALIGNMENT);
    JAVA_CLASS thisClass = heap_alloc_loh(vmCurrentContext, requiredSize);
    if (!thisClass) {
        monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
        heap_free_uncollectable(vmCurrentContext, entry);
        fprintf(stderr, "Bootstrap Classloader: unable to alloc class %s\n", classInfo->thisClass);
        // TODO: throw OOM exception
        return (JAVA_CLASS) JAVA_NULL;
    }

    // We then register the class
    entry->key = classInfo;
    entry->clazz = thisClass;
    HASH_ADD_PTR(g_loadedClasses, key, entry);
    thisClass->state = CLASS_STATE_REGISTERED;

    JAVA_OBJECT classObject = ptr_inc(thisClass, align_size_up(classInfo->classSize, SIZE_ALIGNMENT));

    // this could be null when the java/lang/Class is not inited. Will fix after java/lang/Class is loaded.
    classObject->clazz = g_class_java_lang_Class;

    // Then we pre-init the class
    thisClass->info = classInfo;
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
    thisClass->classInstance = classObject;
    // Then resolve direct superinterfaces
    if (classInfo->interfaceCount > 0) {
        for (int i = 0; i < classInfo->interfaceCount; i++) {
            JAVA_CLASS it = cl_bootstrap_find_class_by_info(vmCurrentContext, classInfo->interfaces[i]);
            if (!it || it->state < CLASS_STATE_RESOLVED) {
                thisClass->state = CLASS_STATE_ERROR;
                monitor_exit(vmCurrentContext, &g_bootstrapClassLock);
                fprintf(stderr, "Bootstrap Classloader: unable to load superinterface %s\n", classInfo->interfaces[i]->thisClass);
                // TODO: throw exception
                return (JAVA_CLASS) JAVA_NULL;
            }
            thisClass->interfaces[i] = it;
        }
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
