//
// Created by noisyfox on 2020/8/27.
//

#include "vm_classloader.h"
#include "vm_boot_classloader.h"
#include "vm_thread.h"
#include <stdio.h>
#include "vm_array.h"

JAVA_BOOLEAN classloader_init(VM_PARAM_CURRENT_CONTEXT) {
    if (!cl_bootstrap_init(vmCurrentContext)) {
        return JAVA_FALSE;
    }

    return JAVA_TRUE;
}

#define class_init_completed(success) do{                                                   \
    ret = monitor_enter(vmCurrentContext, &clazz_slot);                                     \
    if (ret != thrd_success) {                                                              \
        fprintf(stderr, "Failed to enter monitor for class %s\n", clazz->info->thisClass);  \
        return JAVA_FALSE;                                                                  \
    }                                                                                       \
    clazz->state = success ? CLASS_STATE_INITIALIZED : CLASS_STATE_ERROR;                   \
    monitor_notify_all(vmCurrentContext, &clazz_slot);                                      \
    monitor_exit(vmCurrentContext, &clazz_slot);                                            \
} while(0)

static JAVA_BOOLEAN classloader_init_class(VM_PARAM_CURRENT_CONTEXT, JAVA_CLASS clazz) {
    if (clazz->state == CLASS_STATE_INITIALIZED) {
        return JAVA_TRUE;
    }

    // jvms8 §5.5 Initialization
    // For each class or interface C, there is a unique initialization lock LC.

    // The procedure for initializing C is then as follows:
    // 1. Synchronize on the initialization lock, LC, for C. This involves waiting until the
    //    current thread can acquire LC.

    // Class instances won't be moved by GC, so we don't need to worry about ref change after lock
    VMStackSlot clazz_slot;
    clazz_slot.type = VM_SLOT_OBJECT;
    clazz_slot.data.o = (JAVA_OBJECT) clazz;
    int ret = monitor_enter(vmCurrentContext, &clazz_slot);
    if (ret != thrd_success) {
        fprintf(stderr, "Failed to enter monitor for class %s\n", clazz->info->thisClass);
        return JAVA_FALSE;
    }

    if (clazz->state == CLASS_STATE_INITIALIZING) {
        if (clazz->initThread != vmCurrentContext->threadId) {
            // 2. If the Class object for C indicates that initialization is in progress for C by some
            //    other thread, then release LC and block the current thread until informed that the
            //    in-progress initialization has completed, at which time repeat this procedure.
            //
            //    Thread interrupt status is unaffected by execution of the initialization
            //    procedure.
            monitor_wait(vmCurrentContext, &clazz_slot, 0, 0);
        } else {
            // 3. If the Class object for C indicates that initialization is in progress for C by the
            //    current thread, then this must be a recursive request for initialization. Release
            //    LC and complete normally.
            monitor_exit(vmCurrentContext, &clazz_slot);
            return JAVA_TRUE;
        }
    }

    if (clazz->state == CLASS_STATE_INITIALIZED) {
        // 4. If the Class object for C indicates that C has already been initialized, then no
        //    further action is required. Release LC and complete normally.
        monitor_exit(vmCurrentContext, &clazz_slot);
        return JAVA_TRUE;
    }

    if (clazz->state == CLASS_STATE_ERROR) {
        // 5. If the Class object for C is in an erroneous state, then initialization is not
        //    possible. Release LC and throw a NoClassDefFoundError.
        monitor_exit(vmCurrentContext, &clazz_slot);
        fprintf(stderr, "Failed to init class %s\n", clazz->info->thisClass);
        // TODO: throw exception
        return JAVA_FALSE;
    }

    assert(clazz->state == CLASS_STATE_RESOLVED);
    assert(clazz->initThread == 0);

    // 6. Otherwise, record the fact that initialization of the Class object for C is in
    //    progress by the current thread, and release LC.
    clazz->state = CLASS_STATE_INITIALIZING;
    clazz->initThread = vmCurrentContext->threadId;
    monitor_exit(vmCurrentContext, &clazz_slot);

    printf("Initializing class %s\n", clazz->info->thisClass);

    // Then, initialize each final static field of C with the constant value in
    // its ConstantValue attribute (§4.7.2), in the order the fields appear in the
    // ClassFile structure.
    // The default value for numeric fields have already set during resolving process,
    // here we only deal with String fields.
    // TODO

    // 7. Init superclasses and superinterfaces
    if (clazz->superClass) {
        JAVA_BOOLEAN superInited = classloader_init_class(vmCurrentContext, clazz->superClass);
        if (superInited != JAVA_TRUE) {
            fprintf(stderr, "Failed to init superclass %s for class %s\n",
                    clazz->superClass->info->thisClass, clazz->info->thisClass);
            // If the initialization of S completes abruptly because of a thrown exception, then
            // acquire LC, label the Class object for C as erroneous, notify all waiting threads,
            // release LC, and complete abruptly, throwing the same exception that resulted
            // from initializing SC.
            class_init_completed(JAVA_FALSE);
            return JAVA_FALSE;
        }
    }
    for (int i = 0; i < clazz->interfaceCount; i++) {
        JAVA_CLASS it = clazz->interfaces[i];
        JAVA_BOOLEAN superInited = classloader_init_class(vmCurrentContext, it);
        if (superInited != JAVA_TRUE) {
            fprintf(stderr, "Failed to init superinterface %s for class %s\n",
                    it->info->thisClass, clazz->info->thisClass);
            // If the initialization of S completes abruptly because of a thrown exception, then
            // acquire LC, label the Class object for C as erroneous, notify all waiting threads,
            // release LC, and complete abruptly, throwing the same exception that resulted
            // from initializing SC.
            class_init_completed(JAVA_FALSE);
            return JAVA_FALSE;
        }
    }

    // If this class is array, then we also init the component type
    if (clazz->info->thisClass[0] == '[') {
        JavaArrayClass *arrayClass = (JavaArrayClass *) clazz;
        JAVA_BOOLEAN componentInited = classloader_init_class(vmCurrentContext, arrayClass->componentType);
        if (componentInited != JAVA_TRUE) {
            fprintf(stderr, "Failed to init component class %s for array %s\n",
                    arrayClass->componentType->info->thisClass, clazz->info->thisClass);
            // If the initialization of S completes abruptly because of a thrown exception, then
            // acquire LC, label the Class object for C as erroneous, notify all waiting threads,
            // release LC, and complete abruptly, throwing the same exception that resulted
            // from initializing SC.
            class_init_completed(JAVA_FALSE);
            return JAVA_FALSE;
        }
    }

    // 9. Next, execute the class or interface initialization method of C.
    // Find the <clinit>
    JavaMethodRetVoid clinit = clazz->info->clinit;
    if (!clinit) {
        // No clinit, success
        class_init_completed(JAVA_TRUE);
        return JAVA_TRUE;
    }
    // call clinit
    // Create a fake stack
    stack_frame_start(-1, 0, 0);
    // Setup the invoke context
    vmCurrentContext->callingClass = clazz;
    // Call the function
    clinit(vmCurrentContext);
    // TODO: handle error
    JAVA_BOOLEAN clinit_success = JAVA_TRUE;
    // Pop the stack
    stack_frame_end();

    if (clinit_success) {
        // 10. If the execution of the class or interface initialization method completes
        //     normally, then acquire LC, label the Class object for C as fully initialized, notify
        //     all waiting threads, release LC, and complete this procedure normally.
        class_init_completed(JAVA_TRUE);
        return JAVA_TRUE;
    } else {
        // TODO: handle error
        class_init_completed(JAVA_FALSE);
        return JAVA_FALSE;
    }
}

#undef class_init_completed

JAVA_CLASS classloader_get_class(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classloader, JavaClassInfo *classInfo) {
    JAVA_CLASS c = NULL;
    if (classloader == NULL) {
        // Use bootstrap cl
        c = cl_bootstrap_find_class_by_info(vmCurrentContext, classInfo);
    }
    if(c) {
        if (!classloader_init_class(vmCurrentContext, c)) {
            return (JAVA_CLASS) JAVA_NULL;
        }
    }

    return c;
}

JAVA_CLASS classloader_get_class_by_name(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classloader, C_CSTR className) {
    JAVA_CLASS c = NULL;
    if (classloader == NULL) {
        // Use bootstrap cl
        c = cl_bootstrap_find_class(vmCurrentContext, className);
    }
    if(c) {
        if (!classloader_init_class(vmCurrentContext, c)) {
            return (JAVA_CLASS) JAVA_NULL;
        }
    }

    return c;
}
