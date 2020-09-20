//
// Created by noisyfox on 2020/9/12.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_class.h"
#include "vm_exception.h"

JAVA_OBJECT method_fastNative_11Psun_reflect10CReflection_14MgetCallerClass_R9Pjava_lang5CClass(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative* methodInfo) {
    VMStackFrame *frame = stack_frame_top(vmCurrentContext); // This is the frame of the direct caller
    VMStackFrame *frame2 = frame->prev;
    if (frame2->type != VM_STACK_FRAME_JAVA) {
        return JAVA_NULL;
    }

    return frame2->thisClass->classInstance;
}

JNIEXPORT jint JNICALL Java_sun_reflect_Reflection_getClassAccessFlags(VM_PARAM_CURRENT_CONTEXT, jclass unused, jclass cls) {
    native_exit_jni(vmCurrentContext);

    jint result = 0;
    JAVA_OBJECT classObj = native_dereference(vmCurrentContext, cls);
    if (!classObj) {
        exception_set_NullPointerException_arg(vmCurrentContext, "cls");
        native_check_exception();
    }
    JAVA_CLASS clazz = class_get_native_class(classObj);
    result = clazz->info->accessFlags;

native_end:
    native_enter_jni(vmCurrentContext);
    return result;
}
