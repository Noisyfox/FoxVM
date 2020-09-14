//
// Created by noisyfox on 2020/9/14.
//

#include "vm_reflection.h"
#include "vm_field.h"
#include "vm_method.h"
#include "classloader/vm_boot_classloader.h"
#include "vm_classloader.h"
#include "vm_string.h"
#include "vm_thread.h"
#include "vm_native.h"
#include "vm_array.h"
#include "vm_exception.h"
#include "vm_gc.h"
#include "vm_class.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Fields in java.lang.reflect.Constructor
static ResolvedField *g_field_java_lang_reflect_Constructor_clazz = NULL;
static ResolvedField *g_field_java_lang_reflect_Constructor_slot = NULL;
static ResolvedField *g_field_java_lang_reflect_Constructor_parameterTypes = NULL;
static ResolvedField *g_field_java_lang_reflect_Constructor_exceptionTypes = NULL;
static ResolvedField *g_field_java_lang_reflect_Constructor_modifiers = NULL;
static ResolvedField *g_field_java_lang_reflect_Constructor_signature = NULL;

#define resolve_field(clazz, name, desc) do {                                   \
    g_field_java_lang_reflect_##clazz##_##name =                                \
        field_find(g_class_java_lang_reflect_##clazz, #name, desc);             \
    if (!g_field_java_lang_reflect_##clazz##_##name) {                          \
        fprintf(stderr, "Reflection: unable to resolve field %s.%s:%s\n",       \
            g_class_java_lang_reflect_##clazz->info->thisClass, #name, desc);   \
        abort();                                                                \
    }                                                                           \
} while(0)

JAVA_VOID reflection_init(VM_PARAM_CURRENT_CONTEXT) {
    resolve_field(Constructor, clazz,          "Ljava/lang/Class;");
    resolve_field(Constructor, slot,           "I");
    resolve_field(Constructor, parameterTypes, "[Ljava/lang/Class;");
    resolve_field(Constructor, exceptionTypes, "[Ljava/lang/Class;");
    resolve_field(Constructor, modifiers,      "I");
    resolve_field(Constructor, signature,      "Ljava/lang/String;");
}

static JAVA_ARRAY reflection_method_get_parameter_types(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT classloader, C_CSTR desc) {
    native_scoped {
        // Store the classloader in the handler
        native_handler_new(h_cl, classloader);

        JAVA_INT parameterCount = method_get_parameter_count(desc);

        // Create a class array
        native_handler_new(h_res, array_new(vmCurrentContext, "[Ljava/lang/Class;", parameterCount));

        // Iter through the parameters
        C_CSTR currentParam = NULL;
        int i = 0;
        while ((currentParam = method_get_next_parameter_type(&desc)) != NULL) {
            JAVA_INT len = desc - currentParam + 1;
            C_STR typeName = heap_alloc_uncollectable(sizeof(char) * (len + 1));
            if (!typeName) {
                // TODO: throw OOM
                fprintf(stderr, "Reflection: unable to allocate %d bytes for type name\n", len + 1);
                abort();
            }
            strncpy(typeName, currentParam, len);

            JAVA_CLASS clazz = classloader_get_class_by_desc(vmCurrentContext,
                                                             native_dereference(vmCurrentContext, h_cl),
                                                             typeName);
            heap_free_uncollectable(typeName);
            native_check_exception();

            // Store the class obj in the array
            array_set_object(vmCurrentContext, (JAVA_ARRAY) native_dereference(vmCurrentContext, h_res), i, clazz->classInstance);

            i++;
        }

        result = h_res;
    } native_scope_end();

    return (JAVA_ARRAY) javaResult;
}

JAVA_OBJECT reflection_method_new_constructor(VM_PARAM_CURRENT_CONTEXT, JAVA_CLASS cls, JAVA_INT index) {
    JavaClassInfo *clazz = cls->info;
    MethodInfo *method = clazz->methods[index];
    assert(method->name == STRING_CONSTANT_INDEX_INIT);

    C_CSTR desc = string_get_constant_utf8(method->descriptor);

    native_scoped {
        native_handler_new(h_sig, string_get_constant(vmCurrentContext, method->signature));
        native_handler_new(h_paramTypes, reflection_method_get_parameter_types(vmCurrentContext, cls->classLoader, desc));
        native_handler_new(h_exTypes, array_new(vmCurrentContext, "[Ljava/lang/Class;", 0)); // TODO

        JAVA_OBJECT constructor = class_alloc_instance(vmCurrentContext, g_class_java_lang_reflect_Constructor);
        native_check_exception();

        *((JAVA_OBJECT*)ptr_inc(constructor, g_field_java_lang_reflect_Constructor_clazz->info.offset)) = cls->classInstance;
        *((JAVA_INT*)ptr_inc(constructor, g_field_java_lang_reflect_Constructor_slot->info.offset)) = index;
        *((JAVA_OBJECT*)ptr_inc(constructor, g_field_java_lang_reflect_Constructor_parameterTypes->info.offset)) = native_dereference(vmCurrentContext, h_paramTypes);
        *((JAVA_OBJECT*)ptr_inc(constructor, g_field_java_lang_reflect_Constructor_exceptionTypes->info.offset)) = native_dereference(vmCurrentContext, h_exTypes);
        *((JAVA_INT*)ptr_inc(constructor, g_field_java_lang_reflect_Constructor_modifiers->info.offset)) = clazz->accessFlags;
        *((JAVA_OBJECT*)ptr_inc(constructor, g_field_java_lang_reflect_Constructor_signature->info.offset)) = native_dereference(vmCurrentContext, h_sig);

        native_handler_of(result, constructor);
    } native_scope_end();

    return javaResult;
}
