//
// Created by noisyfox on 2018/9/23.
//

#include "vm_main.h"
#include "vm_memory.h"
#include "vm_classloader.h"
#include "vm_array.h"

int vm_main(int argc, char *argv[], JavaMethodRetVoid entrance) {
    // Init low level memory system first
    if (!mem_init()) {
        return -1;
    }

    thread_init();

    // Init base type systems
    array_init();

    // Create main thread
    VMThreadContext thread_main = {0};
    VM_PARAM_CURRENT_CONTEXT = &thread_main;

    // Init main thread
    stack_frame_make_root(&vmCurrentContext->frameRoot);
    thread_native_init(vmCurrentContext);
    thread_native_attach_main(vmCurrentContext);
    // register the main thread to global thread list
    thread_managed_add(vmCurrentContext);

    HeapConfig heapConfig = {
//            .maxSize=0,
//            .newRatio=2,
//            .survivorRatio=8
    };

    heap_init(vmCurrentContext, &heapConfig);
    // Init main tlab
    tlab_init(&vmCurrentContext->tlab);

    // Init classloader
    classloader_init(vmCurrentContext);

    // TODO: create java Thread Object for main thread

    // start GC thread
//    gc_thread_start(vmCurrentContext);

    stack_frame_start(0, 1, 0);

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
