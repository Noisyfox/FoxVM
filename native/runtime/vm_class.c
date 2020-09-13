//
// Created by noisyfox on 2020/9/7.
//

#include "vm_class.h"
#include "vm_classloader.h"
#include "vm_stack.h"
#include "classloader/vm_boot_classloader.h"
#include "vm_array.h"
#include "vm_memory.h"
#include "vm_gc.h"
#include <string.h>

JAVA_BOOLEAN class_assignable_desc(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *s, C_CSTR t) {
    VMStackFrame *frame = stack_frame_top(vmCurrentContext);
    JAVA_OBJECT classLoader = frame->thisClass ? frame->thisClass->classLoader : JAVA_NULL;

    JAVA_CLASS tc = classloader_get_class_by_name(vmCurrentContext, classLoader, t);
    assert(tc);

    return class_assignable(s, tc->info);
}

static JAVA_BOOLEAN class_assignable_interface(JavaClassInfo *s, JavaClassInfo *t) {
    if (!class_is_interface(t)) {
        return JAVA_FALSE;
    }
    if (s == t) {
        return JAVA_TRUE;
    }

    for (uint16_t i = 0; i < s->interfaceCount; i++) {
        if (class_assignable_interface(s->interfaces[i], t)) {
            return JAVA_TRUE;
        }
    }

    if (s->superClass) {
        return class_assignable_interface(s->superClass, t);
    }

    return JAVA_FALSE;
}

JAVA_BOOLEAN class_assignable(JavaClassInfo *s, JavaClassInfo *t) {
    // java/lang/Object is the superclass of all other classes
    if (s == t || t == g_classInfo_java_lang_Object) {
        return JAVA_TRUE;
    }

    if (class_is_interface(t)) {
        return class_assignable_interface(s, t);
    }

    if (class_is_interface(s)) {
        // • If S is an interface type, then:
        //   – If T is a class type, then T must be Object.
        //   – If T is an interface type, then T must be the same interface as
        //     S or a superinterface of S.
        return JAVA_FALSE;
    } else if (class_is_array(s)) {
        // • If S is a class representing the array type SC[], that is, an array
        //   of components of type SC, then:
        //   – If T is a class type, then T must be Object.
        //   – If T is an interface type, then T must be one of the interfaces
        //     implemented by arrays (JLS §4.10.3).
        if (class_is_array(t)) {
            // – If T is an array type TC[], that is, an array of components of
            //   type TC, then one of the following must be true:
            //   › TC and SC are the same primitive type.
            //   › TC and SC are reference types, and type SC can be cast to TC
            //     by these run-time rules.
            JavaArrayClass *sac = (JavaArrayClass *) s;
            JavaArrayClass *tac = (JavaArrayClass *) t;

            return class_assignable(sac->componentType->info, tac->componentType->info);
        } else {
            return JAVA_FALSE;
        }
    } else {
        // • If S is an ordinary (nonarray) class, then:
        //   – If T is a class type, then S must be the same class as T, or S
        //     must be a subclass of T;
        //   – If T is an interface type, then S must implement interface T.
        JavaClassInfo *super = s->superClass;
        while (super != NULL) {
            if (super == t) {
                return JAVA_TRUE;
            }
            super = super->superClass;
        }

        return JAVA_FALSE;
    }
}

JAVA_CLASS class_get_native_class(JAVA_OBJECT classObj) {
    return (JAVA_CLASS) *((JAVA_OBJECT *) ptr_inc(classObj, g_field_java_lang_Class_fvmNativeType->offset));
}

C_CSTR class_pretty_descriptor(C_CSTR descriptor) {
    // Count the number of '['s to get the dimensionality.
    C_CSTR c = descriptor;
    size_t dim = 0;
    while (*c == TYPE_DESC_ARRAY) {
        dim++;
        c++;
    }

    // Reference or primitive?
    if (*c == TYPE_DESC_REFERENCE) {
        // "[[La/b/C;" -> "a.b.C[][]".
        c++;  // Skip the 'L'.
    } else {
        // "[[B" -> "byte[][]".
        // To make life easier, we make primitives look like unqualified
        // reference types.
        switch (*c) {
            case TYPE_DESC_BYTE:    c = "byte;"; break;
            case TYPE_DESC_CHAR:    c = "char;"; break;
            case TYPE_DESC_DOUBLE:  c = "double;"; break;
            case TYPE_DESC_FLOAT:   c = "float;"; break;
            case TYPE_DESC_INT:     c = "int;"; break;
            case TYPE_DESC_LONG:    c = "long;"; break;
            case TYPE_DESC_SHORT:   c = "short;"; break;
            case TYPE_DESC_BOOLEAN: c = "boolean;"; break;
            case TYPE_DESC_VOID:    c = "void;"; break;  // Used when decoding return types.
            default: {
                // Make a copy of the descriptor
                size_t len = strlen(descriptor);
                C_STR result = heap_alloc_uncollectable((len + 1) * sizeof(char));
                if (result) {
                    strcpy(result, descriptor);
                }
                return result;
            }
        }
    }

    // At this point, 'c' is a string of the form "fully/qualified/Type;"
    // or "primitive;". Rewrite the type with '.' instead of '/':
    C_STR result = heap_alloc_uncollectable((strlen(c) + dim * 2 + 1) * sizeof(char));
    if (!result) {
        return NULL;
    }
    C_CSTR p = c;
    size_t index = 0;
    while (*p != ';' && *p != '\0') {
        char ch = *p++;
        if (ch == '/') {
            ch = '.';
        }
        result[index++] = ch;
    }
    // ...and replace the semicolon with 'dim' "[]" pairs:
    for (size_t i = 0; i < dim; ++i) {
        result[index++] = '[';
        result[index++] = ']';
    }
    return result;
}
