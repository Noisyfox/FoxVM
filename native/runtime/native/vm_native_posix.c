//
// Created by noisyfox on 2020/9/2.
//

#include "vm_native.h"
#include <dlfcn.h>
#include <stdio.h>

void *native_load_library(C_CSTR name) {
    void *handle = dlopen(name, RTLD_LOCAL | RTLD_LAZY);
    if (!handle) {
        C_CSTR errorMsg = dlerror();
        fprintf(stderr, "Unable to load library '%s': %s", name ? name : "NULL", errorMsg ? errorMsg : "Unknown error");
    }

    return handle;
}

void *native_find_symbol(void *handle, C_CSTR symbol) {
    return dlsym(handle, symbol);
}
