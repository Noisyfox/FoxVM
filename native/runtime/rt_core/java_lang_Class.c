//
// Created by noisyfox on 2020/9/2.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_bytecode.h"
#include "vm_class.h"
#include "vm_array.h"
#include "vm_primitive.h"
#include "vm_classloader.h"
#include <string.h>
#include <stdio.h>

static C_STR java_lang_Class_getClassInternalName(VM_PARAM_CURRENT_CONTEXT, jstring name) {
    C_CSTR utfName = vmCurrentContext->jni->GetStringUTFChars(&vmCurrentContext->jni, name, NULL);
    if (exception_occurred(vmCurrentContext) || utfName == NULL) {
        return NULL;
    }
    C_STR dupName = strdup(utfName);
    vmCurrentContext->jni->ReleaseStringUTFChars(&vmCurrentContext->jni, name, utfName);
    if (exception_occurred(vmCurrentContext)) {
        free(dupName);
        return NULL;
    }

    if (!dupName) {
        // TODO: throw OOM
        fprintf(stderr, "Unable to dup the class name\n");
        abort();
    }
    // Replace the dot in the class name to slash
    C_STR p = dupName;
    while (*p != '\0') {
        if (*p == '.') {
            *p = '/';
        }
        p++;
    }

    return dupName;
}

JNIEXPORT jclass JNICALL Java_java_lang_Class_forName0(VM_PARAM_CURRENT_CONTEXT, jclass cls, jstring name, jboolean initialize, jobject loader, jclass caller) {
    C_STR internalName = java_lang_Class_getClassInternalName(vmCurrentContext, name);
    if (exception_occurred(vmCurrentContext)) {
        return NULL;
    }

    native_exit_jni(vmCurrentContext);
    jclass result = NULL;
    do {
        JAVA_OBJECT classloader = native_dereference(vmCurrentContext, loader);
        if (exception_occurred(vmCurrentContext)) {
            break;
        }

        JAVA_CLASS c = NULL;
        if (initialize) {
            c = classloader_get_class_by_name_init(vmCurrentContext, classloader, internalName);
        } else {
            c = classloader_get_class_by_name(vmCurrentContext, classloader, internalName);
        }
        if (exception_occurred(vmCurrentContext)) {
            break;
        }
        if (c) {
            result = native_get_local_ref(vmCurrentContext, c->classInstance);
        }
    } while (0);

    free(internalName);
    native_enter_jni(vmCurrentContext);
    return result;
}

JNIEXPORT jclass JNICALL Java_java_lang_Class_getPrimitiveClass(VM_PARAM_CURRENT_CONTEXT, jclass cls, jstring name) {
    C_CSTR utfName = vmCurrentContext->jni->GetStringUTFChars(&vmCurrentContext->jni, name, NULL);
    if (exception_occurred(vmCurrentContext) || utfName == NULL) {
        return NULL;
    }

    native_exit_jni(vmCurrentContext);

    jclass result = NULL;
    JAVA_CLASS clazz = primitive_of_name(utfName);
    if (clazz != NULL) {
        result = native_get_local_ref(vmCurrentContext, clazz->classInstance);
    }

    native_enter_jni(vmCurrentContext);

    vmCurrentContext->jni->ReleaseStringUTFChars(&vmCurrentContext->jni, name, utfName);

    return result;
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

JNIEXPORT jboolean JNICALL Java_java_lang_Class_isInterface(VM_PARAM_CURRENT_CONTEXT, jobject thiz) {
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, thiz);
    JAVA_CLASS clazz = class_get_native_class(classObj);
    JAVA_BOOLEAN isInterface = !clazz->isPrimitive && class_is_interface(clazz->info);

    native_enter_jni(vmCurrentContext);

    return isInterface;
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
