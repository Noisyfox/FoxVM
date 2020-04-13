//
// Created by noisyfox on 2018/11/17.
//

#include <string.h>
#include <stdio.h>
#include "vm_gc_old.h"
#include "vm_memory.h"
#include "vm_thread.h"

/*
 * Definition of cardtable bytemap:
 *
 *    7     6     5     4     3     2     1      0
 * +-----+-----+-----+-----+-----+-----+-----+-------+
 * |   state   |         variable data               |
 * +-----+-----+-----+-----+-----+-----+-----+-------+
 * |  0     0  |             free_after              |  free (not committed)
 * +-----+-----+-----+-----+-----+-----+-----+-------+
 * |  0     1  |           committed_after           |  committed
 * +-----+-----+-----+-----+-----+-----+-----+-------+
 * |  1     0  |          reserved           | dirty |  allocated
 * +-----+-----+-----+-----+-----+-----+-----+-------+
 * |  1     1  |                                     |  reserved
 * +-----+-----+-----+-----+-----+-----+-----+-------+
 *
 * state: the allocation status of this chunk.
 *   - free: the memory chunk is not committed, only reserved from OS.
 *   - committed: the memory is committed from OS, ready for allocation.
 *   - allocated: the corresponding memory chunk is already allocated to a managed object.
 *
 * free_after: the number of free chunks after current chunk. This field
 *   has only 6 bits with a maximum value of 0x3F (63). If there are more
 *   than 63 free chunks afterwards, then this value will be 63, and the
 *   flag value at [current + 64] will be added to get the total number.
 *
 * committed_after: similar as free_after, but for number of committed chunks.
 *
 * dirty: the object stored in the corresponding chunk contains reference pointing
 *   to an object located in young generation heap.
 *
 */
#define CARDTABLE_STATE_MASK ((uint8_t)0xC0)
#define CARDTABLE_STATE_SHIFT 6
#define CARDTABLE_STATE_FREE 0
#define CARDTABLE_STATE_COMMITTED 1
#define CARDTABLE_STATE_ALLOCATED 2

#define CARDTABLE_FREE_AFTER_MASK ((uint8_t)0x3F)
#define CARDTABLE_FREE_AFTER_SHIFT 0

#define CARDTABLE_COMMITTED_AFTER_MASK ((uint8_t)0x3F)
#define CARDTABLE_COMMITTED_AFTER_SHIFT 0

#define CARDTABLE_FLAG_DIRTY ((uint8_t)0x01) // Has cross generation reference

// Object need to be moved to old gen after 5 times of young gen gc
#define GC_SURVIVOR_AGE_MAX 5

typedef enum {
    gc_type_failed = -1,
    gc_type_nop = 0,
    gc_type_minor = 1,
    gc_type_major = 2
} GCType;

static GCType gc_trigger(VM_PARAM_CURRENT_CONTEXT, GCType type);

static JAVA_VOID gc_thread_entrance(VM_PARAM_CURRENT_CONTEXT);

static JAVA_VOID gc_thread_terminated(VM_PARAM_CURRENT_CONTEXT);

// Header for the young gen
typedef struct {
    VMSpinLock sync; // Global lock for accessing the young gen

    void *edenStart;
    void *reservedEnd;
    void *largeObjIndex;
    void *tlabIndex;
    void *edenEnd;
    void *survivor0Limit;
    void *survivorMiddle;
    void *survivor1Limit;
    void *youngEnd;
} JavaHeapYoungGen;

typedef struct {
    VMSpinLock sync; // Global lock for accessing the old gen

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

    VMSpinLock gcSync;
    GCType gcRequired;
    GCType gcLast;
    uint64_t gcCount;
    JAVA_BOOLEAN gcThreadRunning;
    JavaObjectBase gcThreadObject; // A dummy object for gc thread.
    VMThreadContext gcThread;

    JavaHeapYoungGen *youngGenHeader;
    JavaHeapOldGen *oldGenHeader;
} JavaHeap;

