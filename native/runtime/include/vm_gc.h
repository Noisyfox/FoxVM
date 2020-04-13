//
// Created by noisyfox on 2020/4/13.
//

#ifndef FOXVM_VM_GC_H
#define FOXVM_VM_GC_H

#include "vm_base.h"

typedef struct {

} HeapConfig;

/**
 * Initialize global application heap.
 * @return 0 if success, 1 otherwise.
 */
int heap_init(VM_PARAM_CURRENT_CONTEXT, HeapConfig *config);

#endif //FOXVM_VM_GC_H
