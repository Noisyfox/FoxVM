//
// Created by noisyfox on 2020/4/16.
//

#include "vm_tlab.h"
#include "vm_gc_priv.h"
#include <assert.h>

size_t tlab_reserve_size() {
    return align_size_up(g_fillerArraySizeMin, SIZE_ALIGNMENT);
}

static inline uint8_t *tlab_limit_hard(ThreadAllocContext *tlab) {
    return ptr_inc(tlab_limit(tlab), tlab_reserve_size());
}

void tlab_retire(ThreadAllocContext *tlab) {
    size_t remaining = ptr_offset(tlab->tlabCurrent, tlab_limit_hard(tlab));

    // Fill the remaining memory with a dummy object
    heap_fill_with_object(tlab->tlabCurrent, remaining);

    // Free the tlab
    tlab_reset(tlab);
}

size_t tlab_calculate_new_size(ThreadAllocContext *tlab, size_t obj_size) {
    assert(is_size_aligned(obj_size, SIZE_ALIGNMENT));

    // Get remaining mem size in gen0
    size_t available_size = heap_gen0_free();
    size_t new_tlab_size = size_min(available_size, tlab->desiredSize + obj_size);

    // Make sure there's enough room for object and filler int[].
    size_t obj_plus_filler_size = obj_size + tlab_reserve_size();

    return size_max(new_tlab_size, obj_plus_filler_size);
}

void tlab_fill(ThreadAllocContext *tlab, void *start, size_t size) {
    assert(size > tlab_reserve_size());

    tlab->tlabHead = start;
    tlab->tlabCurrent = start;
    tlab->tlabLimit = ptr_inc(start, size - tlab_reserve_size());
    tlab->wasteLimit = tlab->desiredSize / TLAB_WASTE_FRACTION;
}
