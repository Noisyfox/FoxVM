//
// Created by noisyfox on 2018/9/23.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_exception.h"
#include "vm_class.h"
#include "vm_bytecode.h"
#include "classloader/vm_boot_classloader.h"
#include "vm_method.h"
#include "vm_string.h"
#include "vm_gc.h"
#include <setjmp.h>
#include <stdio.h>

#define MAX_FORMAT_MESSAGE_SIZE 512

JAVA_BOOLEAN exception_matches(JAVA_OBJECT ex, int32_t current_sp, int32_t start, int32_t end, JavaClassInfo *type) {
    if (current_sp < start || current_sp >= end) {
        return JAVA_FALSE;
    }

    if (type == NULL) {
        return JAVA_TRUE;
    }

    return class_assignable(obj_get_class(ex)->info, type);
}

JAVA_VOID exception_raise(VM_PARAM_CURRENT_CONTEXT) {
    if (vmCurrentContext->exception == JAVA_NULL) {
        fprintf(stderr, "exception_raise() called with vmCurrentContext->exception not set");
        abort();
    }

    VMStackFrame *frame = stack_frame_top(vmCurrentContext);
    ExceptionFrame *exFrame = frame->exceptionHandler;
    if (exFrame == NULL) {
        fprintf(stderr, "exception_raise() called with no available exceptionHandler");
        abort();
    }

    longjmp(exFrame->jmpTarget, EX_VAL);
}

JAVA_VOID exception_set(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT ex) {
    assert(ex);

    if (vmCurrentContext->exception) {
        fprintf(stderr, "exception_set() called with vmCurrentContext->exception already set");
        abort();
    }

    vmCurrentContext->exception = ex;
}

JAVA_VOID exception_set_new(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *exClass, C_CSTR message) {
    JAVA_OBJECT ex = exception_new(vmCurrentContext, exClass, message);
    if (ex) {
        exception_set(vmCurrentContext, ex);
    }
}

JAVA_VOID exception_set_newf(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *exClass, C_CSTR format, ...) {
    va_list ap;
    char message[MAX_FORMAT_MESSAGE_SIZE];
    va_start(ap, format);
    vsnprintf(message, MAX_FORMAT_MESSAGE_SIZE, format, ap);
    va_end(ap);

    return exception_set_new(vmCurrentContext, exClass, message);
}

JAVA_OBJECT exception_new(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *exClass, C_CSTR message) {
    // TODO: check that exClass is a subtype of Throwable

    MethodInfo *constructor = method_find(exClass, "<init>", "(Ljava/lang/String;)V");
    if (constructor == NULL) {
        exception_set_NoSuchFieldError(vmCurrentContext, "Unable to find method <init>(Ljava/lang/String;)V");
        return JAVA_NULL;
    }

    if (method_is_static(constructor)) {
        exception_set_IncompatibleClassChangeError(vmCurrentContext, NULL);
        return JAVA_NULL;
    }

    JAVA_OBJECT messageStr = JAVA_NULL;
    if (message) {
        messageStr = string_create_utf8(vmCurrentContext, message);
        if (messageStr == JAVA_NULL) {
            return JAVA_NULL;
        }
    }

    stack_frame_start(NULL, 3, 1);
    exception_frame_start();
        exception_suppressed(JAVA_NULL);
    exception_block_end();

    local_of(0).type = VM_SLOT_OBJECT;
    local_of(0).data.o = messageStr;

    // Create the exception instance
    bc_new(exClass);
    bc_dup();
    // Push the message string to stack
    bc_aload(0);
    // Call the init method
    bc_invoke_special(constructor->code);
    // Put the exception instance to local 0
    bc_astore(0);

    JAVA_OBJECT ex = local_of(0).data.o;

    stack_frame_end();

    return ex;
}

JAVA_OBJECT exception_newf(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *exClass, C_CSTR format, ...) {
    va_list ap;
    char message[MAX_FORMAT_MESSAGE_SIZE];
    va_start(ap, format);
    vsnprintf(message, MAX_FORMAT_MESSAGE_SIZE, format, ap);
    va_end(ap);

    return exception_new(vmCurrentContext, exClass, message);
}

JAVA_VOID exception_set_NoSuchFieldError(VM_PARAM_CURRENT_CONTEXT, C_CSTR message) {
    exception_set_new(vmCurrentContext, g_classInfo_java_lang_NoSuchFieldError, message);
}

JAVA_VOID exception_set_IncompatibleClassChangeError(VM_PARAM_CURRENT_CONTEXT, C_CSTR message) {
    exception_set_new(vmCurrentContext, g_classInfo_java_lang_IncompatibleClassChangeError, message);
}

JAVA_VOID exception_set_UnsatisfiedLinkError(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method, C_CSTR message) {
    exception_set_newf(vmCurrentContext, g_classInfo_java_lang_UnsatisfiedLinkError,
                       "Unable to find native method %s%s: %s", method->method.name, method->method.descriptor, message);
}

JAVA_VOID exception_set_ArrayStoreException(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *arrayType, JavaClassInfo *elementType) {
    C_CSTR arrayTypeName = class_pretty_descriptor(arrayType->thisClass);
    C_CSTR elementTypeName = class_pretty_descriptor(elementType->thisClass);
    if (!arrayTypeName || !elementTypeName) {
        heap_free_uncollectable((void *) arrayTypeName);
        heap_free_uncollectable((void *) elementTypeName);
        // TODO: throw OOM?
        return;
    }

    exception_set_newf(vmCurrentContext, g_classInfo_java_lang_ArrayStoreException,
                       "%s cannot be stored in array %s", elementTypeName, arrayTypeName);

    heap_free_uncollectable((void *) arrayTypeName);
    heap_free_uncollectable((void *) elementTypeName);
}
