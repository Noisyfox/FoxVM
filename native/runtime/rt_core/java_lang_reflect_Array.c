//
// Created by noisyfox on 2020/9/12.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_bytecode.h"
#include "vm_array.h"

JNIEXPORT jint JNICALL Java_java_lang_reflect_Array_getLength0(VM_PARAM_CURRENT_CONTEXT, jclass cls, jobject array) {
    native_exit_jni(vmCurrentContext);

    JAVA_ARRAY arrayObj = (JAVA_ARRAY) native_dereference(vmCurrentContext, array);
    jint length = arrayObj->length;

    native_enter_jni(vmCurrentContext);

    return length;
}
