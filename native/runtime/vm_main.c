//
// Created by noisyfox on 2018/9/23.
//

#include "vm_main.h"

int vm_main(int argc, char *argv[], VMMainEntrance entrance) {
    thread_init();

    VMThreadContext thread_main = {0};
    VM_PARAM_CURRENT_CONTEXT = &thread_main;

    thread_native_init(vmCurrentContext);
    thread_native_attach_main(vmCurrentContext);
    // register the main thread to global thread list
    thread_managed_add(vmCurrentContext);

    HeapConfig heapConfig = {
            .maxSize=0,
            .newRatio=2,
            .survivorRatio=8
    };

    heap_init(vmCurrentContext, &heapConfig);

    tlab_init(vmCurrentContext, &vmCurrentContext->tlab);
    // TODO: create java Thread Object for main thread

    // start GC thread
    gc_thread_start(vmCurrentContext);

    entrance(vmCurrentContext, (JAVA_CLASS) JAVA_NULL, NULL);

    // Shutdown VM
    gc_thread_shutdown(vmCurrentContext);
//    thread_sleep(vmCurrentContext, 5000, 0);
    thread_managed_remove(vmCurrentContext);
    thread_native_free(vmCurrentContext);
    vmCurrentContext->threadId = 0;
    // TODO: free up heap.

    return 0;
}
