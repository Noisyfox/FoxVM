//
// Created by noisyfox on 2018/10/20.
//

#include "vm_thread.h"
#include "vm_array.h"
#include "vm_memory.h"
#include "vm_bytecode.h"

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

/**
 * Impl of array.clone() method
 */
JAVA_OBJECT g_array_5Mclone_R9Pjava_lang6CObject(VM_PARAM_CURRENT_CONTEXT) {
    stack_frame_start(0, 0, 1);
    bc_prepare_arguments(1);
    bc_check_objectref();

    // TODO
    assert(!"Not implemented");

    stack_frame_end();
    return JAVA_NULL;
}
