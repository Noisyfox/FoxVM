//
// Created by noisyfox on 2018/10/20.
//

#include "vm_thread.h"
#include "vm_array.h"
#include "vm_memory.h"
#include "vm_bytecode.h"
#include "vm_classloader.h"
#include "classloader/vm_boot_classloader.h"
#include "vm_gc.h"
#include "vm_class.h"
#include <stdio.h>
#include <string.h>

JavaClass* g_class_array_Z = NULL;
JavaClass* g_class_array_B = NULL;
JavaClass* g_class_array_C = NULL;
JavaClass* g_class_array_S = NULL;
JavaClass* g_class_array_I = NULL;
JavaClass* g_class_array_J = NULL;
JavaClass* g_class_array_F = NULL;
JavaClass* g_class_array_D = NULL;

// Check whether an element of am array with the given type must be
// aligned 0 mod 8.
static inline JAVA_BOOLEAN array_element_alignment(BasicType t) {
    return (t == VM_TYPE_DOUBLE || t == VM_TYPE_LONG) ? 8 : ANY_ALIGNMENT;
}

size_t array_header_size(BasicType t) {
    return align_size_up(sizeof(JavaArrayBase), array_element_alignment(t));
}

void *array_base(JAVA_ARRAY a, BasicType t) {
    return ptr_inc(a, array_header_size(t));
}

void *array_element_at(JAVA_ARRAY a, BasicType t, size_t index) {
    return ptr_inc(array_base(a, t), type_size(t) * index);
}

size_t array_max_length(BasicType t) {
    size_t header_size = array_header_size(t);
    // Available size of element space in bytes
    size_t remaining_elements_size = align_size_down(SIZE_MAX, SIZE_ALIGNMENT) - header_size;

    // Element count
    size_t elements_count = remaining_elements_size / type_size(t);

    // Make sure the count is less than the max Java int, otherwise it's
    // impossible to use by Java code.
    return size_min(JAVA_INT_MAX, elements_count);
}

size_t array_size_of_type(BasicType t, size_t length) {
    return align_size_up(array_header_size(t) + type_size(t) * length, SIZE_ALIGNMENT);
}

static JAVA_ARRAY array_clone(VM_PARAM_CURRENT_CONTEXT, JAVA_ARRAY array) {
    JAVA_CLASS c = obj_get_class((JAVA_OBJECT) array);
    JAVA_INT length = array->length;
    C_CSTR desc = c->info->thisClass;
    BasicType elementType = array_type_of(desc);

    native_scoped {
        // Store the original
        native_handler_new(h_orig, array);
        // Create new one
        native_handler_new(h_cloned, array_new(vmCurrentContext, desc, length));
        // Copy the array
        JAVA_ARRAY orig = (JAVA_ARRAY) native_dereference(vmCurrentContext, h_orig);
        JAVA_ARRAY cloned = (JAVA_ARRAY) native_dereference(vmCurrentContext, h_cloned);
        size_t size = type_size(elementType) * length;
        memcpy(array_base(cloned, elementType), array_base(orig, elementType), size);

        result = h_cloned;
    } native_scope_end();

    return (JAVA_ARRAY) javaResult;
}

/**
 * Impl of array.clone() method
 */
JAVA_OBJECT g_array_5Mclone_R9Pjava_lang6CObject(VM_PARAM_CURRENT_CONTEXT) {
    stack_frame_start(&g_array_methodInfo_5Mclone_R9Pjava_lang6CObject, 0, 1);
    bc_prepare_arguments(1);
    bc_check_objectref();

    JAVA_ARRAY cloned = array_clone(vmCurrentContext, (JAVA_ARRAY) local_of(0).data.o);
    exception_raise_if_occurred(vmCurrentContext);

    stack_frame_end();

    return (JAVA_OBJECT) cloned;
}

JAVA_ARRAY array_new(VM_PARAM_CURRENT_CONTEXT, C_CSTR desc, JAVA_INT length) {
    assert(desc);
    assert(desc[0] == TYPE_DESC_ARRAY);

    JAVA_CLASS clazz = classloader_get_class_by_name_init(vmCurrentContext,
                                                          stack_frame_top(vmCurrentContext)->thisClass->classLoader,
                                                          desc);
    if (exception_occurred(vmCurrentContext)) {
        return (JAVA_ARRAY) JAVA_NULL;
    }
    assert(clazz); // TODO: remove this once I figure out how to safely throw exception in classloader

    if (length < 0) {
        exception_set_NegativeArraySizeException(vmCurrentContext, length);
        return (JAVA_ARRAY) JAVA_NULL;
    }

    BasicType elementType = array_type_of(desc);
    size_t objectSize = array_size_of_type(elementType, length);

    JAVA_ARRAY array = heap_alloc(vmCurrentContext, objectSize);
    if (!array) {
        fprintf(stderr, "Unable to alloc array %s with length %d\n", desc, length);
        // TODO: throw OOM exception
        abort();
    }

    array->baseObject.clazz = clazz;
    array->length = length;

    return array;
}

JAVA_VOID array_set_object(VM_PARAM_CURRENT_CONTEXT, JAVA_ARRAY array, JAVA_INT index, JAVA_OBJECT obj) {
    assert(index >= 0); // TODO: throw ArrayIndexOutOfBoundsException instead
    assert(index < array->length); // TODO: throw ArrayIndexOutOfBoundsException instead

    BasicType arrayType = array_type_of(obj_get_class(&array->baseObject)->info->thisClass);

    if (arrayType != VM_TYPE_OBJECT && arrayType != VM_TYPE_ARRAY) {
        fprintf(stderr, "Cannot store object in an array of primitive type\n");
        abort();
    }

    if (obj) {
        // ..., if arrayref is not null and the actual type of
        // value is not assignment compatible (JLS §5.2) with the actual
        // type of the components of the array, aastore throws an
        // ArrayStoreException.
        JavaClassInfo *valueTypeInfo = obj_get_class(obj)->info;
        JavaClassInfo *componentTypeInfo = ((JavaArrayClass *) obj_get_class(&array->baseObject))->componentType->info;
        if (!class_assignable(valueTypeInfo, componentTypeInfo)) {
            exception_set_ArrayStoreException(vmCurrentContext, obj_get_class(&array->baseObject)->info, valueTypeInfo);
            return;
        }
    }

    JAVA_OBJECT *element = array_element_at(array, arrayType, index);
    *element = obj;
}
