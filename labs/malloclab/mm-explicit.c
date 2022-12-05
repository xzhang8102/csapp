#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memlib.h"
#include "mm.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

#ifdef DEBUG
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_requires(...) assert(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#define dbg_ensures(...) assert(__VA_ARGS__)
#else
/* When debugging is disnabled, no code gets generated for these */
#define dbg_printf(...)
#define dbg_requires(...)
#define dbg_assert(...)
#define dbg_ensures(...)
#endif

// Basic Constants
typedef uint64_t word_t;
static const size_t WSIZE = sizeof(word_t);
// mainly for alignment
// static const size_t DSIZE = 2 * WSIZE;
// MIN_BLOCK_SIZE: header (8B), prev (8B), next (8B), footer (8B)
static const size_t MIN_BLOCK_SIZE = 4 * WSIZE;
// NOTE: customizable
// when current heap is not enough, ask for CHUNKSIZE bytes
static const size_t CHUNKSIZE = (1 << 12);

static const word_t ALLOC_MASK = 0x1;
static const word_t SIZE_MASK = ~(word_t)0x7;

typedef struct block
{
    /* Don't forget primitive alignment for structure
     * Header contains size + allocation flag */
    word_t header;
    struct block *prev_blk;
    struct block *next_blk;
    /*
     * We can't declare the footer as part of the struct, since its starting
     * position is unknown
     */
} block_t;

// Global Variables
static block_t *heap_head = NULL;

// Function Declaration
static block_t *extend_heap(size_t size);
static block_t *coalesce(block_t *blk);
static block_t *first_fit(size_t size);
static void place(block_t *blk, size_t size);
static void link_blk(block_t *prev, block_t *new_blk);
static block_t *remove_blk(block_t *blk);

/* utils */
static word_t pack(size_t size, bool alloc);
static size_t get_size(block_t *block);
static size_t calc_real_size(size_t size);
static bool get_alloc(block_t *block);
static size_t extract_size(word_t word);
static block_t *payload_to_block(void *bp);
static void *block_to_payload(block_t *block);
static void write_header(block_t *block, size_t size, bool alloc);
static void write_footer(block_t *block, size_t size, bool alloc);
static block_t *find_next_blk(block_t *blk);
static block_t *find_prev_blk(block_t *blk);

/*
 * init heap with a block with no payload
 */
int mm_init(void)
{
    /*
     * ******** heap start *********
     * | prologue header (8 bytes) |   heap_head <─┐
     * | pointer  prev   (8 bytes) |               │
     * | pointer  next   (8 bytes) |   ──┐         │
     * | prologue footer (8 bytes) |     │         │
     * | epilogue header (8 bytes) |   <─┘         │
     * | pointer  prev   (8 bytes) |   ────────────┘
     * | pointer  next   (8 bytes) |
     * ******** heap end   *********
     */
    heap_head = (block_t *)mem_sbrk(7 * WSIZE);
    if (heap_head == NULL)
        return -1;
    write_header(heap_head, MIN_BLOCK_SIZE, true);
    write_footer(heap_head, MIN_BLOCK_SIZE, true);
    heap_head->prev_blk = NULL;
    block_t *epilogue_header = find_next_blk(heap_head);
    write_header(epilogue_header, 0, true);
    epilogue_header->prev_blk = heap_head;
    epilogue_header->next_blk = NULL;
    heap_head->next_blk = epilogue_header;

    block_t *new_blk;
    if ((new_blk = extend_heap(CHUNKSIZE)) == NULL)
        return -1;
    return 0;
}

void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size_t real_size = calc_real_size(size);

    block_t *bp;
    if ((bp = first_fit(real_size)) != NULL)
    {
        // now bp points to a free block
        dbg_ensures(!get_alloc(bp));
        place(bp, real_size);
        dbg_ensures(get_alloc(bp));
        return block_to_payload(bp);
    }
    // request more heap
    if ((bp = extend_heap(real_size)) == NULL)
        return NULL;
    place(bp, real_size);
    return block_to_payload(bp);
}

void mm_free(void *payload)
{
    if (payload == NULL)
        return;
    block_t *blk = payload_to_block(payload);
    size_t size = get_size(blk);
    write_header(blk, size, false);
    write_footer(blk, size, false);
    coalesce(blk);
}

