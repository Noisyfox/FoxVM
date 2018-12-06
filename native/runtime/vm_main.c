//
// Created by noisyfox on 2018/9/23.
//

#include "vm_main.h"

int vm_main(int argc, char *argv[], VMMainEntrance entrance) {
    thread_init();

    VMThreadContext thread_main = {0};
    thread_native_init(&thread_main);
    thread_native_attach_main(&thread_main);

    // TODO: register the main thread to global thread list

    HeapConfig heapConfig = {
            .maxSize=0,
            .newRatio=2,
            .survivorRatio=8
    };

    heap_init(&heapConfig);

    tlab_init(&thread_main, &thread_main.tlab);
    // TODO: create java Thread Object for main thread

    entrance(&thread_main, (JAVA_CLASS) JAVA_NULL, NULL);

    return 0;
}
