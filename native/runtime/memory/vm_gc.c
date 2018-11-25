//
// Created by noisyfox on 2018/11/17.
//

#include <string.h>
#include "vm_gc.h"
#include "vm_memory.h"
#include "opa_primitives.h"

#define SYNC_LOCK_FREE 0
#define SYNC_LOCK_HELD 1

// Header for the young gen
typedef struct {
    OPA_int_t sync; // Global lock for accessing the young gen

    void *edenStart;
    void *largeObjIndex;
    void *tlabIndex;
    void *edenEnd;
    void *survivorMiddle;
    void *youngEnd;
} JavaHeapYoungGen;

typedef struct {
    OPA_int_t sync; // Global lock for accessing the old gen

    uint64_t chunkSize;
    uint64_t chunkCount;
    uint8_t *byteMap;
    void *dataStart;
} JavaHeapOldGen;

// Header for the whole heap
typedef struct {
    uint64_t heapSize;

    uint64_t tlabSize;
    uint64_t tlabMaxAllocSize;
    uint64_t largeObjectSize;

    JavaHeapYoungGen *youngGenHeader;
    JavaHeapOldGen *oldGenHeader;
} JavaHeap;

static JavaHeap *g_heap = NULL;


int heap_init(HeapConfig *config) {
    mem_init();

    // Calculate size of different part of the heap
    uint64_t max_heap_size;
    if (config->maxSize > 0) {
        max_heap_size = config->maxSize;
    } else {
        MemoryStatus status;
        mem_get_status(&status);
        uint64_t max_avail = status.availVirt < status.totalPhys ? status.availVirt : status.totalPhys;
        max_heap_size = max_avail / 2;
    }
    max_heap_size = align_size_down(max_heap_size, mem_page_size());
    // TODO: check if max_heap_size is too small

    uint64_t heap_header_size = align_size_up(sizeof(JavaHeap), mem_page_size());
    uint64_t heap_data_size = max_heap_size - heap_header_size;

    uint64_t young_gen_size = heap_data_size / (config->newRatio + 1);
    young_gen_size = align_size_down(young_gen_size, mem_page_size());

    uint64_t survivor_size = young_gen_size / (config->survivorRatio + 2);
    survivor_size = align_size_down(survivor_size, mem_page_size());

    uint64_t young_gen_header_size = align_size_up(sizeof(JavaHeapYoungGen), mem_alloc_granularity());
    uint64_t eden_size = young_gen_size - young_gen_header_size - survivor_size * 2;

    uint64_t old_gen_size = heap_data_size - young_gen_size;
    uint64_t old_gen_header_size = align_size_up(sizeof(JavaHeapOldGen), mem_alloc_granularity());
    uint64_t old_gen_chunk_size = align_size_up(OLD_GEN_CHUNK_SIZE_MIN, mem_alloc_granularity());
    // Calculate required bytemap size
    uint64_t old_gen_bytemap_data_size = old_gen_size - old_gen_header_size;
    uint64_t old_gen_remain = old_gen_bytemap_data_size % old_gen_chunk_size;
    uint64_t old_gen_chunk_count = old_gen_bytemap_data_size / old_gen_chunk_size;
    while (old_gen_remain < old_gen_chunk_count && old_gen_chunk_count > 0) {
        old_gen_chunk_count--;
        old_gen_remain += old_gen_chunk_size;
    }
    if (is_size_aligned_up(old_gen_remain, mem_alloc_granularity()) != JAVA_TRUE) {
        // Make sure the remaining is aligned
        return -1;
    }
    if (old_gen_chunk_count == 0) {
        return -1;
    }

    // Reserve the whole heap
    void *heap_mem = mem_reserve(NULL, max_heap_size, mem_page_size());

    // Commit the heap header + young gen header
    if (mem_commit(heap_mem, heap_header_size + young_gen_header_size) != JAVA_TRUE) {
        mem_release(heap_mem, max_heap_size);
        return -1;
    }
    memset(heap_mem, 0, heap_header_size + young_gen_header_size);
    JavaHeap *heap = heap_mem;
    // Init heap header
    heap->heapSize = max_heap_size;
    heap->tlabSize = align_size_up(TLAB_SIZE_MIN, mem_alloc_granularity());
    heap->tlabMaxAllocSize = heap->tlabSize / TLAB_MAX_ALLOC_RATIO;
    heap->largeObjectSize = LARGE_OBJECT_SIZE_MIN;

    JavaHeapYoungGen *young_gen = ptr_offset(heap_mem, heap_header_size);
    heap->youngGenHeader = young_gen;
    // Init young gen header
    OPA_store_int(&young_gen->sync, SYNC_LOCK_FREE);
    young_gen->edenStart = ptr_offset(young_gen, young_gen_header_size);
    young_gen->largeObjIndex = young_gen->edenStart;
    young_gen->edenEnd = ptr_offset(young_gen->edenStart, eden_size);
    young_gen->tlabIndex = young_gen->edenEnd;
    young_gen->survivorMiddle = ptr_offset(young_gen->edenEnd, survivor_size);
    young_gen->youngEnd = ptr_offset(young_gen->survivorMiddle, survivor_size);

    // Double check
    if (ptr_offset(young_gen, young_gen_size) != young_gen->youngEnd) {
        mem_release(heap_mem, max_heap_size);
        return -1;
    }

    // Commit old gen header + bytemap
    if (mem_commit(young_gen->youngEnd, old_gen_header_size + old_gen_remain) != JAVA_TRUE) {
        mem_release(heap_mem, max_heap_size);
        return -1;
    }
    memset(young_gen->youngEnd, 0, old_gen_header_size + old_gen_remain);
    JavaHeapOldGen *old_gen = young_gen->youngEnd;
    heap->oldGenHeader = old_gen;
    // Init old gen
    OPA_store_int(&old_gen->sync, SYNC_LOCK_FREE);
    old_gen->chunkSize = old_gen_chunk_size;
    old_gen->chunkCount = old_gen_chunk_count;
    old_gen->byteMap = ptr_offset(old_gen, old_gen_header_size);
    old_gen->dataStart = ptr_offset(old_gen->byteMap, old_gen_remain);

    // Double check
    if (ptr_offset(old_gen->dataStart, old_gen->chunkSize * old_gen->chunkCount) !=
        ptr_offset(heap_mem, max_heap_size)) {
        mem_release(heap_mem, max_heap_size);
        return -1;
    }

    g_heap = heap;
    return 0;
}

