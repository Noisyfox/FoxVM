//
// Created by noisyfox on 2020/4/16.
//

#include "vm_gc_priv.h"
#include "vm_memory.h"
#include "vm_array.h"
#include <assert.h>

size_t g_fillerArraySizeMax = 0;
size_t g_fillerArraySizeMin = 0;
size_t g_fillerSizeMin = 0;

void heap_fill_with_object(void *start, size_t size) {
    assert(size <= g_fillerArraySizeMax);
    assert(is_ptr_aligned(start, DATA_ALIGNMENT));
    assert(is_size_aligned(size, SIZE_ALIGNMENT));

    if (size >= g_fillerArraySizeMin) {
        // Fill the memory using int array
        JAVA_ARRAY array = start;
        size_t element_size = ptr_offset(array_base(array, VM_TYPE_INT), ptr_inc(start, size));
        size_t element_count = element_size / type_size(VM_TYPE_INT);
        assert(size == array_size_of_type(VM_TYPE_INT, element_count));

        // Set class
        assert(g_class_array_I != NULL);
        array->baseObject.clazz = g_class_array_I;
        array->baseObject.monitor = NULL;
        // Set the array properties
        array->length = element_count;
    } else if (size > 0) {
        assert(size == g_fillerSizeMin);
        // Fill the memory with a plain object
        JAVA_OBJECT object = start;
        // TODO: set class
    }
}
