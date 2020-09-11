//
// Created by noisyfox on 2020/9/12.
//

#include "vm_base.h"
#include "vm_thread.h"

JAVA_OBJECT method_fastNative_11Psun_reflect10CReflection_14MgetCallerClass_R9Pjava_lang5CClass(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative* methodInfo) {
    VMStackFrame *frame = stack_frame_top(vmCurrentContext); // This is the frame of the direct caller
    VMStackFrame *frame2 = frame->prev;
    if (frame2->type != VM_STACK_FRAME_JAVA) {
        return JAVA_NULL;
    }

    return frame2->thisClass->classInstance;
}
