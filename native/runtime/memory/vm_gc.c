//
// Created by noisyfox on 2020/4/13.
//

#include "vm_gc.h"
#include "vm_memory.h"
#include "vm_thread.h"

/*
 * Card table defs and common functions
 *
 * A card table is used for tracking cross-generation references. Since when collecting an older
 * generation, we guarantee to also collect all younger generations, we only need to track references
 * from older generation to younger generation.
 *
 * The card table uses 1 bit to represent 1<<CARD_BYTE_SHIFT bits of heap. However to prevent
 * issues when updating each bits concurrently, we simply mark the entire byte in the write barrier,
 * which means that in actuality the minimum unit tracked by the card table is a 1<<CARD_BYTE_SHIFT bytes
 * region of the heap.
 *
 * If the pointer value being written refers to an object in the ephemeral generation (gen0 + gen1,
 * which is the only generation from which objects are compacted), then the card table must be updated
 * to reflect the location of this pointer. The byte location to be updated is obtained using the
 * `card_byte()` function, and 0xFF is masked into it.
 */

#ifdef TARGET_64BIT
// 1 byte in card table -> 1<<11 bytes (2KB) of memory
#define CARD_BYTE_SHIFT 11
#else
// 1 byte in card table -> 1<<10 bytes (1KB) of memory
#define CARD_BYTE_SHIFT 10
#endif // TARGET_64BIT

#define card_byte(addr) (((size_t)(addr)) >> CARD_BYTE_SHIFT)

// Returns the number of BYTEs in the card table that cover the
// range of addresses [from, to[.
size_t card_count_of(void *from, void *to) {
    return card_byte(ptr_dec(to, 1)) - card_byte(from) + 1;
}

// Returns the number of bytes to allocate for a card table
// that covers the range of addresses [from, to[.
size_t card_size_of(void *from, void *to) {
    return card_count_of(from, to) * sizeof(uint8_t);
}

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

/**
 * Create the initial card table & brick table that covers the current heap address range.
 */
static int init_card_table() {
    size_t cs = card_size_of(g_heap.lowest_addr, g_heap.highest_addr);
    return 0;
}

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

    // Create card table from current memory range
    int res = init_card_table();
    if (res) {
        return res;
    }

    return 0;
}
