//
// Created by noisyfox on 2018/9/23.
//

#include "vm_main.h"
#include "vm_memory.h"
#include "vm_native.h"
#include "vm_classloader.h"
#include "vm_method.h"

static void main_call_initializeSystemClass(VM_PARAM_CURRENT_CONTEXT) {
    JAVA_CLASS java_lang_System = classloader_get_class_by_name_init(vmCurrentContext, JAVA_NULL, "java/lang/System");

    JavaClassInfo *info = java_lang_System->info;
    MethodInfo *method_initializeSystemClass = method_find(info, "initializeSystemClass", "()V");
    assert(method_initializeSystemClass);

    ((JavaMethodRetVoid) method_initializeSystemClass->code)(vmCurrentContext);
}

int vm_main(int argc, char *argv[], JavaMethodRetVoid entrance) {
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

    // Since we are calling into java function, we need a java stack frame
    stack_frame_start(NULL, 1, 0);

    // Call `java.lang.System#initializeSystemClass`
    main_call_initializeSystemClass(vmCurrentContext);

    bc_aconst_null();
    entrance(vmCurrentContext);

    stack_frame_end();

    // Shutdown VM
//    gc_thread_shutdown(vmCurrentContext);
//    thread_sleep(vmCurrentContext, 5000, 0);
    thread_managed_remove(vmCurrentContext);
    thread_native_free(vmCurrentContext);
    vmCurrentContext->threadId = 0;
    // TODO: free up heap.

    return 0;
}