size_t young_gen_expand(JavaHeapYoungGen *youngGen, size_t increase) {
    if (increase == 0) {
        return 0;
    }

    increase = align_size_up(increase, mem_page_size());

    void *current_reserved_end = youngGen->reservedEnd;
    void *new_reserved_end = ptr_dec(current_reserved_end, increase);

    if (new_reserved_end > current_reserved_end) {
        // Overflow?
        return 0;
    }
    if (new_reserved_end < youngGen->edenStart) {
        increase = (size_t) (((uintptr_t) current_reserved_end) - ((uintptr_t) youngGen->edenStart));
        new_reserved_end = youngGen->edenStart;
    }

    if (mem_commit(new_reserved_end, increase) != JAVA_TRUE) {
        return 0;
    }

    youngGen->reservedEnd = new_reserved_end;
    return increase;
}

static JavaHeap *g_heap = NULL;

/**
 * Header region not included.
 */
static inline JAVA_BOOLEAN ptr_in_young_gen(void *ptr) {
    JavaHeapYoungGen *youngGen = g_heap->youngGenHeader;
    if (ptr < youngGen->edenStart) {
        return JAVA_FALSE;
    }

    if (ptr < youngGen->youngEnd) {
        return JAVA_TRUE;
    }

    return JAVA_FALSE;
}

static inline JAVA_BOOLEAN ptr_in_old_gen(void *ptr) {
    JavaHeapOldGen *oldGen = g_heap->oldGenHeader;
    if (ptr < oldGen->dataStart) {
        return JAVA_FALSE;
    }

    if (ptr < ptr_inc(g_heap, g_heap->heapSize)) {
        return JAVA_TRUE;
    }

    return JAVA_FALSE;
}

static inline JAVA_BOOLEAN ptr_in_heap(void *ptr) {
    return ptr_in_young_gen(ptr) || ptr_in_old_gen(ptr);
}

/**
 * Get the corresponding byte for the given ptr in old gen bytemap.
 * @param ptr
 * @return A pointer to the flag byte, or NULL if ptr not in old gen
 */
static inline uint8_t *ptr_old_gen_bytemap_slot(void *ptr) {
    JavaHeapOldGen *oldGen = g_heap->oldGenHeader;
    if (ptr < oldGen->dataStart || ptr >= ptr_inc(g_heap, g_heap->heapSize)) {
        // Ptr not in old gen
        return NULL;
    }

    uint64_t offset = ptr_offset(oldGen->dataStart, ptr);
    uint64_t slotNum = offset / oldGen->chunkSize;

    return &oldGen->byteMap[slotNum];
}

static inline JAVA_BOOLEAN ptr_olg_gen_is_clean(void *ptr, JAVA_BOOLEAN young_only) {
    if (young_only) {
        uint8_t *bytemap_slot = ptr_old_gen_bytemap_slot(ptr);
        if (bytemap_slot != NULL) {
            // Check if it's marked as having ref to young gen
            uint8_t mark = *bytemap_slot;
            if ((mark & CARDTABLE_FLAG_DIRTY) != CARDTABLE_FLAG_DIRTY) {
                // Doesn't have reference to young gen
                return JAVA_TRUE;
            }
        }
    }

    return JAVA_FALSE;
}

