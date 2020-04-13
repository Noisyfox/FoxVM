//
// Created by noisyfox on 2020/4/13.
//

#include "vm_gc.h"
#include "vm_memory.h"
#include "vm_thread.h"

// Heap segment default size
#ifdef TARGET_64BIT
#define SOH_SEGMENT_ALLOC ((size_t)(1024*1024*256))
#define LOH_SEGMENT_ALLOC ((size_t)(1024*1024*128))
#else
#define SOH_SEGMENT_ALLOC ((size_t)(1024*1024*16))
#define LOH_SEGMENT_ALLOC ((size_t)(1024*1024*16))
#endif //TARGET_64BIT

// The initial commit size when creating a new segment
#define SEGMENT_INITIAL_COMMIT (mem_page_size())
// Make sure the start memory is aligned
#define SEGMENT_START_OFFSET (align_size_up(sizeof(HeapSegment), DATA_ALIGNMENT))
// The segment alignment required by card table
#define SEGMENT_ALIGNMENT DATA_ALIGNMENT

typedef struct _HeapSegment {
    uint8_t *start; // The start memory that can be used for object alloc
    uint8_t *committed; // The end of committed memory
    uint8_t *end; // The end of the segment
    uint32_t flags;

    struct _HeapSegment *next; // Pointer to next chained segment
} HeapSegment;

/**
 * Allocate a new heap segment with the given size.
 *
 * @param size the size of the segment, include the header size. Can't be smaller than `SEGMENT_INITIAL_COMMIT`.
 * @return NULL if can't allocate new memory. Otherwise a new HeapSegment* will be returned,
 * the address is aligned with `SEGMENT_ALIGNMENT`.
 */
static HeapSegment *alloc_heap_segment(size_t size) {
    // Reserve memory
    void *mem = mem_reserve(NULL, size, SEGMENT_ALIGNMENT);
    if (!mem) {
        return NULL;
    }

    // Commit the first page
    if (mem_commit(mem, SEGMENT_INITIAL_COMMIT) != JAVA_TRUE) {
        mem_release(mem, size);
        return NULL;
    }

    HeapSegment *segment = mem;

    // Init each field
    segment->start = ptr_inc(mem, SEGMENT_START_OFFSET);
    segment->committed = ptr_inc(mem, SEGMENT_INITIAL_COMMIT);
    segment->end = ptr_inc(mem, size);
    segment->flags = 0;
    segment->next = NULL;

    return segment;
}

typedef struct {
    // The size of each SOH segment
    size_t soh_segment_size;

    // Minimum LOH segment size
    size_t min_loh_segment_size;

    // The current max possible memory range of the heap, which is covered by card table.
    uint8_t *lowest_addr;
    uint8_t *highest_addr;
} JavaHeap;

static JavaHeap g_heap = {0};

int heap_init(VM_PARAM_CURRENT_CONTEXT, HeapConfig *config) {
    // Init low level memory system first
    if (!mem_init()) {
        return -1;
    }

    // Determine the size of each heap segment
    g_heap.soh_segment_size = SOH_SEGMENT_ALLOC;
    g_heap.min_loh_segment_size = LOH_SEGMENT_ALLOC;

    // Create first SOH and LOH segment
    HeapSegment *soh_seg = alloc_heap_segment(g_heap.soh_segment_size);
    HeapSegment *loh_seg = alloc_heap_segment(g_heap.min_loh_segment_size);
    if (!soh_seg || !loh_seg) {
        // Something went wrong
        return -1;
    }

    // Store the current memory range
    g_heap.lowest_addr = ptr_min(soh_seg, loh_seg);
    g_heap.highest_addr = ptr_max(soh_seg->end, loh_seg->end);

    return 0;
}
