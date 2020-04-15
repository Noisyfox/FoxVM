//
// Created by noisyfox on 2020/4/13.
//

#include "vm_gc.h"
#include "vm_memory.h"
#include "vm_thread.h"
#include <assert.h>

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

// codes for the brick entries:
// entry == 0 -> not assigned
// entry >0 offset is entry-1
// entry <0 jump back entry bricks

// 1 brick covers the region of 2 cards
#define BRICK_SIZE (((size_t)1) << (CARD_BYTE_SHIFT + 1))

size_t brick_count_of(void *from, void *to) {
    assert(is_ptr_aligned(from, BRICK_SIZE) == JAVA_TRUE);
    assert(is_ptr_aligned(to, BRICK_SIZE) == JAVA_TRUE);

    return ptr_offset(from, to) / BRICK_SIZE;
}

size_t brick_size_of(void *from, void *to) {
    return brick_count_of(from, to) * sizeof(int16_t);
}

typedef struct _CardTable {
    // The current max possible memory range covered by this card table
    uint8_t *lowest_addr;
    uint8_t *highest_addr;

    int16_t *brick_table;

    size_t size; // Size of the entire card table, include the header and brick table
    struct _CardTable *next;  // Pointer to next chained card table
} CardTable;

/**
 * Since the normal card table covers the memory range of [lowest_addr, highest_addr[,
 * so to get the index of any give memory address `addr`, you have to subtract `lowest_addr` first
 * then divide it by the card size (1<<CARD_BYTE_SHIFT), which then means each time you assign a reference,
 * the write barrier needs to look up the `lowest_addr` from another memory location first which could impact
 * the performance.
 *
 * By 'translating' the card table, we map the table to the entire virtual memory space, so the following two
 * expressions are equal:
 * ptr_inc(ct, sizeof(CardTable) + ptr_offset(lowest_addr, addr) >> CARD_BYTE_SHIFT)  // use original card table
 * ptr_inc(translated_ct, addr >> CARD_BYTE_SHIFT)                                    // use translated card table
 *
 * By rearranging the equation, we have:
 * translated_ct = ptr_inc(ct, sizeof(CardTable) - lowest_addr >> CARD_BYTE_SHIFT)
 *
 * After the translate, the card table byte of giving `addr` can be easily found by:
 * ptr_inc(translated_ct, card_byte(addr))
 */
uint8_t *card_table_translate(CardTable *ct) {
    uint8_t *base_addr = ptr_inc(ct, sizeof(CardTable));

    return ptr_dec(base_addr, card_byte(ct->lowest_addr));
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
// The segment alignment required by card table / brick table
#define SEGMENT_ALIGNMENT size_max(mem_page_size(), BRICK_SIZE)

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

typedef enum {
    // small object heap includes generations [0-2], which are "generations" in the general sense.
    soh_gen0 = 0,
    soh_gen1 = 1,
    soh_gen2 = 2,
    max_generation = soh_gen2,

    // large object heap, technically not a generation, but it is convenient to represent it as such
    loh_generation = 3,

    // number of ephemeral generations
    ephemeral_generation_count = max_generation,

    // number of all generations
    total_generation_count = loh_generation + 1
} GCGeneration;

// Memory info of each generation
typedef struct {
    GCGeneration gen;
    HeapSegment *start_segment; // The head of the chained segments that used by this generation
    HeapSegment *allocation_segment; // The segment currently used to allocate
    uint8_t *allocation_current; // Current allocation address, points to a position on `allocation_segment`
} Generation;

typedef struct {
    // The size of each SOH segment
    size_t soh_segment_size;

    // Minimum LOH segment size
    size_t min_loh_segment_size;

    // The current max possible memory range of the heap, which is covered by card table.
    uint8_t *lowest_addr;
    uint8_t *highest_addr;

    CardTable *card_table;
    uint8_t *card_table_translated;

    // The generation table
    Generation generations[total_generation_count];
} JavaHeap;

static JavaHeap g_heap = {0};

/**
 * Create the initial card table & brick table that covers the current heap address range.
 */
static int card_table_init() {
    // Required size of the card table content
    size_t cs = card_size_of(g_heap.lowest_addr, g_heap.highest_addr);
    // Required size of the brick table
    size_t bs = brick_size_of(g_heap.lowest_addr, g_heap.highest_addr);

    // Allocate memory
    size_t alloc_size = sizeof(CardTable) + cs + bs;
    void *mem = mem_reserve(NULL, alloc_size, ANY_ALIGNMENT);
    if (!mem) {
        return -1;
    }

    // Commit memory
    if (mem_commit(mem, alloc_size) != JAVA_TRUE) {
        mem_release(mem, alloc_size);
        return -1;
    }

    CardTable *card_table = mem;

    // Init the card table
    card_table->lowest_addr = g_heap.lowest_addr;
    card_table->highest_addr = g_heap.highest_addr;
    card_table->brick_table = ptr_inc(mem, sizeof(CardTable) + cs);
    card_table->size = alloc_size;
    card_table->next = NULL;

    g_heap.card_table = card_table;

    // Translate card table
    g_heap.card_table_translated = card_table_translate(card_table);

    return 0;
}

static inline Generation *generation_of(GCGeneration n) {
    assert (((n < total_generation_count) && (n >= soh_gen0)));

    return &g_heap.generations[n];
}

#define youngest_generation (generation_of (soh_gen0))
#define large_object_generation (generation_of (loh_generation))

static void generation_make(GCGeneration gen, HeapSegment *seg) {
    Generation *generation = generation_of(gen);

    generation->gen = gen;
    generation->start_segment = seg;
    generation->allocation_segment = seg;
    generation->allocation_current = seg->start;
}

int heap_init(VM_PARAM_CURRENT_CONTEXT, HeapConfig *config) {
    // Init low level memory system first
    if (!mem_init()) {
        return -1;
    }

    // Determine the size of each heap segment
    g_heap.soh_segment_size = align_size_up(SOH_SEGMENT_ALLOC, SIZE_ALIGNMENT);
    g_heap.min_loh_segment_size = align_size_up(LOH_SEGMENT_ALLOC, SIZE_ALIGNMENT);

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
    int res = card_table_init();
    if (res) {
        return res;
    }

    // Init soh generations
    for (int i = max_generation; i >= 0; i--) {
        generation_make(i, soh_seg);
    }

    // Init loh generation
    generation_make(loh_generation, loh_seg);

    return 0;
}