int heap_init(VM_PARAM_CURRENT_CONTEXT, HeapConfig *config) {
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
    uint64_t eden_initial_size = eden_size / 8;

    uint64_t old_gen_size = heap_data_size - young_gen_size;
    uint64_t old_gen_header_size = align_size_up(sizeof(JavaHeapOldGen), mem_alloc_granularity());
    uint64_t old_gen_chunk_size = align_size_up(OLD_GEN_CHUNK_SIZE_MIN, mem_alloc_granularity());
    // Calculate required bytemap size
    uint64_t old_gen_bytemap_data_size = old_gen_size - old_gen_header_size;
    uint64_t old_gen_remain = old_gen_bytemap_data_size % old_gen_chunk_size;
    uint64_t old_gen_chunk_count = old_gen_bytemap_data_size / old_gen_chunk_size;
    if (old_gen_remain < old_gen_chunk_count) {
        // calculate L so that remain + chunk_size * L >= chunk_count - L
        // so that L >= (chunk_count - count_remain) / (chunk_size + 1)
        uint64_t d = old_gen_chunk_count - old_gen_remain;
        uint64_t c_1 = old_gen_chunk_size + 1;
        uint64_t l = d / c_1;
        if (l * c_1 < d) {
            l++;
        }

        if (old_gen_chunk_count <= l) {
            return -1;
        }
        old_gen_remain += l * old_gen_chunk_size;
        old_gen_chunk_count -= l;
    }
    if (is_size_aligned_up(old_gen_remain, mem_alloc_granularity()) != JAVA_TRUE) {
        // Make sure the remaining is aligned
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
    // Init gc thread
    spin_lock_init(&heap->gcSync);
    if (spin_lock_try_enter(&heap->gcSync) != JAVA_TRUE) {
        mem_release(heap_mem, max_heap_size);
        return -1;
    }
    heap->gcCount = 0;
    heap->gcRequired = gc_type_nop;
    heap->gcLast = gc_type_nop;
    heap->gcThreadRunning = JAVA_FALSE;
    heap->gcThread.entrance = gc_thread_entrance;
    heap->gcThread.terminated = gc_thread_terminated;
    if (monitor_create(vmCurrentContext, &heap->gcThreadObject) != thrd_success) {
        mem_release(heap_mem, max_heap_size);
        return -1;
    }
    heap->gcThread.currentThread = &heap->gcThreadObject;

    JavaHeapYoungGen *young_gen = ptr_inc(heap_mem, heap_header_size);
    heap->youngGenHeader = young_gen;
    // Init young gen header
    spin_lock_init(&young_gen->sync);
    young_gen->edenStart = ptr_inc(young_gen, young_gen_header_size);
    young_gen->edenEnd = ptr_inc(young_gen->edenStart, eden_size);
    young_gen->reservedEnd = young_gen->edenEnd;
    young_gen->tlabIndex = young_gen->edenEnd;
    young_gen->survivor0Limit = young_gen->edenEnd;
    young_gen->survivorMiddle = ptr_inc(young_gen->edenEnd, survivor_size);
    young_gen->survivor1Limit = young_gen->survivorMiddle;
    young_gen->youngEnd = ptr_inc(young_gen->survivorMiddle, survivor_size);
    // Pre-alloc eden initial space
    if (young_gen_expand(young_gen, eden_initial_size) < eden_initial_size) {
        mem_release(heap_mem, max_heap_size);
        return -1;
    }
    young_gen->largeObjIndex = young_gen->reservedEnd;

    // Double check
    if (ptr_inc(young_gen, young_gen_size) != young_gen->youngEnd) {
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
    spin_lock_init(&old_gen->sync);
    old_gen->chunkSize = old_gen_chunk_size;
    old_gen->chunkCount = old_gen_chunk_count;
    old_gen->byteMap = ptr_inc(old_gen, old_gen_header_size);
    old_gen->dataStart = ptr_inc(old_gen->byteMap, old_gen_remain);

    // Double check
    if (ptr_inc(old_gen->dataStart, old_gen->chunkSize * old_gen->chunkCount) !=
        ptr_inc(heap_mem, max_heap_size)) {
        mem_release(heap_mem, max_heap_size);
        return -1;
    }

    g_heap = heap;
    return 0;
}

//*********************************************************************************************************
// Allocator related functions
//*********************************************************************************************************

/**
 * Get a new TLAB buffer.
 * @param __vmCurrentThreadContext
 * @return true for success, false otherwise.
 */
static JAVA_BOOLEAN alloc_tlab(VM_PARAM_CURRENT_CONTEXT) {
    // Give GC a chance to run before we try alloc more space
    thread_checkpoint(vmCurrentContext);

    JavaHeapYoungGen *youngGen = g_heap->youngGenHeader;

    // Acquire the young gen lock
    spin_lock_enter(vmCurrentContext, &youngGen->sync);

    while (1) {
        void *limit = youngGen->tlabIndex;
        void *head = ptr_dec(limit, g_heap->tlabSize);
        if (head < youngGen->largeObjIndex) {
            // Eden full, need trigger a minor GC
            GCType actual_gc;
            do {
                actual_gc = gc_trigger(vmCurrentContext, gc_type_minor);
            } while (actual_gc < gc_type_minor && actual_gc >= gc_type_nop);

            if (actual_gc == gc_type_failed) {
                // GC failed due to OOM.
                spin_lock_exit(&youngGen->sync);
                return JAVA_FALSE;
            }

            // Now retry tlab alloc
        } else {
            youngGen->tlabIndex = head;
            ThreadAllocContext *tlab = &vmCurrentContext->tlab;
            tlab->tlabHead = head;
            tlab->tlabCurrent = head;
            tlab->tlabLimit = limit;

            // Release lock
            spin_lock_exit(&youngGen->sync);
            return JAVA_TRUE;
        }
    }
}

/**
 * @param start
 * @return usable address after header
 */
static void *init_heap_header(void *start) {
    ObjectHeapHeader *h = start;
    h->flag = HEAP_FLAG_NORMAL;

    return ptr_inc(start, sizeof(ObjectHeapHeader));
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
                if (r != JAVA_TRUE) {
                    return NULL;
                }
            }
        }
    } else if (size > g_heap->largeObjectSize) {
        // Alloc on old gen

    } else {
        // Alloc in yong gen, outside TLAB
        JavaHeapYoungGen *youngGen = g_heap->youngGenHeader;

        // Acquire the young gen lock
        spin_lock_enter(vmCurrentContext, &youngGen->sync);

        while (1) {
            uint8_t *result = youngGen->largeObjIndex;
            uint8_t *advance = result + size;

            if ((void *) advance <= youngGen->tlabIndex) {
                youngGen->largeObjIndex = advance;

                // Release lock
                spin_lock_exit(&youngGen->sync);
                return init_heap_header(result);
            } else {
                // Eden full, need trigger a minor GC
                GCType actual_gc;
                do {
                    actual_gc = gc_trigger(vmCurrentContext, gc_type_minor);
                } while (actual_gc < gc_type_minor && actual_gc >= gc_type_nop);

                if (actual_gc == gc_type_failed) {
                    // GC failed due to OOM.
                    spin_lock_exit(&youngGen->sync);
                    return NULL;
                }

                // Now retry alloc
            }
        }
    }
}

