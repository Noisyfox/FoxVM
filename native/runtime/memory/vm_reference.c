//
// Created by noisyfox on 2018/10/24.
//

#include "vm_reference.h"
#include <stdint.h>
#include <string.h>
#include "vm_memory.h"

#define BLOCK_LENGTH 64

typedef struct _NativeReference     NativeReference;
typedef struct _ReferenceRowMeta    ReferenceRowMeta;
typedef struct _ReferenceRow        ReferenceRow;
typedef struct _ReferenceBlockMeta  ReferenceBlockMeta;
typedef struct _ReferenceBlock      ReferenceBlock;
typedef union _ReferenceRowUnion    ReferenceRowUnion;
typedef union _NativeReferenceUnion NativeReferenceUnion;

struct _ReferenceBlock {
    union _ReferenceRowUnion {
        struct _ReferenceBlockMeta {
            volatile uint64_t bitmap;
        } meta;

        struct _ReferenceRow {
            union _NativeReferenceUnion {
                struct _ReferenceRowMeta {
                    volatile uint64_t bitmap;
                } meta;

                struct _NativeReference {
                    OPA_ptr_t targetObject; // Not NULL!
                } ref;
            } refs[BLOCK_LENGTH];
        } row;
    } rows[BLOCK_LENGTH];
};

#define BLOCK_ALIGN sizeof(ReferenceBlock)
#define BLOCK_ALIGN_MASK (~((uintptr_t)(sizeof(ReferenceBlock) - 1)))

#define BLOCK_MAX_COUNT 1024
#define BITMAP_AXIS_FULL 0xFFFFFFFFFFFFFFFEull
enum {
    LOCK_IS_FREE = 0, LOCK_IS_TAKEN = 1
};
static OPA_int_t block_pool_lock = OPA_INT_T_INITIALIZER(LOCK_IS_FREE);
static ReferenceBlock *block_pool[BLOCK_MAX_COUNT] = {0};

static inline void pool_lock() {
    while (OPA_cas_int(&block_pool_lock, LOCK_IS_FREE, LOCK_IS_TAKEN) == LOCK_IS_TAKEN) {
        OPA_busy_wait();
    }
}

static inline void pool_unlock() {
    OPA_store_int(&block_pool_lock, LOCK_IS_FREE);
}


static NativeReference *ref_pick() {
    NativeReference *ref = NULL;

    pool_lock();
    // Find a free reference
    for (int i = 0; i < BLOCK_MAX_COUNT; i++) {
        ReferenceBlock *b = block_pool[i];
        if (b == NULL) {
            b = mem_aligned_malloc(sizeof(ReferenceBlock), BLOCK_ALIGN);
            if (b == NULL) {
                // Alloc fail
                break;
            }

            memset(b, 0, sizeof(ReferenceBlock));
            block_pool[i] = b;
        }

        // Find a free row
        ReferenceBlockMeta *block_meta = &b->rows[0].meta;
        uint64_t row_bitmap = block_meta->bitmap;
        if (row_bitmap != BITMAP_AXIS_FULL) {
            size_t row;
            for (row = 1; row < BLOCK_LENGTH; row++) {
                if ((((((uint64_t) 1) << row)) & row_bitmap) == 0) {
                    break;
                }
            }
            if (row >= BLOCK_LENGTH) {
                // Something wrong...?
                // TODO: log
            } else {
                // Find a free slot
                ReferenceRow *row_p = &b->rows[row].row;
                ReferenceRowMeta *row_meta = &row_p->refs[0].meta;
                uint64_t col_bitmap = row_meta->bitmap;

                if (col_bitmap != BITMAP_AXIS_FULL) {
                    // Something wrong...?
                    // TODO: log
                } else {
                    size_t col;
                    for (col = 1; col < BLOCK_LENGTH; col++) {
                        if ((((((uint64_t) 1) << col)) & col_bitmap) == 0) {
                            break;
                        }
                    }
                    if (col >= BLOCK_LENGTH) {
                        // Something wrong...?
                        // TODO: log
                    } else {
                        // Find an empty slot!
                        ref = &row_p->refs[col].ref;

                        // Update bitmap
                        col_bitmap |= ((uint64_t) 1) << col;
                        row_meta->bitmap = col_bitmap; // Col bitmap
                        if (col_bitmap == BITMAP_AXIS_FULL) {
                            block_meta->bitmap = row_bitmap | (((uint64_t) 1) << row); // Row bitmap
                        }
                        break;
                    }
                }
            }
        }
    }
    pool_unlock();

    return ref;
}

static void ref_put_back(NativeReference *ref) {
    // Clear reference
    OPA_store_ptr(&ref->targetObject, NULL);

    // Calculate block address
    ReferenceBlock *block = (ReferenceBlock *) (((uintptr_t) ref) & BLOCK_ALIGN_MASK);
    uintptr_t offset = ((uintptr_t) ref) - ((uintptr_t) block);
    size_t row = (size_t) (offset / sizeof(ReferenceRowUnion));
    size_t column = (size_t) ((offset % sizeof(ReferenceRowUnion)) / sizeof(NativeReferenceUnion));

    pool_lock();
    // Update bitmap
    block->rows[row].row.refs[0].meta.bitmap &= ~(((uint64_t) 1) << column);
    block->rows[0].meta.bitmap &= ~(((uint64_t) 1) << row);
    pool_unlock();
}


JAVA_REF ref_obtain(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj) {
    // TODO: make sure not in checkpoint
    OPA_ptr_t *ptr = &obj->ref;
    NativeReference *ref = OPA_load_ptr(ptr);

    if (ref) {// Reference already created
        return ref;
    }

    // Create new reference
    ref = ref_pick(); // TODO: check NULL
    OPA_store_ptr(&ref->targetObject, obj);
    NativeReference *prev_ref = OPA_cas_ptr(ptr, NULL, ref);
    if (prev_ref != NULL) {
        // Raise condition. Already ref-ed by another thread, so we release this ref.
        ref_put_back(ref);
        ref = prev_ref;
    }
    return ref;
}

JAVA_REF ref_update(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj) {
    OPA_ptr_t *ptr = &obj->ref;
    NativeReference *ref = OPA_load_ptr(ptr);

    if (ref) {
        OPA_store_ptr(&ref->targetObject, obj);
    }

    return ref;
}

JAVA_OBJECT ref_dereference(VM_PARAM_CURRENT_CONTEXT, JAVA_REF ref) {
    // TODO: make sure not in checkpoint
    return OPA_load_ptr(&((NativeReference *) ref)->targetObject);
}
