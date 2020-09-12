//
// Created by noisyfox on 2020/9/2.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_bytecode.h"
#include "vm_class.h"
#include "vm_array.h"

JNIEXPORT jclass JNICALL Java_java_lang_Class_getPrimitiveClass(VM_PARAM_CURRENT_CONTEXT, jclass cls, jstring name) {
    return NULL;
}

JAVA_OBJECT method_fastNative_9Pjava_lang5CClass_15MgetClassLoader0_R9Pjava_lang11CClassLoader(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative* methodInfo) {
    stack_frame_start(&methodInfo->method, 0, 1);
    bc_prepare_arguments(1);
    bc_check_objectref();

    JAVA_OBJECT classObj = local_of(0).data.o;
    JAVA_CLASS clazz = class_get_native_class(classObj);

    stack_frame_end();

    return clazz->classLoader;
}

JNIEXPORT jboolean JNICALL Java_java_lang_Class_isArray(VM_PARAM_CURRENT_CONTEXT, jobject thiz) {
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, thiz);
    JAVA_CLASS clazz = class_get_native_class(classObj);
    JAVA_BOOLEAN isArray = !clazz->isPrimitive && clazz->info->thisClass[0] == TYPE_DESC_ARRAY;

    native_enter_jni(vmCurrentContext);

    return isArray;
}

JNIEXPORT jclass JNICALL Java_java_lang_Class_getComponentType(VM_PARAM_CURRENT_CONTEXT, jobject thiz) {
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, thiz);
    JAVA_CLASS clazz = class_get_native_class(classObj);
    JAVA_BOOLEAN isArray = !clazz->isPrimitive && clazz->info->thisClass[0] == TYPE_DESC_ARRAY;

    jclass result = NULL;
    if (isArray) {
        JAVA_OBJECT componentClassObj = ((JavaArrayClass*)clazz)->componentType->classInstance;
        result = native_get_local_ref(vmCurrentContext, componentClassObj);
    }

    native_enter_jni(vmCurrentContext);

    return result;
}

JNIEXPORT jboolean JNICALL Java_java_lang_Class_isPrimitive(VM_PARAM_CURRENT_CONTEXT, jobject thiz) {
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, thiz);
    JAVA_CLASS clazz = class_get_native_class(classObj);
    JAVA_BOOLEAN isPrimitive = clazz->isPrimitive;

    native_enter_jni(vmCurrentContext);

    return isPrimitive;
}