//*********************************************************************************************************
// GC related functions
//*********************************************************************************************************

/**
 * Trigger a GC with the given type.
 *
 * @return The actual gc type that performed. Can be different to the required type.
 */
static GCType gc_trigger(VM_PARAM_CURRENT_CONTEXT, GCType type) {
    if (type < gc_type_minor) {
        return gc_type_nop;
    }

    // Wake up the gc thread
    monitor_enter(vmCurrentContext, &g_heap->gcThreadObject);
    {
        if (g_heap->gcRequired < type) {
            g_heap->gcRequired = type;
        }
        monitor_notify_all(vmCurrentContext, &g_heap->gcThreadObject);

        monitor_exit(vmCurrentContext, &g_heap->gcThreadObject);
    }

    /*
     * GC thread will hold the gc lock until all other threads are asked to suspend.
     * So if we could touch the gc lock, then we are guaranteed to be blocked when
     * passing a checkpoint.
     */
    spin_lock_touch_unsafe(&g_heap->gcSync);
    thread_checkpoint(vmCurrentContext);

    // The gc thread finished working and resumed the world.
    // Get the type of gc that performed during this period.
    return g_heap->gcLast;
}

#define obj_marked(obj, flag) obj_test_flags_and((obj), (flag))

static JAVA_VOID mark_object(JAVA_OBJECT obj, uintptr_t gc_flag, JAVA_BOOLEAN young_only);

