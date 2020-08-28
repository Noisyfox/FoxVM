//
// Created by noisyfox on 2020/8/27.
//

#include "vm_classloader.h"
#include "vm_boot_classloader.h"

JAVA_BOOLEAN classloader_init(VM_PARAM_CURRENT_CONTEXT) {
    if (!cl_bootstrap_init(vmCurrentContext)) {
        return JAVA_FALSE;
    }

    return JAVA_TRUE;
}
