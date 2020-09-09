//
// Created by noisyfox on 2018/9/23.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_exception.h"
#include "vm_class.h"
#include <setjmp.h>

JAVA_BOOLEAN exception_matches(JAVA_OBJECT ex, int32_t current_sp, int32_t start, int32_t end, JavaClassInfo *type) {
    if (current_sp < start || current_sp >= end) {
        return JAVA_FALSE;
    }

    if (type == NULL) {
        return JAVA_TRUE;
    }

    return class_assignable(obj_get_class(ex)->info, type);
}
