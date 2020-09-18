//
// Created by noisyfox on 2020/9/18.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_bytecode.h"
#include "vm_reflection.h"
#include "vm_array.h"
#include "vm_class.h"
#include "vm_string.h"
#include "vm_method.h"
#include "vm_classloader.h"
#include "vm_primitive.h"
#include <stdio.h>

static void invoke_constructor(VM_PARAM_CURRENT_CONTEXT, MethodInfo* method, JAVA_OBJECT obj, JAVA_ARRAY args) {
    JAVA_INT argLength = args->length + 1;

    // Creating a stack frame, C99 has dynamic size array! Yeah!
    stack_frame_start(NULL, argLength, 0);
    exception_frame_start();
        exception_suppressedv();
    exception_block_end();

    // Push $this
    stack_push_object(obj);

    // Push arguments
    C_CSTR desc = string_get_constant_utf8(method->descriptor);
    C_CSTR currentParam = NULL;
    int i = 0;
    while ((currentParam = method_get_next_parameter_type(&desc)) != NULL) {
        JAVA_OBJECT paramObj = *((JAVA_OBJECT *) array_element_at(args, VM_TYPE_OBJECT, i));

        switch (currentParam[0]) {
            case TYPE_DESC_BYTE: {
                JAVA_BYTE v = primitive_unbox_b(vmCurrentContext, paramObj);
                exception_raise_if_occurred(vmCurrentContext);
                stack_push_int(v);
                break;
            }
            case TYPE_DESC_CHAR: {
                JAVA_CHAR v = primitive_unbox_c(vmCurrentContext, paramObj);
                exception_raise_if_occurred(vmCurrentContext);
                stack_push_int((JAVA_UCHAR)v);
                break;
            }
            case TYPE_DESC_DOUBLE: {
                JAVA_DOUBLE v = primitive_unbox_d(vmCurrentContext, paramObj);
                exception_raise_if_occurred(vmCurrentContext);
                stack_push_double(v);
                break;
            }
            case TYPE_DESC_FLOAT: {
                JAVA_FLOAT v = primitive_unbox_f(vmCurrentContext, paramObj);
                exception_raise_if_occurred(vmCurrentContext);
                stack_push_float(v);
                break;
            }
            case TYPE_DESC_INT: {
                JAVA_INT v = primitive_unbox_i(vmCurrentContext, paramObj);
                exception_raise_if_occurred(vmCurrentContext);
                stack_push_int(v);
                break;
            }
            case TYPE_DESC_LONG: {
                JAVA_LONG v = primitive_unbox_j(vmCurrentContext, paramObj);
                exception_raise_if_occurred(vmCurrentContext);
                stack_push_long(v);
                break;
            }
            case TYPE_DESC_SHORT: {
                JAVA_SHORT v = primitive_unbox_s(vmCurrentContext, paramObj);
                exception_raise_if_occurred(vmCurrentContext);
                stack_push_int(v);
                break;
            }
            case TYPE_DESC_BOOLEAN: {
                JAVA_BOOLEAN v = primitive_unbox_z(vmCurrentContext, paramObj);
                exception_raise_if_occurred(vmCurrentContext);
                stack_push_int(v);
                break;
            }

            case TYPE_DESC_ARRAY:
            case TYPE_DESC_REFERENCE:
                // No unboxing required
                stack_push_object(paramObj);
                break;
        }
        i++;
    }

    // Invoke the constructor
    bc_invoke_special(method->code);

    stack_frame_end();
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Constructor_newInstance0(VM_PARAM_CURRENT_CONTEXT, jobject cst, jobjectArray args) {
    native_exit_jni(vmCurrentContext);
    jobject result = NULL;

    JAVA_OBJECT constructorObj = native_dereference(vmCurrentContext, cst);
    native_check_exception();

    JAVA_CLASS declaringClass = reflection_constructor_get_declaring_class(constructorObj);
    JAVA_INT slot = reflection_constructor_get_slot(constructorObj);

    MethodInfo *constructorInfo = declaringClass->info->methods[slot];

    // Create the instance
    if (!classloader_init_class(vmCurrentContext, declaringClass)) {
        native_check_exception();
        // TODO: throw ExceptionInInitializerError instead.
        fprintf(stderr, "Unable to init the class %s\n", declaringClass->info->thisClass);
        abort();
    }
    native_handler_new(h_obj, class_alloc_instance(vmCurrentContext, declaringClass));

    // Invoke the constructor
    invoke_constructor(vmCurrentContext, constructorInfo,
                       native_dereference(vmCurrentContext, h_obj),
                       (JAVA_ARRAY) native_dereference(vmCurrentContext, args));
    native_check_exception();

    result = h_obj;
native_end:
    native_enter_jni(vmCurrentContext);
    return result;
}
