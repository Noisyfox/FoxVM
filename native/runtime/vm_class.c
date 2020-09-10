//
// Created by noisyfox on 2020/9/7.
//

#include "vm_class.h"
#include "vm_classloader.h"
#include "vm_stack.h"
#include "classloader/vm_boot_classloader.h"
#include "vm_array.h"

JAVA_BOOLEAN class_is_interface(JavaClassInfo *c) {
    return (c->accessFlags & CLASS_ACC_INTERFACE) == CLASS_ACC_INTERFACE;
}

JAVA_BOOLEAN class_is_array(JavaClassInfo *c) {
    return c->thisClass[0] == TYPE_DESC_ARRAY;
}

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
