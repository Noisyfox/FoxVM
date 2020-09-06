//
// Created by noisyfox on 2020/9/7.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_native.h"

JNIEXPORT jobject JNICALL Java_java_lang_Thread_currentThread(VM_PARAM_CURRENT_CONTEXT, jclass cls) {
    native_exit_jni(vmCurrentContext);

    jobject thrd = native_get_local_ref(vmCurrentContext, vmCurrentContext->currentThread);

    native_enter_jni(vmCurrentContext);

    return thrd;
}

JNIEXPORT void JNICALL Java_java_lang_Thread_setPriority0(VM_PARAM_CURRENT_CONTEXT, jobject jthread, jint prio) {
    // TODO
}