static JAVA_VOID mark_class(JAVA_CLASS clazz, uintptr_t gc_flag, JAVA_BOOLEAN young_only);

static inline JAVA_VOID mark_dirty_object(JAVA_OBJECT obj, uintptr_t gc_flag, JAVA_BOOLEAN young_only) {
    if (ptr_olg_gen_is_clean(obj, young_only) != JAVA_TRUE) {
        mark_object(obj, gc_flag, young_only);
    }
}

static inline JAVA_VOID mark_dirty_class(JAVA_CLASS clazz, uintptr_t gc_flag, JAVA_BOOLEAN young_only) {
    if (ptr_olg_gen_is_clean(clazz, young_only) != JAVA_TRUE) {
        mark_class(clazz, gc_flag, young_only);
    }
}

static inline uintptr_t obj_get_gc_age(JAVA_OBJECT obj) {
    return (obj_get_flags(obj) & OBJECT_FLAG_AGE_MASK) >> OBJECT_FLAG_AGE_SHIFT;
}

static inline JAVA_VOID do_mark_obj(JAVA_OBJECT obj, uintptr_t gc_flag) {
    // Mark obj
    obj_reset_flags_masked(obj, OBJECT_FLAG_GC_MARK_MASK, gc_flag);

    // Increase living time if it's in young gen
    if (ptr_in_young_gen(obj) == JAVA_TRUE) {
        uintptr_t age = obj_get_gc_age(obj);

        // Increase age by 1
        age++;

        // Save age back
        uintptr_t age_flags = age << OBJECT_FLAG_AGE_SHIFT;
        obj_reset_flags_masked(obj, OBJECT_FLAG_GC_MARK_MASK, age_flags);
    }
}

static JAVA_VOID mark_object(JAVA_OBJECT obj, uintptr_t gc_flag, JAVA_BOOLEAN young_only) {
    if (obj == JAVA_NULL) {
        return;
    }

    // Class instance need to be handled separately
    JAVA_CLASS currentClass = obj_get_class(obj);
    if (currentClass == (JAVA_CLASS) JAVA_NULL) {
        mark_class((JAVA_CLASS) obj, gc_flag, young_only);
        return;
    }

    // Already marked
    if (obj_marked(obj, gc_flag) == JAVA_TRUE) {
        return;
    }

    // Mark current obj
    do_mark_obj(obj, gc_flag);

    // Mark class instance first
    mark_dirty_class(currentClass, gc_flag, young_only);

    // Then walk pass all instance fields and mark reference fields
    while (currentClass != (JAVA_CLASS) JAVA_NULL) {
        FieldTable *fieldTable = currentClass->fieldTable;

        for (int i = 0; i < fieldTable->fieldCount; i++) {
            FieldDesc *desc = &fieldTable->fields[i];
            if (desc->isReference != JAVA_TRUE) {
                // Ignore non-reference type fields
                continue;
            }

            if (field_is_static(desc) == JAVA_TRUE) {
                // Ignore static fields since it's processed when marking Class
                continue;
            }

            JAVA_OBJECT referencedObj = *((JAVA_OBJECT *) (ptr_inc(obj, desc->offset)));
            mark_dirty_object(referencedObj, gc_flag, young_only);
        }

        // Then mark parent class fields
        currentClass = currentClass->parentClass;
    }
}

