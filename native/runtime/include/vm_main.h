//
// Created by noisyfox on 2018/9/23.
//

#ifndef FOXVM_VM_MAIN_H
#define FOXVM_VM_MAIN_H

#include "vm_exception.h"
#include "vm_memory.h"
#include "vm_gc.h"

// TODO: change type of args to string array once available
typedef JAVA_VOID (*VMMainEntrance)(VM_PARAM_CURRENT_CONTEXT, JAVA_CLASS clazz, void *args);

/**
 * Preparing the VM and start running.
 *
 * @param argc
 * @param argv
 * @param entrance
 * @return
 */
int vm_main(int argc, char *argv[], VMMainEntrance entrance);

#endif //FOXVM_VM_MAIN_H
