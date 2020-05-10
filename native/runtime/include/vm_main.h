//
// Created by noisyfox on 2018/9/23.
//

#ifndef FOXVM_VM_MAIN_H
#define FOXVM_VM_MAIN_H

#include "vm_thread.h"
//#include "vm_exception.h"
//#include "vm_memory.h"
#include "vm_gc.h"
#include "vm_bytecode.h"

/**
 * Preparing the VM and start running.
 *
 * @param argc
 * @param argv
 * @param entrance
 * @return
 */
int vm_main(int argc, char *argv[], JavaMethodRetVoid entrance);

#endif //FOXVM_VM_MAIN_H