static JAVA_VOID mark_class(JAVA_CLASS clazz, uintptr_t gc_flag, JAVA_BOOLEAN young_only) {
    if (clazz == (JAVA_CLASS) JAVA_NULL) {
        return;
    }

    // Already marked
    if (obj_marked((JAVA_OBJECT) clazz, gc_flag) == JAVA_TRUE) {
        return;
    }

    // Mark current class
    do_mark_obj((JAVA_OBJECT) clazz, gc_flag);

    // Mark classloader first
    mark_dirty_object(clazz->classLoader, gc_flag, young_only);

    // Then walk pass all static fields and mark reference fields
    FieldTable *fieldTable = clazz->fieldTable;
    for (int i = 0; i < fieldTable->fieldCount; i++) {
        FieldDesc *desc = &fieldTable->fields[i];
        if (desc->isReference != JAVA_TRUE) {
            // Ignore non-reference type fields
            continue;
        }

        if (field_is_static(desc) != JAVA_TRUE) {
            // Ignore non-static fields
            continue;
        }

        JAVA_OBJECT referencedObj = *((JAVA_OBJECT *) (ptr_inc(clazz, desc->offset)));
        mark_dirty_object(referencedObj, gc_flag, young_only);
    }

    // Mark parent classes
    mark_dirty_class(clazz->parentClass, gc_flag, young_only);
    for (int i = 0; i < clazz->interfaceCount; i++) {
        mark_dirty_class(clazz->parentInterfaces[i], gc_flag, young_only);
    }
}

static inline uint64_t bytemap_next_allocated(uint64_t cursor) {
    JavaHeapOldGen *oldGen = g_heap->oldGenHeader;
    uint64_t chunkCount = oldGen->chunkCount;
    uint8_t *byteMap = oldGen->byteMap;

    while (cursor < chunkCount) {
        uint8_t flag = byteMap[cursor];

        uint8_t state = (flag & CARDTABLE_STATE_MASK) >> CARDTABLE_STATE_SHIFT;
        switch (state) {
            case CARDTABLE_STATE_FREE:
                cursor += ((flag & CARDTABLE_FREE_AFTER_MASK) >> CARDTABLE_FREE_AFTER_SHIFT);
                break;
            case CARDTABLE_STATE_COMMITTED:
                cursor += ((flag & CARDTABLE_COMMITTED_AFTER_MASK) >> CARDTABLE_COMMITTED_AFTER_SHIFT);
                break;
            case CARDTABLE_STATE_ALLOCATED:
                return cursor;
            default:
                break;
        }
        cursor++;
    }

    return cursor;
}

static inline uint64_t bytemap_next_dirty(uint64_t cursor) {
    JavaHeapOldGen *oldGen = g_heap->oldGenHeader;
    uint64_t chunkCount = oldGen->chunkCount;
    uint8_t *byteMap = oldGen->byteMap;

    while ((cursor = bytemap_next_allocated(cursor)) < chunkCount) {
        uint8_t flag = byteMap[cursor];
        if ((flag & CARDTABLE_FLAG_DIRTY) == CARDTABLE_FLAG_DIRTY) {
            return cursor;
        }
        cursor++;
    }

    return cursor;
}

static inline size_t obj_get_size(JAVA_OBJECT obj) {
    JAVA_CLASS clazz = obj_get_class(obj);
    if (clazz == (JAVA_CLASS) JAVA_NULL) {
        return ((JAVA_CLASS) obj)->classSize;
    } else {
        return clazz->instanceSize;
    }
}

// Mark old gen objects with card table marked dirty
static JAVA_VOID mark_old_gen_dirty_objects(uintptr_t gc_flag) {
    JavaHeapOldGen *oldGen = g_heap->oldGenHeader;
    uint64_t chunkSize = oldGen->chunkSize;
    uint64_t chunkCount = oldGen->chunkCount;
    void *dataStart = oldGen->dataStart;
    uint64_t chunk = 0;
    while ((chunk = bytemap_next_dirty(chunk)) < chunkCount) {
        JAVA_OBJECT obj = ptr_inc(dataStart, chunk * chunkSize + sizeof(ObjectHeapHeader));

        // Mark dirty object
        mark_object(obj, gc_flag, JAVA_TRUE);

        // Chunk number for this object
        size_t object_size = obj_get_size(obj);
        uint64_t chunkNumber = (object_size + sizeof(ObjectHeapHeader) - 1) / chunkSize + 1;

        chunk += chunkNumber;
    }
}

