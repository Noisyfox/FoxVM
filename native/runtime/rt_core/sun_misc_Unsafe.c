//
// Created by noisyfox on 2020/9/10.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_native.h"
#include "vm_class.h"
#include "vm_array.h"
#include "vm_exception.h"
#include <stdio.h>

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_arrayBaseOffset(VM_PARAM_CURRENT_CONTEXT, jobject unsafe, jclass cls) {
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, cls);
    JAVA_CLASS clazz = class_get_native_class(classObj);
    BasicType t = array_type_of(clazz->info->thisClass);

    native_enter_jni(vmCurrentContext);

    return array_header_size(t);
}

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_arrayIndexScale(VM_PARAM_CURRENT_CONTEXT, jobject unsafe, jclass cls) {
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, cls);
    JAVA_CLASS clazz = class_get_native_class(classObj);
    BasicType t = array_type_of(clazz->info->thisClass);

    native_enter_jni(vmCurrentContext);

    return type_size(t);
}

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_addressSize(VM_PARAM_CURRENT_CONTEXT, jobject unsafe) {
    return sizeof(uintptr_t);
}

JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_allocateMemory(VM_PARAM_CURRENT_CONTEXT, jobject unsafe, jlong bytes) {
    size_t sz = (size_t) bytes;
    if (sz != (JAVA_ULONG) bytes || bytes < 0) {
        native_exit_jni(vmCurrentContext);
        exception_set_IllegalArgumentException(vmCurrentContext, "bytes");
        native_enter_jni(vmCurrentContext);
        return 0;
    }

    if (sz == 0) {
        return 0;
    }
    sz = align_size_up(sz, SIZE_ALIGNMENT);
    void *x = malloc(sz);
    if (x == NULL) {
        // TODO: throw OOM
        fprintf(stderr, "Unsafe: unable to allocate memory");
        abort();
    }

    return (uintptr_t) x;
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_freeMemory(VM_PARAM_CURRENT_CONTEXT, jobject unsafe, jlong address) {
    free((void *) (uintptr_t) address);
}

JNIEXPORT jbyte JNICALL Java_sun_misc_Unsafe_getByte(VM_PARAM_CURRENT_CONTEXT, jobject unsafe, jlong address) {
    return *((jbyte*)(uintptr_t)address);
}

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putLong(VM_PARAM_CURRENT_CONTEXT, jobject unsafe, jlong address, jlong x) {
    *((jlong*)(uintptr_t)address) = x;
}
