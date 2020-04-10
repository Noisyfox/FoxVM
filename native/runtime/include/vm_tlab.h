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

// The size of each TLAB, 8k
#define TLAB_SIZE_MIN  ((size_t)(8*1024))

// Object larger than 1/4 of TLAB_SIZE will alloc out of TLAB
#define TLAB_MAX_ALLOC_RATIO 4

typedef struct _AllocContext ThreadAllocContext;

struct _AllocContext {
    uint8_t *tlabHead; // beginning of this TLAB
    uint8_t *tlabCurrent;  // current allocation position of TLAB
    uint8_t *tlabLimit; // The end of TLAB
};

#endif //FOXVM_VM_TLAB_H