/**
 * Calculate the total size of survived objects with different ages in young gen.
 * The object size includes the size of the object itself and it's heap header.
 *
 * @param start the start address of young gen region to be scanned, inclusive
 * @param end the end of address of young gen region to be scanned, exclusive.
 * @param gc_flag
 * @param size_out an array of total size of survived objects with different ages. The index
 *      of this array is the age of the object - 1.
 */
static JAVA_VOID gc_scan_young_gen_size(void *start, void *end, uintptr_t gc_flag, uint64_t *size_out) {
    ObjectHeapHeader *objectHeader = start;
    void *objectCursor;

    while ((objectCursor = ptr_inc(objectHeader, sizeof(ObjectHeapHeader))) <= end
           && objectHeader->flag != HEAP_FLAG_EMPTY) {

        if (objectHeader->flag == HEAP_FLAG_FORWARD) {
            ForwardPointer *fp = objectCursor;
            // Skip remaining data
            objectHeader = ptr_inc(fp, sizeof(ForwardPointer) + fp->remainingSize);
        } else if (objectHeader->flag == HEAP_FLAG_NORMAL) {
            JAVA_OBJECT obj = objectCursor;
            size_t object_size = obj_get_size(obj);

            if (obj_marked(obj, gc_flag)) {
                uintptr_t age = obj_get_gc_age(obj);

                // Add size. Survived object will have an age of at least 1.
                size_out[age - 1] += object_size + sizeof(ObjectHeapHeader);
            }

            objectHeader = ptr_inc(obj, object_size);
        }
    }
}

static JAVA_VOID gc_minor(uintptr_t gc_flag) {
    VMThreadContext *thread = NULL;

    while ((thread = thread_managed_next(thread)) != NULL) {
        // Mark the thread object first
        mark_object(thread->currentThread, gc_flag, JAVA_TRUE);

        // TODO: Then mark the thread stack

    }

    // Mark old gen objects with card table marked dirty
    mark_old_gen_dirty_objects(gc_flag);

    // Calculate size of all survived objects in young gen
    uint64_t survObjSizeInEden[GC_SURVIVOR_AGE_MAX] = {};
    uint64_t survObjSizeInSurvivor[GC_SURVIVOR_AGE_MAX] = {};
    memset(survObjSizeInEden, 0, sizeof(survObjSizeInEden));
    // Scan Eden region
    JavaHeapYoungGen *youngGen = g_heap->youngGenHeader;
    // First, the large object region
    gc_scan_young_gen_size(youngGen->reservedEnd, youngGen->largeObjIndex, gc_flag, survObjSizeInEden);
    // Then scan tlabs
    void *tlabStart = youngGen->tlabIndex;
    void *tlabEnd = ptr_inc(tlabStart, g_heap->tlabSize);
    while (tlabEnd <= youngGen->edenEnd) {
        gc_scan_young_gen_size(tlabStart, tlabEnd, gc_flag, survObjSizeInEden);

        tlabStart = tlabEnd;
        tlabEnd = ptr_inc(tlabStart, g_heap->tlabSize);
    }
    // Then scan active survivor region
    if (gc_flag == OBJECT_FLAG_GC_MARK_0) {
        gc_scan_young_gen_size(youngGen->edenEnd, youngGen->survivor0Limit, gc_flag, survObjSizeInSurvivor);
    } else {
        gc_scan_young_gen_size(youngGen->survivorMiddle, youngGen->survivor1Limit, gc_flag, survObjSizeInSurvivor);
    }

    // Determine object age high water mark

    // TODO: Move object older than high water mark from young to old gen
}

