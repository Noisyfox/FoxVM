//
// Created by noisyfox on 2020/9/11.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_bytecode.h"

JAVA_OBJECT method_fastNative_9Pjava_lang6CObject_8MgetClass_R9Pjava_lang5CClass(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative* methodInfo) {
    stack_frame_start(&methodInfo->method, 0, 1);
    bc_prepare_arguments(1);
    bc_check_objectref();

    JAVA_OBJECT obj = local_of(0).data.o;
    JAVA_CLASS clazz = obj_get_class(obj);

    stack_frame_end();

    return clazz->classInstance;
}

JAVA_VOID method_fastNative_9Pjava_lang6CObject_9MnotifyAll(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *methodInfo) {
    stack_frame_start(&methodInfo->method, 0, 1);
    bc_prepare_arguments(1);
    bc_check_objectref();

    monitor_notify_all(vmCurrentContext, &local_of(0));

    stack_frame_end();
}
