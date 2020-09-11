//
// Created by noisyfox on 2020/9/2.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_bytecode.h"
#include "vm_class.h"

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