static JAVA_VOID gc_thread_entrance(VM_PARAM_CURRENT_CONTEXT) {
    printf("gc_thread_entrance\n");

    while (1) {
        GCType targetGC;
        monitor_enter(vmCurrentContext, vmCurrentContext->currentThread);
        {
            while (1) {
                // Check state
                if (g_heap->gcThreadRunning) {
                    targetGC = g_heap->gcRequired;
                } else {
                    targetGC = gc_type_failed;
                }
                g_heap->gcRequired = gc_type_nop;

                if (targetGC == gc_type_nop) {
                    monitor_wait(vmCurrentContext, vmCurrentContext->currentThread, 0, 0);
                } else {
                    break;
                }
            }

            g_heap->gcLast = targetGC;

            monitor_exit(vmCurrentContext, vmCurrentContext->currentThread);
        }

        if (targetGC == gc_type_failed) {
            break;
        }

        // Ask all thread to suspend
        thread_stop_the_world(vmCurrentContext);
        // Release gc sync
        spin_lock_exit(&g_heap->gcSync);
        // Wait all thread to suspend
        thread_wait_until_world_stopped(vmCurrentContext);

        // World stopped, now the gc can start
        // Increase the gc count
        uint64_t gc_count = ++g_heap->gcCount;
        // Pick the gc mark flag based on gc count
        uintptr_t gc_mark_flag = (gc_count & 1) == 0 ? OBJECT_FLAG_GC_MARK_0 : OBJECT_FLAG_GC_MARK_1;

        // Do actual gc
        switch (targetGC) {
            case gc_type_minor:
                gc_minor(gc_mark_flag);
                break;
            case gc_type_major:
                break;
            default:
                g_heap->gcLast = gc_type_nop;
                break;
        }

        // Re-lock the gc sync
        if (spin_lock_try_enter(&g_heap->gcSync) != JAVA_TRUE) {
            // ?????????????????????? WHY??????
            fprintf(stderr, "Re-enter the gc sync failed!");
            exit(-1);
        }

        // Resume the world
        thread_resume_the_world(vmCurrentContext);
    }

}

static JAVA_VOID gc_thread_terminated(VM_PARAM_CURRENT_CONTEXT) {
    // GC thread exit.
    printf("gc_thread_terminated\n");

    if (g_heap->gcThreadRunning) {
        // Restart the gc thread.
        thread_native_free(vmCurrentContext);
        thread_native_init(vmCurrentContext);
        thread_start(vmCurrentContext);
    } else {
        // Release the gc sync
        spin_lock_exit(&g_heap->gcSync);

        monitor_enter(vmCurrentContext, vmCurrentContext->currentThread);
        {
            monitor_notify_all(vmCurrentContext, vmCurrentContext->currentThread);

            monitor_exit(vmCurrentContext, vmCurrentContext->currentThread);
        }
    }
}

int gc_thread_start(VM_PARAM_CURRENT_CONTEXT) {
    int result;

    monitor_enter(vmCurrentContext, &g_heap->gcThreadObject);
    {
        if (g_heap->gcThreadRunning == JAVA_TRUE) {
            result = thrd_success;
        } else {
            thread_native_init(&g_heap->gcThread);
            tlab_reset(&g_heap->gcThread.tlab);
            g_heap->gcThreadRunning = JAVA_TRUE;

            result = thread_start(&g_heap->gcThread);
        }

        monitor_exit(vmCurrentContext, &g_heap->gcThreadObject);
    }

    return result;
}

int gc_thread_shutdown(VM_PARAM_CURRENT_CONTEXT) {
    monitor_enter(vmCurrentContext, &g_heap->gcThreadObject);
    {
        g_heap->gcThreadRunning = JAVA_FALSE;
        monitor_notify_all(vmCurrentContext, &g_heap->gcThreadObject);

        while (thread_get_state(&g_heap->gcThread) != thrd_stat_terminated) {
            monitor_wait(vmCurrentContext, &g_heap->gcThreadObject, 1000, 0);
        }
        thread_native_free(&g_heap->gcThread);

        monitor_exit(vmCurrentContext, &g_heap->gcThreadObject);
    }

    return thrd_success;
}
