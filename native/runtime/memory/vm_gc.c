//
// Created by noisyfox on 2020/4/13.
//

#include "vm_gc.h"
#include "vm_gc_priv.h"
#include "vm_memory.h"
#include "vm_thread.h"
#include "vm_array.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

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
static inline size_t card_count_of(void *from, void *to) {
    return card_byte(ptr_dec(to, 1)) - card_byte(from) + 1;
}

// Returns the number of bytes to allocate for a card table
// that covers the range of addresses [from, to[.
static inline size_t card_size_of(void *from, void *to) {
    return card_count_of(from, to) * sizeof(uint8_t);
}

// codes for the brick entries:
// entry == 0 -> not assigned
// entry >0 offset is entry-1
// entry <0 jump back entry bricks

// 1 brick covers the region of 2 cards
#define BRICK_SIZE (((size_t)1) << (CARD_BYTE_SHIFT + 1))
// Value to jump back to previous brick
#define BRICK_VAL_PREVIOUS ((int16_t)-1)

static inline size_t brick_count_of(void *from, void *to) {
    assert(is_ptr_aligned(from, BRICK_SIZE) == JAVA_TRUE);
    assert(is_ptr_aligned(to, BRICK_SIZE) == JAVA_TRUE);

    return ptr_offset(from, to) / BRICK_SIZE;
}

static inline size_t brick_size_of(void *from, void *to) {
    return brick_count_of(from, to) * sizeof(int16_t);
}

typedef struct _CardTable {
    // The current max possible memory range covered by this card table
    uint8_t *lowestAddr;
    uint8_t *highestAddr;

    int16_t *brickTable;

    size_t size; // Size of the entire card table, include the header and brick table
    struct _CardTable *next;  // Pointer to next chained card table
} CardTable;

/**
 * Since the normal card table covers the memory range of [lowestAddr, highestAddr[,
 * so to get the index of any give memory address `addr`, you have to subtract `lowestAddr` first
 * then divide it by the card size (1<<CARD_BYTE_SHIFT), which then means each time you assign a reference,
 * the write barrier needs to look up the `lowestAddr` from another memory location first which could impact
 * the performance.
 *
 * By 'translating' the card table, we map the table to the entire virtual memory space, so the following two
 * expressions are equal:
 * ptr_inc(ct, sizeof(CardTable) + ptr_offset(lowestAddr, addr) >> CARD_BYTE_SHIFT)  // use original card table
 * ptr_inc(translated_ct, addr >> CARD_BYTE_SHIFT)                                    // use translated card table
 *
 * By rearranging the equation, we have:
 * translated_ct = ptr_inc(ct, sizeof(CardTable) - lowestAddr >> CARD_BYTE_SHIFT)
 *
 * After the translate, the card table byte of giving `addr` can be easily found by:
 * ptr_inc(translated_ct, card_byte(addr))
 */
