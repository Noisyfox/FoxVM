//
// Created by noisyfox on 2020/4/16.
//

#ifndef FOXVM_VM_GC_PRIV_H
#define FOXVM_VM_GC_PRIV_H

#include "vm_base.h"

// Utilities for turning raw memory into filler objects.
extern size_t g_fillerArraySizeMax;  // The largest region that can be filled with a single int array
extern size_t g_fillerArraySizeMin;  // The smallest region that can be filled with a single int array
extern size_t g_fillerSizeMin; // The smallest region that can be filled.
/** Fill the memory region with a single object. */
void heap_fill_with_object(void *start, size_t size);

/** Get the free memory size of gen0. */
size_t heap_gen0_free();

/** Make sure the end of the tlab can fit a fill array. */
size_t tlab_reserve_size();

#endif //FOXVM_VM_GC_PRIV_H