void *mm_realloc(void *old_payload, size_t size)
{
    if (old_payload == NULL)
        return mm_malloc(size);
    if (size == 0)
    {
        free(old_payload);
        return NULL;
    }
    block_t *blk = payload_to_block(old_payload);
    if (!get_alloc(blk))
        return NULL;

    // blk is an allocated block
    size_t real_size = calc_real_size(size);
    size_t old_size = get_size(blk);

    // NOTE: data should keep the same within min(old_size, real_size)
    if (old_size >= real_size)
    {
        place(blk, real_size);
        return old_payload;
    }
    else
    {
        block_t *prev = find_prev_blk(blk);
        block_t *next = find_next_blk(blk);
        bool prev_alloc = get_alloc(prev);
        bool next_alloc = get_alloc(next);
        size_t prev_size = get_size(prev);
        size_t next_size = get_size(next);
        size_t test_size = old_size;
        if (!prev_alloc)
            test_size += prev_size;
        if (!next_alloc)
            test_size += next_size;

        if (test_size >= real_size) // coalescing can solve the problem
        {
            // coalesce next
            if (prev_alloc && !next_alloc)
            {
                next = remove_blk(next);
                write_header(blk, test_size, true);
                write_footer(blk, test_size, true);
                place(blk, real_size);
                return old_payload;
            }
            // coalesce prev and copy data
            else if (!prev_alloc)
            {
                prev = remove_blk(prev);
                if (!next_alloc)
                    next = remove_blk(next);
                blk = prev;
                write_header(blk, test_size, true);
                write_footer(blk, test_size, true);
                memcpy(block_to_payload(blk), old_payload,
                       old_size - 2 * WSIZE);
                place(blk, real_size);
                return block_to_payload(blk);
            }
            else
                return NULL;
        }
        else
        {
            block_t *new_blk;
            if ((new_blk = mm_malloc(real_size)) == NULL)
                return NULL;
            memcpy(block_to_payload(new_blk), old_payload,
                   old_size - 2 * WSIZE);
            mm_free(old_payload);
            return block_to_payload(new_blk);
        }
    }
}

void *mm_calloc(size_t nmemb, size_t size)
{
    size_t bytes = nmemb * size;
    void *payload = mm_malloc(bytes);
    if (payload != NULL)
        memset(payload, 0, bytes);
    return payload;
}

void mm_checkheap(int verbose)
{
    (void)verbose;
    if (heap_head == NULL)
    {
        printf("heap_head is NULL.\n");
        return;
    }
    // check prologue
    dbg_assert(get_size(heap_head) == MIN_BLOCK_SIZE);
    dbg_assert(heap_head->prev_blk == NULL);
    word_t *prologue_footer = (word_t *)(((char *)heap_head) + 3 * WSIZE);
    dbg_assert(extract_size(*prologue_footer) == MIN_BLOCK_SIZE);

    int list_cnt = 0;
    block_t *prev, *curr;
    for (prev = heap_head, curr = heap_head->next_blk; curr != NULL;
         prev = curr, curr = curr->next_blk)
    {
        list_cnt++;
        dbg_ensures(prev->next_blk == curr);
        dbg_ensures(curr->prev_blk == prev);
    }
    list_cnt--;
    dbg_ensures(get_size(prev) == 0);

    int free_cnt = 0;
    block_t *ptr;
    for (ptr = find_next_blk(heap_head); get_size(ptr) > 0;
         ptr = find_next_blk(ptr))
    {
        if (!get_alloc(ptr))
            free_cnt++;
    }
    // printf("free block number: %d\n" "list block number: %d\n", free_cnt,
    // list_cnt);
    dbg_assert(free_cnt == list_cnt);
}
/* Helper Function Definition */

static block_t *extend_heap(size_t size)
{
    void *bp;

    size_t real_size = WSIZE * ((size + (WSIZE)-1) / WSIZE);
    if ((bp = mem_sbrk(real_size)) == (void *)-1)
        return NULL;
    /*
     * | epilogue header (8B) | pointer prev (8B) | pointer next (8B) | (bp)
     */
    block_t *blk = (block_t *)(((char *)bp) - 3 * WSIZE);
    block_t *epilogue_prev = blk->prev_blk;
    // init blk
    write_header(blk, real_size, false);
    write_footer(blk, real_size, false);
    blk->next_blk = NULL;
    blk->prev_blk = NULL;

    // new epilogue header
    block_t *new_epilogue = find_next_blk(blk);
    write_header(new_epilogue, 0, true);
    new_epilogue->prev_blk = epilogue_prev;
    new_epilogue->next_blk = NULL;
    epilogue_prev->next_blk = new_epilogue;

    return coalesce(blk);
}

