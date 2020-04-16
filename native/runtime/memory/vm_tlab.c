//
// Created by noisyfox on 2020/4/16.
//

#include "vm_tlab.h"
#include "vm_gc_priv.h"


void tlab_retire(ThreadAllocContext *tlab) {
    // Fill the remaining memory with a dummy object
    heap_fill_with_object(tlab->tlabCurrent, tlab_free(tlab));

    // Free the tlab
    tlab_reset(tlab);
}