static inline uint8_t *card_table_translate(CardTable *ct) {
    uint8_t *base_addr = ptr_inc(ct, sizeof(CardTable));

    return ptr_dec(base_addr, card_byte(ct->lowestAddr));
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
    uint8_t *allocated; // The end of allocated memory
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
static HeapSegment *heap_segment_alloc(size_t size) {
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
    segment->allocated = segment->start;
    segment->committed = ptr_inc(mem, SEGMENT_INITIAL_COMMIT);
    segment->end = ptr_inc(mem, size);
    segment->flags = 0;
    segment->next = NULL;

    return segment;
}

/**
 * Commit more space to the end of the given segment.
 *
 * @return the actual space that is committed. 0 means commit failure.
 */
static size_t heap_segment_grow(HeapSegment *segment, size_t required_size) {
    assert(ptr_inc(segment->committed, required_size) <= (void *) segment->end);

    // Calculate new committed end
    void *committed = ptr_inc(segment->committed, required_size);
    committed = align_ptr(committed, mem_page_size());
    committed = ptr_min(committed, segment->end);

    // Commit the memory
    size_t commit_size = ptr_offset(segment->committed, committed);
    assert(commit_size >= required_size);
    if (mem_commit(segment->committed, commit_size) != JAVA_TRUE) {
        // Commit failed
        return 0;
    }

    // Update segment info
    segment->committed = committed;

    return required_size;
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

typedef struct {
    size_t minSize; // Minimum allocation budget
    size_t maxSize; // Maximum allocation budget
} StaticData;

// Statistic data of the generation
typedef struct {
    size_t allocBudget; // Desired allocation size before triggering a GC
    ptrdiff_t runningBudget; // The size of how much space is left for allocations until the next GC under the current allocation budget

    size_t collectionCount; // GC count of this generation
} DynamicData;

// Memory info of each generation
typedef struct {
    GCGeneration gen;
    HeapSegment *startSegment; // The head of the chained segments that used by this generation
    HeapSegment *allocationSegment; // The segment currently used to allocate
    uint8_t *allocationStart; // The start address of this generation

    StaticData staticData;
    DynamicData dynamicData;
} Generation;

typedef struct {
    void *addressLow; // Lowest address being condemned
    void *addressHigh; // Highest address being condemned
} GCContext;

typedef struct {
    // The size of each SOH segment
    size_t sohSegmentSize;

    // Minimum LOH segment size
    size_t minLohSegmentSize;

    // The current max possible memory range of the heap, which is covered by card table.
    uint8_t *lowestAddr;
    uint8_t *highestAddr;

    CardTable *cardTable;
    uint8_t *cardTableTranslated;

    // The generation table
    Generation generations[total_generation_count];

    // Lock when alloc directly from heap
    VMSpinLock moreSpaceLockSoh;
    VMSpinLock moreSpaceLockLoh;

    // Fields used by GC
    VMSpinLock gcLock;
    GCContext gcContext;
} JavaHeap;

static JavaHeap g_heap = {0};

/**
 * Create the initial card table & brick table that covers the current heap address range.
 */
static int card_table_init() {
    // Required size of the card table content
    size_t cs = card_size_of(g_heap.lowestAddr, g_heap.highestAddr);
    // Required size of the brick table
    size_t bs = brick_size_of(g_heap.lowestAddr, g_heap.highestAddr);

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
    card_table->lowestAddr = g_heap.lowestAddr;
    card_table->highestAddr = g_heap.highestAddr;
    card_table->brickTable = ptr_inc(mem, sizeof(CardTable) + cs);
    card_table->size = alloc_size;
    card_table->next = NULL;

    g_heap.cardTable = card_table;

    // Translate card table
    g_heap.cardTableTranslated = card_table_translate(card_table);

    return 0;
}

typedef size_t Brick;

/** Get the index of brick of given address */
static inline Brick brick_of(void *addr) {
    return ptr_offset(g_heap.cardTable->lowestAddr, addr) / BRICK_SIZE;
}

/** Get the start address of the region covered by brick b */
static inline void *brick_start_addr_of(Brick b) {
    return ptr_inc(g_heap.cardTable->lowestAddr, b * BRICK_SIZE);
}

/** Set the given block b to value val */
static inline void brick_set(Brick b, int16_t val) {
    g_heap.cardTable->brickTable[b] = val;
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
    generation->startSegment = seg;
    generation->allocationSegment = seg;
    generation->allocationStart = seg->start;
}

size_t heap_gen0_free() {
    Generation *gen0 = youngest_generation;
    return ptr_offset(gen0->allocationSegment->allocated, gen0->allocationSegment->end);
}

int heap_init(VM_PARAM_CURRENT_CONTEXT, HeapConfig *config) {
    // Init gc constants
    g_fillerArraySizeMax = array_max_size_of_type(VM_TYPE_INT);
    g_fillerArraySizeMin = array_min_size_of_type(VM_TYPE_INT);
    g_fillerSizeMin = MIN_OBJECT_SIZE;

    // Determine the size of each heap segment
    g_heap.sohSegmentSize = align_size_up(SOH_SEGMENT_ALLOC, SIZE_ALIGNMENT);
    g_heap.minLohSegmentSize = align_size_up(LOH_SEGMENT_ALLOC, SIZE_ALIGNMENT);

    // Create first SOH and LOH segment
    HeapSegment *soh_seg = heap_segment_alloc(g_heap.sohSegmentSize);
    HeapSegment *loh_seg = heap_segment_alloc(g_heap.minLohSegmentSize);
    if (!soh_seg || !loh_seg) {
        // Something went wrong
        return -1;
    }

    // Store the current memory range
    g_heap.lowestAddr = ptr_min(soh_seg, loh_seg);
    g_heap.highestAddr = ptr_max(soh_seg->end, loh_seg->end);

    // Create card table from current memory range
    int res = card_table_init();
    if (res) {
        return res;
    }

    // Init soh generations
    for (int i = max_generation; i >= soh_gen0; i--) {
        generation_make(i, soh_seg);
    }

    // Init loh generation
    generation_make(loh_generation, loh_seg);

    // Init generation static & dynamic data
    {
        // Gen0 max size is between 6MB and 200 MB, and also won't be larger than half of the SOH segment size
        size_t gen0_max_size = size_max(
                6 * 1024 * 1024,
                size_min(
                        align_size_up(g_heap.sohSegmentSize / 2, SIZE_ALIGNMENT),
                        200 * 1024 * 1024
                )
        );
        // Gen0 max size is larger than 6 MB, and also won't be larger than half of the SOH segment size
        size_t gen1_max_size = size_max(
                6 * 1024 * 1024,
                align_size_up(g_heap.sohSegmentSize / 2, SIZE_ALIGNMENT)
        );
        generation_of(soh_gen0)->staticData = (StaticData) {
                .minSize = 256 * 1024,
                .maxSize = gen0_max_size,
        };
        generation_of(soh_gen1)->staticData = (StaticData) {
                .minSize = 160 * 1024,
                .maxSize = gen1_max_size,
        };
        generation_of(soh_gen2)->staticData = (StaticData) {
                .minSize = 256 * 1024,
                .maxSize = SSIZE_MAX,
        };
        generation_of(loh_generation)->staticData = (StaticData) {
                .minSize = 3 * 1024 * 1024,
                .maxSize = SSIZE_MAX,
        };

        // Init dynamic data
        for (int i = total_generation_count - 1; i >= soh_gen0; i--) {
            Generation *gen = generation_of(i);
            // Init alloc budget
            gen->dynamicData.allocBudget = gen->staticData.minSize;
            gen->dynamicData.runningBudget = gen->dynamicData.allocBudget;

            // Init some other fields
            gen->dynamicData.collectionCount = 0;
        }
    }

    // Init alloc locks
    spin_lock_init(&g_heap.moreSpaceLockSoh);
    spin_lock_init(&g_heap.moreSpaceLockLoh);

    // Init GC data
    spin_lock_init(&g_heap.gcLock);

    return 0;
}

//*********************************************************************************************************
// GC related functions
//*********************************************************************************************************

/** Retire all tlabs. */
static void gc_reclaim_tlabs() {
    HeapSegment *ephemeral_seg = youngest_generation->allocationSegment;

    thread_iterate(thread) {
        ThreadAllocContext *tlab = &thread->tlab;
        if (!tlab_allocated(tlab)) {
            continue;
        }

        tlab_retire(tlab, JAVA_TRUE);
    }
}

typedef void (*scan_func)(JAVA_OBJECT *, void *);

/** Scan GC roots */
static void scan_roots(scan_func fn, void *scan_context) {
    assert(fn != NULL);

    // Scan thread stacks
    thread_iterate(thread) {
        // Scan each frame
        stack_frame_iterate(thread, frame) {
            // Scan operand stack
            stack_frame_operand_stack_iterate(frame, slot) {
                if (slot->type == VM_SLOT_OBJECT) {
                    fn(&slot->data.o, scan_context);
                }
            }
            // Scan local slots
            stack_frame_local_iterate(frame, slot) {
                if (slot->type == VM_SLOT_OBJECT) {
                    fn(&slot->data.o, scan_context);
                }
            }
        }
    }
}

/** Promote an object */
static void object_promote(JAVA_OBJECT *obj_p, void *scan_context) {
    JAVA_OBJECT obj = *obj_p;

    // Check if obj is in our gc range
    if ((void *) obj < g_heap.gcContext.addressLow || (void *) obj >= g_heap.gcContext.addressHigh) {
        return;
    }

    // Mark object
    if(obj_is_marked(obj) == JAVA_FALSE) {
        obj_set_marked(obj);

        // TODO: mark object fields
    }
}

/** Mark living objects */
static void heap_mark(GCGeneration gen) {
    printf("Marking roots\n");
    scan_roots(object_promote, NULL);
}

/** Real GC work once the world is stopped */
static void heap_gc(GCGeneration gen) {
    // Retire all TLABs
    gc_reclaim_tlabs();

    // Check alloc budget to see if any older gen needs GC as well
    for (int i = total_generation_count - 1; i > (int) gen; i--) {
        if (generation_of(i)->dynamicData.runningBudget < 0) {
            gen = i;
            if (gen == loh_generation) {
                gen = max_generation;
            }
            printf("Gen %d run out of budget, trigger gen %dd GC instead.\n", i, gen);
            break;
        }
    }

    // Set GC address range
    if (gen == max_generation) {
        // Process the whole heap
        g_heap.gcContext.addressLow = g_heap.lowestAddr;
        g_heap.gcContext.addressHigh = g_heap.highestAddr;
    } else {
        // Process ephemeral segment only
        g_heap.gcContext.addressLow = youngest_generation->allocationSegment->start;
        g_heap.gcContext.addressHigh = youngest_generation->allocationSegment->allocated;
    }

    // TODO: merge card table

    // Mark phase
    heap_mark(gen);

    // Update collect counts
    for (int i = soh_gen0; i <= (int) gen; i++) {
        generation_of(i)->dynamicData.collectionCount++;
        if (i == max_generation) {
            // Also collect LOH
            generation_of(loh_generation)->dynamicData.collectionCount++;
        }
    }
}

/** Trigger a GC of given generation. */
static size_t gc(VM_PARAM_CURRENT_CONTEXT, GCGeneration gen) {
    DynamicData *dd = &generation_of(gen)->dynamicData;
    size_t count_before_lock = dd->collectionCount;

    spin_lock_enter(vmCurrentContext, &g_heap.gcLock);

    // don't trigger another GC if one was already in progress
    // while waiting for the lock
    {
        size_t count_after_lock = dd->collectionCount;
        if (count_before_lock != count_after_lock) {
            spin_lock_exit(&g_heap.gcLock);

            return count_after_lock;
        }
    }

    // Suspend all threads except this one
    thread_stop_the_world(vmCurrentContext);
    thread_wait_until_world_stopped(vmCurrentContext);

    // Do the real GC work
    heap_gc(gen);

    // Resume the world
    thread_resume_the_world(vmCurrentContext);

    spin_lock_exit(&g_heap.gcLock);
    return dd->collectionCount;
}

//*********************************************************************************************************
// Allocator related functions
//*********************************************************************************************************

static JAVA_BOOLEAN generation_alloc_allowed(GCGeneration gen) {
    if (generation_of(gen)->dynamicData.runningBudget < 0) {
        // Run out of budget
        return JAVA_FALSE;
    }
    return JAVA_TRUE;
}

typedef enum {
    a_state_start = 0,
    a_state_check_budget,
    a_state_can_allocate,
    a_state_cant_allocate,
    a_state_try_fit,
    a_state_trigger_gen0_gc,
    a_state_trigger_ephemeral_gc,
    a_state_trigger_full_gc,
} AllocationState;

static const char * const g_allocationStateStr[] = {
        "a_state_start",
        "a_state_check_budget",
        "a_state_can_allocate",
        "a_state_cant_allocate",
        "a_state_try_fit",
        "a_state_trigger_gen0_gc",
        "a_state_trigger_ephemeral_gc",
        "a_state_trigger_full_gc",
};

typedef enum {
    f_can_fit,
    f_too_large,
    f_commit_failed,
} FitResult;

/** Try fit the given size at the end of the segment */
static FitResult heap_segment_try_fit_end(HeapSegment *segment, size_t size, void **out) {
    uint8_t *result = segment->allocated;
    assert(segment->start <= result && result <= segment->committed);

    uint8_t *allocated = ptr_inc(result, size);
    if (allocated <= segment->committed) {
        // Success
        segment->allocated = allocated;
        *out = result;
        return f_can_fit;
    } else if (allocated <= segment->end) {
        // Need to commit more space
        size_t extra_size = ptr_offset(segment->committed, allocated);
        if (heap_segment_grow(segment, extra_size) < extra_size) {
            // Can't commit required size
            return f_commit_failed;
        } else {
            assert(allocated <= segment->committed);
            segment->allocated = allocated;
            *out = result;
            return f_can_fit;
        }
    } else {
        // Can't fit into current segment
        return f_too_large;
    }
}

static FitResult heap_soh_try_fit(size_t size, void **out) {
    return heap_segment_try_fit_end(youngest_generation->allocationSegment, size, out);
}

static JAVA_BOOLEAN heap_alloc_soh(VM_PARAM_CURRENT_CONTEXT, size_t size, void **out) {
    assert(out != NULL);

    Generation *gen0 = youngest_generation;
    AllocationState alloc_state = a_state_start;

    while (1) {
        printf("SOH alloc state: %s\n", g_allocationStateStr[alloc_state]);

        switch (alloc_state) {
            case a_state_start: {
                // Lock the generation
                spin_lock_enter(vmCurrentContext, &g_heap.moreSpaceLockSoh);

                alloc_state = a_state_check_budget;
                break;
            }
            case a_state_check_budget: {
                // Check gen0 budget and determine if a gen0 gc is needed
                if (generation_alloc_allowed(soh_gen0) == JAVA_TRUE) {
                    alloc_state = a_state_try_fit;
                } else {
                    alloc_state = a_state_trigger_gen0_gc;
                }
                break;
            }
            case a_state_try_fit: {
                switch (heap_soh_try_fit(size, out)) {
                    case f_can_fit:
                        // Success
                        alloc_state = a_state_can_allocate;
                        break;
                    case f_too_large:
                        // Can't fit into current segment, need a ephemeral (gen1) gc
                        alloc_state = a_state_trigger_ephemeral_gc;
                        break;
                    case f_commit_failed:
                        // Can't commit required size, do a full gc
                        alloc_state = a_state_trigger_full_gc;
                        break;
                }
                break;
            }
            case a_state_trigger_gen0_gc: {
                gc(vmCurrentContext, soh_gen0);
                alloc_state = a_state_try_fit;
                break;
            }
            case a_state_trigger_ephemeral_gc: {
                alloc_state = a_state_cant_allocate;
                break;
            }
            case a_state_trigger_full_gc: {
                alloc_state = a_state_cant_allocate;
                break;
            }
            case a_state_can_allocate: {
                // Consume alloc budget
                gen0->dynamicData.runningBudget -= size;

                // Release the lock
                spin_lock_exit(&g_heap.moreSpaceLockSoh);

                void *result = *out;
                assert(result != NULL);

                // Zero out memory
                memset(result, 0, size);

                // Mark brick table
                Brick b = brick_of(result);
                // Mark first brick
                brick_set(b, ptr_offset(brick_start_addr_of(b), result) + 1);
                // Fill the rest bricks
                void *end = align_ptr(ptr_inc(result, size), BRICK_SIZE);
                Brick end_brick = brick_of(end);
                for (b++; b <= end_brick; b++) {
                    brick_set(b, BRICK_VAL_PREVIOUS);
                }
                goto exit;
            }
            case a_state_cant_allocate: {
                spin_lock_exit(&g_heap.moreSpaceLockSoh);
                goto exit;
            }
        }
    }

    exit:
    return alloc_state == a_state_can_allocate ? JAVA_TRUE : JAVA_FALSE;
}

/** Allocate given size of memory from given gen. */
static void *heap_alloc_more_space(VM_PARAM_CURRENT_CONTEXT, size_t size, GCGeneration gen) {
    assert(is_size_aligned(size, SIZE_ALIGNMENT));
    assert(gen == soh_gen0 || gen == loh_generation);

    // Give GC a chance to run before we try alloc more space
    thread_checkpoint(vmCurrentContext);

    void *result = NULL;

    if (gen == soh_gen0) {
        heap_alloc_soh(vmCurrentContext, size, &result);
    }

    return result;
}

// Large objects go directly to old gen
#define LARGE_OBJECT_SIZE_MIN ((size_t)(85000))

void *heap_alloc(VM_PARAM_CURRENT_CONTEXT, size_t size) {
    size = align_size_up(size, SIZE_ALIGNMENT);
    assert(size >= MIN_OBJECT_SIZE);

    if (size >= LARGE_OBJECT_SIZE_MIN) {
        // Alloc on LOH
        return heap_alloc_more_space(vmCurrentContext, size, loh_generation);
    } else {
        // Alloc on SOH
        ThreadAllocContext *tlab = &vmCurrentContext->tlab;
        while (1) {
            // Fast path
            {
                // Check if fit in TLAB
                uint8_t *result = tlab->tlabCurrent;
                uint8_t *current = ptr_inc(result, size);
                if (current <= tlab_limit(tlab)) {
                    tlab->tlabCurrent = current;

                    return result;
                }
            }

            // Can't fit in current TLAB, this can because of:
            // 1. TLAB is not allocated
            // 2. TLAB is almost full
            if (tlab_allocated(tlab)) {
                // Retain tlab and allocate object in LOH directly if
                // the amount free in the tlab is too large to discard.
                if (tlab_free(tlab) > tlab->wasteLimit) {
                    // Alloc on SOH directly
                    return heap_alloc_more_space(vmCurrentContext, size, soh_gen0);
                } else {
                    // Discard current tlab
                    tlab_retire(tlab, JAVA_FALSE);
                }
            }

            // Alloc a new tlab
            {
                size_t new_tlab_size = tlab_calculate_new_size(tlab, size);
                void *new_tlab_mem = heap_alloc_more_space(vmCurrentContext, new_tlab_size, soh_gen0);
                if (new_tlab_mem == NULL) {
                    // OOM
                    return NULL;
                }

                // Setup the tlab with new acquired memory
                tlab_fill(tlab, new_tlab_mem, new_tlab_size);
            }
        }
    }

    return NULL;
}