static block_t *coalesce(block_t *blk)
{
#ifdef DEBUG
    dbg_ensures(!get_alloc(blk));
    block_t *ptr;
    for (ptr = heap_head; ptr != NULL; ptr = ptr->next_blk)
        if (ptr == blk)
            printf("found block: %p\n", blk);
#endif
    block_t *prev_blk = find_prev_blk(blk);
    block_t *next_blk = find_next_blk(blk);
    bool prev_alloc = get_alloc(prev_blk);
    bool next_alloc = get_alloc(next_blk);
    size_t prev_blk_size = get_size(prev_blk);
    size_t next_blk_size = get_size(next_blk);

    size_t blk_size = get_size(blk);
    size_t new_size;
    if (prev_alloc && next_alloc) // both allocated
    {
        link_blk(heap_head, blk);
        return blk;
    }
    else if (prev_alloc && !next_alloc) // coalesce next block
    {
        // remove next block from original link
        next_blk = remove_blk(next_blk);
        new_size = blk_size + next_blk_size;
        write_header(blk, new_size, false);
        write_footer(blk, new_size, false);
        // Last In First Out
        link_blk(heap_head, blk);
        return blk;
    }
    else if (!prev_alloc && next_alloc)
    {
        prev_blk = remove_blk(prev_blk);
        new_size = blk_size + prev_blk_size;
        write_header(prev_blk, new_size, false);
        write_footer(prev_blk, new_size, false);
        link_blk(heap_head, prev_blk);
        return prev_blk;
    }
    else
    {
        prev_blk = remove_blk(prev_blk);
        next_blk = remove_blk(next_blk);
        new_size = blk_size + prev_blk_size + next_blk_size;
        write_header(prev_blk, new_size, false);
        write_footer(prev_blk, new_size, false);
        link_blk(heap_head, prev_blk);
        return prev_blk;
    }
}

static block_t *first_fit(size_t size)
{
    block_t *bp;
    for (bp = heap_head; bp != NULL; bp = bp->next_blk)
        if (!get_alloc(bp) && get_size(bp) >= size)
            return bp;
    return NULL;
}

// unlink blk from the linked list
// then place size bytes inside blk
static void place(block_t *blk, size_t size)
{
    if (!get_alloc(blk))
        blk = remove_blk(blk);
    size_t curr_size = get_size(blk);
    if (curr_size < size)
        return;
    if (curr_size - size < MIN_BLOCK_SIZE)
    {
        write_header(blk, curr_size, true);
        write_footer(blk, curr_size, true);
    }
    else
    {
        write_header(blk, size, true);
        write_footer(blk, size, true);

        blk = find_next_blk(blk);
        write_header(blk, curr_size - size, false);
        write_footer(blk, curr_size - size, false);
        link_blk(heap_head, blk);
    }
}

// link new_blk between prev block and its original next block
static void link_blk(block_t *prev, block_t *new_blk)
{
    if (prev == NULL || new_blk == NULL)
        return;
    block_t *next = prev->next_blk;
    prev->next_blk = new_blk;
    if (next != NULL)
        next->prev_blk = new_blk;
    new_blk->prev_blk = prev;
    new_blk->next_blk = next;
}

// remove blk from its original block
static block_t *remove_blk(block_t *blk)
{
    block_t *prev = blk->prev_blk;
    block_t *next = blk->next_blk;
    blk->next_blk = NULL;
    blk->prev_blk = NULL;
    if (prev != NULL)
        prev->next_blk = next;
    if (next != NULL)
        next->prev_blk = prev;
    return blk;
}

static size_t calc_real_size(size_t size)
{
    // size with overhead (payload at least 16 bytes)
    size_t real_size;
    if (size <= 2 * WSIZE)
        real_size = MIN_BLOCK_SIZE;
    else
        real_size = 2 * WSIZE + WSIZE * ((size + (WSIZE)-1) / WSIZE);
    return real_size;
}

/*
 * pack: returns a header reflecting a specified size and its alloc status.
 *       If the block is allocated, the lowest bit is set to 1, and 0 otherwise.
 */
static word_t pack(size_t size, bool alloc)
{
    return alloc ? (size | 1) : size;
}

static size_t get_size(block_t *block)
{
    return extract_size(block->header);
}

static bool get_alloc(block_t *block)
{
    return (bool)((block->header) & ALLOC_MASK);
}

static size_t extract_size(word_t word)
{
    return (word & SIZE_MASK);
}

static block_t *payload_to_block(void *bp)
{
    return (block_t *)(((char *)bp) - WSIZE);
}

static void *block_to_payload(block_t *block)
{
    return (void *)((char *)block + WSIZE);
}

static void write_header(block_t *block, size_t size, bool alloc)
{
    block->header = pack(size, alloc);
}

static void write_footer(block_t *block, size_t size, bool alloc)
{
    word_t *footerp = (word_t *)(((char *)block) + get_size(block) - WSIZE);
    *footerp = pack(size, alloc);
}

// find next neighbour block
static block_t *find_next_blk(block_t *blk)
{
    block_t *next_blk = (block_t *)(((char *)blk) + get_size(blk));
    return next_blk;
}

// find previous neighbour block
static block_t *find_prev_blk(block_t *blk)
{
    word_t *prev_blk_footer = (word_t *)(((char *)blk) - WSIZE);
    block_t *prev_blk =
        (block_t *)(((char *)blk) - extract_size(*prev_blk_footer));
    return prev_blk;
}
