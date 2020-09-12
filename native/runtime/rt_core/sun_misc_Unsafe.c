//
// Created by noisyfox on 2020/9/10.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_native.h"
#include "vm_class.h"
#include "vm_array.h"

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