void tlab_init(VM_PARAM_CURRENT_CONTEXT, ThreadAllocContext *tlab) {
    tlab->tlabHead = 0;
    tlab->tlabCurrent = 0;
    tlab->tlabLimit = 0;
}

/**
 * Get a new TLAB buffer.
 * @param __vmCurrentThreadContext
 * @return 0 for success, 1 otherwise.
 */
int alloc_tlab(VM_PARAM_CURRENT_CONTEXT) {

}

/**
 * @param start
 * @return usable address after header
 */
static void *init_heap_header(void *start) {
    ObjectHeapHeader *h = start;
    h->flag = HEAP_FLAG_NORMAL;

    return ptr_offset(start, sizeof(ObjectHeapHeader));
}

void *heap_alloc(VM_PARAM_CURRENT_CONTEXT, size_t size) {
    // Make sure the size can contain a ForwardPointer
    if (size < sizeof(ForwardPointer)) {
        size = sizeof(ForwardPointer);
    }
    // Add heap header
    size += sizeof(ObjectHeapHeader);

    if (size <= g_heap->tlabMaxAllocSize) {
        // Try alloc on TLAB
        ThreadAllocContext *tlab = &vmCurrentContext->tlab;

        while (1) {
            uint8_t *result = tlab->tlabCurrent;
            uint8_t *advance = result + size;

            if (advance <= tlab->tlabLimit) {
                // Alloc from TLAB by increasing the current pointer
                tlab->tlabCurrent = advance;

                return init_heap_header(result);
            } else {
                // Need a new TLAB
                int r = alloc_tlab(vmCurrentContext);
                if (r) {
                    return NULL;
                }
            }
        }
    } else if (size > g_heap->largeObjectSize) {
        // Alloc on old gen

    } else {
        // Alloc in yong gen, outside TLAB

    }
}
