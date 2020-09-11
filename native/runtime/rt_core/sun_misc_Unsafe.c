//
// Created by noisyfox on 2020/9/10.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_native.h"
#include "vm_class.h"
#include "vm_array.h"

static inline BasicType get_element_basic_type(JAVA_CLASS clazz) {
    if (clazz == g_class_array_Z) {
        return VM_TYPE_BOOLEAN;
    } else if (clazz == g_class_array_B) {
        return VM_TYPE_BYTE;
    } else if (clazz == g_class_array_C) {
        return VM_TYPE_CHAR;
    } else if (clazz == g_class_array_S) {
        return VM_TYPE_SHORT;
    } else if (clazz == g_class_array_I) {
        return VM_TYPE_INT;
    } else if (clazz == g_class_array_J) {
        return VM_TYPE_LONG;
    } else if (clazz == g_class_array_F) {
        return VM_TYPE_FLOAT;
    } else if (clazz == g_class_array_D) {
        return VM_TYPE_DOUBLE;
    }

    return VM_TYPE_OBJECT;
}

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_arrayBaseOffset(VM_PARAM_CURRENT_CONTEXT, jobject unsafe, jclass cls) {
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, cls);
    JAVA_CLASS clazz = class_get_native_class(classObj);
    BasicType t = get_element_basic_type(clazz);

    native_enter_jni(vmCurrentContext);

    return array_header_size(t);
}

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_arrayIndexScale(VM_PARAM_CURRENT_CONTEXT, jobject unsafe, jclass cls) {
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, cls);
    JAVA_CLASS clazz = class_get_native_class(classObj);
    BasicType t = get_element_basic_type(clazz);

    native_enter_jni(vmCurrentContext);

    return type_size(t);
}

JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_addressSize(VM_PARAM_CURRENT_CONTEXT, jobject unsafe) {
    return sizeof(uintptr_t);
}
