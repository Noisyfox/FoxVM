//
// Created by noisyfox on 2020/4/10.
//
// Thread-local allocation buffer.
// For small object only, allocated inside gen 0 (eden). Each gen 0 gc will reclaim ALL tlabs
// from all threads, then if a thread need to allocate small object afterwards, a new tlab will
// be allocated to that thread.
//

#ifndef FOXVM_VM_TLAB_H
#define FOXVM_VM_TLAB_H

#include "vm_base.h"
#include "vm_memory.h"

// The minimum size of each TLAB, 8k
#define TLAB_SIZE_MIN  ((size_t)(8*1024))

// By default the waste limit is 1/64 of the size
#define TLAB_WASTE_FRACTION 64

typedef struct _AllocContext ThreadAllocContext;

struct _AllocContext {
    uint8_t *tlabHead; // beginning of this TLAB
    uint8_t *tlabCurrent;  // current allocation position of TLAB
    uint8_t *tlabLimit; // The end of TLAB, reserved size excluded.
    size_t wasteLimit; // Don't discard TLAB if remaining space is larger than this.

    size_t desiredSize; // Desired TLAB size of this thread.
};

/** Init the tlab */
void tlab_init(ThreadAllocContext *tlab);

/** Reset the given tlab to un-allocated. */
static inline void tlab_reset(ThreadAllocContext *tlab) {
    tlab->tlabHead = 0;
    tlab->tlabCurrent = 0;
    tlab->tlabLimit = 0;
    tlab->wasteLimit = 0;
}

/** Fill the tlab with given memory and size */
void tlab_fill(ThreadAllocContext *tlab, void *start, size_t size);

static inline uint8_t *tlab_limit(ThreadAllocContext *tlab) {
    return tlab->tlabLimit;
}

/**
 * Total space in the tlab that can be used for object allocation.
 * The reserved size not included.
 */
static inline size_t tlab_size(ThreadAllocContext *tlab) {
    return ptr_offset(tlab->tlabHead, tlab_limit(tlab));
}

/**
 * Remaining free space in the tlab that can be used for object allocation.
 * The reserved size not included.
 */
static inline size_t tlab_free(ThreadAllocContext *tlab) {
    return ptr_offset(tlab->tlabCurrent, tlab_limit(tlab));
}

/** Whether the given tlab is allocated. */
static inline JAVA_BOOLEAN tlab_allocated(ThreadAllocContext *tlab) {
    return tlab_size(tlab) == 0 ? JAVA_FALSE : JAVA_TRUE;
}

/** Compute the size for the new TLAB. */
size_t tlab_calculate_new_size(ThreadAllocContext *tlab, size_t obj_size);

/**
 * Fills the current tlab with a dummy filler array to create an illusion
 * of a contiguous heap.
 *
 * The remaining space in the tlab is guaranteed to be larger than `g_fillerArraySizeMin`
 * so a fill array can be fit.
 */
void tlab_retire(ThreadAllocContext *tlab);

#endif //FOXVM_VM_TLAB_H
