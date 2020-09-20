//
// Created by noisyfox on 2018/9/23.
//

#include "vm_main.h"
#include "vm_memory.h"
#include "vm_native.h"
#include "vm_classloader.h"
#include "vm_method.h"
#include "vm_reflection.h"
#include "vm_primitive.h"
#include <stdio.h>
#include <string.h>

static void main_call_initializeSystemClass(VM_PARAM_CURRENT_CONTEXT) {
    JAVA_CLASS java_lang_System = classloader_get_class_by_name_init(vmCurrentContext, JAVA_NULL, "java/lang/System");

    JavaClassInfo *info = java_lang_System->info;
    MethodInfo *method_initializeSystemClass = method_find(info, "initializeSystemClass", "()V");
    assert(method_initializeSystemClass);

    ((JavaMethodRetVoid) method_initializeSystemClass->code)(vmCurrentContext);
}

static void start_main(VM_PARAM_CURRENT_CONTEXT, int argc, char *argv[], C_STR mainClass) {
    // Since we are calling into java function, we need a java stack frame
    stack_frame_start(NULL, 1, 0);
    exception_frame_start();
        exception_suppressedv();
    exception_block_end();

    // Call `java.lang.System#initializeSystemClass`
    main_call_initializeSystemClass(vmCurrentContext);

    JAVA_CLASS c;
    {
        size_t len = strlen(mainClass);
        char mainClass2[len + 1];
        memcpy(mainClass2, mainClass, (len + 1) * sizeof(char));

        C_STR p = mainClass;
        while ((*p) != '\0') {
            if ((*p) == '.') {
                (*p) = '/';
            }
            p++;
        }

        c = classloader_get_class_by_name_init(vmCurrentContext, JAVA_NULL, mainClass2);
    }
    exception_raise_if_occurred(vmCurrentContext);
    if (!c) {
        fprintf(stderr,
                "Error: Could not find or load main class %s\n", mainClass);
        abort();
    }

    MethodInfo *mainFunc = method_find(c->info, "main", "([Ljava/lang/String;)V");
    if(!mainFunc) {
        fprintf(stderr,
                "Error: Main method not found in class %s, please define the main method as:\n   public static void main(String[] args)\n", mainClass);
        abort();
    }
    // TODO: prepare arguments
    bc_aconst_null();
    bc_invoke_static(c->info, mainFunc->code);

    stack_frame_end();
}

static void try_print_exception(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT ex) {
    stack_frame_start(NULL, 1, 0);

    stack_push_object(ex);

    JavaClassInfo *clazz = obj_get_class(ex)->info;
    JAVA_INT i = method_vtable_find(clazz, "printStackTrace", "()V");
    assert(i >= 0);

    bc_invoke_virtual(1, clazz, i);

    stack_frame_end();
}

int vm_main(int argc, char *argv[], C_STR mainClass) {
    // Init low level memory system first
    if (!mem_init()) {
        return -1;
    }

    // Init heap first because we need it for allocating thread context
    HeapConfig heapConfig = {
//            .maxSize=0,
//            .newRatio=2,
//            .survivorRatio=8
    };
    heap_init(&heapConfig);

    // Then we init native thread system
    thread_init();

    // Create main thread
    VMThreadContext* thread_main = heap_alloc_uncollectable(sizeof(VMThreadContext));
    assert(thread_main);
    VM_PARAM_CURRENT_CONTEXT = thread_main;

    // Init main tlab
    tlab_init(&vmCurrentContext->tlab);

    // Init main thread
    native_make_root_stack_frame(&vmCurrentContext->frameRoot);
    thread_native_init(vmCurrentContext);
    // register the main thread to global thread list
    thread_managed_add(vmCurrentContext);
    thread_native_attach(vmCurrentContext);

    // Init jni
    native_init();
    native_thread_attach_jni(vmCurrentContext);

    // Init classloader
    if (!classloader_init(vmCurrentContext)) {
        return -1;
    }

    // Create java Thread Object for main thread
    thread_init_main(vmCurrentContext);

    // start GC thread
//    gc_thread_start(vmCurrentContext);

    // Init reflection after the type system is initialized
    reflection_init(vmCurrentContext);
    primitive_init(vmCurrentContext);

    start_main(vmCurrentContext, argc, argv, mainClass);
    if (exception_occurred(vmCurrentContext)) {
        fprintf(stderr, "Unhandled exception");
        try_print_exception(vmCurrentContext, exception_clear(vmCurrentContext));
        abort();
    }

    // Shutdown VM
//    gc_thread_shutdown(vmCurrentContext);
//    thread_sleep(vmCurrentContext, 5000, 0);
    thread_managed_remove(vmCurrentContext);
    thread_native_free(vmCurrentContext);
    vmCurrentContext->threadId = 0;
    // TODO: free up heap.

    return 0;
}
