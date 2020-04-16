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

// The size of each TLAB, 8k
#define TLAB_SIZE_MIN  ((size_t)(8*1024))

// Object larger than 1/4 of TLAB_SIZE will alloc out of TLAB
#define TLAB_MAX_ALLOC_RATIO 4

typedef struct _AllocContext ThreadAllocContext;

struct _AllocContext {
    uint8_t *tlabHead; // beginning of this TLAB
    uint8_t *tlabCurrent;  // current allocation position of TLAB
    uint8_t *tlabLimit; // The end of TLAB
    size_t waste_limit; // Don't discard TLAB if remaining space is larger than this.

    size_t desired_size; // Desired TLAB size of this thread.
};

/**
 * Reset the given tlab to un-allocated.
 */
static inline void tlab_reset(ThreadAllocContext *tlab) {
    tlab->tlabHead = 0;
    tlab->tlabCurrent = 0;
    tlab->tlabLimit = 0;
    tlab->waste_limit = 0;
}

static inline size_t tlab_size(ThreadAllocContext *tlab) {
    return ptr_offset(tlab->tlabLimit, tlab->tlabHead);
}

static inline size_t tlab_free(ThreadAllocContext *tlab) {
    return ptr_offset(tlab->tlabCurrent, tlab->tlabLimit);
}

/** Whether the given tlab is allocated. */
static inline JAVA_BOOLEAN tlab_allocated(ThreadAllocContext *tlab) {
    return tlab_size(tlab) == 0 ? JAVA_FALSE : JAVA_TRUE;
}

/**
 * Fills the current tlab with a dummy filler array to create an illusion
 * of a contiguous heap.
 *
 * The remaining space in the tlab must be at least `MIN_OBJECT_SIZE` bytes.
 */
void tlab_retire(ThreadAllocContext *tlab);

#endif //FOXVM_VM_TLAB_H
