//
// Created by noisyfox on 2020/9/1.
//

#ifndef FOXVM_VM_STRING_H
#define FOXVM_VM_STRING_H

#include "vm_base.h"

// Index of pre-defined string constants in the generated constant pool
// Must be sync with [io.noisyfox.foxvm.translator.cgen.StringConstantPool]
#define STRING_CONSTANT_NULL -1
#define STRING_CONSTANT_INDEX_INIT 0 /* "<init>" */
#define STRING_CONSTANT_INDEX_CLINIT 1 /* "<clinit>" */
#define STRING_CONSTANT_INDEX_CLONE 2 /* "clone" */
#define STRING_CONSTANT_INDEX_CLONE_DESCRIPTOR 3 /* "()Ljava/lang/Object;" */

extern FieldInfo *g_field_java_lang_String_value;

C_CSTR string_get_constant_utf8(JAVA_INT constant_index);

JAVA_OBJECT string_get_constant(VM_PARAM_CURRENT_CONTEXT, JAVA_INT constant_index);

JAVA_OBJECT string_create_utf8(VM_PARAM_CURRENT_CONTEXT, C_CSTR utf8);

JAVA_ARRAY string_get_value_array(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT str);

/** Get the length in bytes of the modified UTF-8 representation of a string */
JAVA_INT string_utf8_length_of(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT str);

/**
 * Returns a pointer to an array of bytes representing the string in modified UTF-8 encoding.
 *
 * The returned string is allocated using `heap_alloc_uncollectable()` and it's
 * the caller's responsibility to release it.
 */
C_STR string_utf8_of(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT str);

#endif //FOXVM_VM_STRING_H
