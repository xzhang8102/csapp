/*
 * mm-segregated.c
 *
 * implement the segregated fits allocator that is mentioned on page 864
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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

typedef uint32_t word_t;
static const size_t WSIZE = sizeof(word_t);
static const size_t DSIZE = 2 * WSIZE;
// header (4B) prev (8B) next (8B) footer(4B)
static const size_t MIN_BLOCK_SIZE = 3 * DSIZE;
static const int SIZE_CLASS_NUMBER = 8;
static const size_t CHUNKSIZE = (1 << 12); // 4KB

static const word_t ALLOC_MASK = 0x1;
static const word_t SIZE_MASK = ~(word_t)0x7;

typedef struct
{
    void *next;
} size_class_head;

/* size class (payload size):
 * |    0 -   16 |
 * |   17 -   64 |
 * |   65 -  128 |
 * |  129 -  256 |
 * |  257 -  512 |
 * |  513 - 2048 |
 * | 2049 - 4096 |
 * | 4097 -    ∞ |
 */

static const size_t UPPER_0 = 16;
static const size_t UPPER_1 = 64;
static const size_t UPPER_2 = 128;
static const size_t UPPER_3 = 256;
static const size_t UPPER_4 = 512;
static const size_t UPPER_5 = 2048;
static const size_t UPPER_6 = 4096;

// function declaration
static void *extend_heap(size_t size);
static void *coalesce(void *header);
static void link_blk(void *header);
static void remove_blk(void *header);
static int find_size_class_index(size_t size);

static size_t pack(size_t size, bool alloc);
static size_t extract_size(void *ptr);
static bool extract_alloc(void *ptr);
static void write_header(void *header, size_t value, bool alloc);
static void write_footer(void *header, size_t value, bool alloc);
static void *header_to_payload(void *header);
static void *payload_to_header(void *payload);
static void **header_to_prev(void *header);
static void **header_to_next(void *header);
static void *header_next_neighbor(void *header);
static void *header_prev_neighbor(void *header);

static size_class_head *size_class_start;

int mm_init(void)
{
    /*
     * size class 0 (8B)
     * size class 1 (8B)
     * size class 2 (8B)
     * size class 3 (8B)
     * size class 4 (8B)
     * size class 5 (8B)
     * size class 6 (8B)
     * size class 7 (8B)
     * padding (4B)
     * prologue header (4B) -- for coalescing
     * prologue footer (4B)
     * epilogue header (4B)
     */
    size_class_start = (size_class_head *)mem_sbrk(10 * DSIZE);
    if (size_class_start == (void *)-1)
        return -1;
    int i;
    // init size class head
    for (i = 0; i < SIZE_CLASS_NUMBER; i++)
    {
        size_class_start[i].next = NULL;
    }
    char *ptr = (char *)(size_class_start + SIZE_CLASS_NUMBER);
    // padding
    *((word_t *)ptr) = 0;
    ptr += WSIZE;
    // prologue
    write_header(ptr, DSIZE, true);
    write_footer(ptr, DSIZE, true);
    ptr += DSIZE;
    // epilogue header
    write_header(ptr, 0, true);

    if (extend_heap(CHUNKSIZE) == NULL)
        return -1;
    return 0;
}

// return the header of a free block
static void *extend_heap(size_t size)
{
    void *ptr;
#ifdef DEBUG
    dbg_ensures(size > MIN_BLOCK_SIZE);
#endif
    size_t real_size = DSIZE * ((size + DSIZE - 1) / DSIZE);
    if ((ptr = mem_sbrk(real_size)) == (void *)-1)
        return NULL;
    // | epilogue header (4B) | ptr
    void *header = ((char *)ptr) - WSIZE;
    write_header(header, real_size, false);
    write_footer(header, real_size, false);
    // new epilogue header
    void *epilogue_header = header_next_neighbor(header);
    write_header(epilogue_header, 0, true);

    *header_to_next(header) = NULL;
    *header_to_prev(header) = NULL;
    return coalesce(header);
}

// coalesce a free block
// return the header of a free block
static void *coalesce(void *header)
{
#ifdef DEBUG
    dbg_assert(!extract_alloc(header));
#endif
    void *prev_neighbor = header_prev_neighbor(header);
    void *next_neighbor = header_next_neighbor(header);
    bool prev_alloc = extract_alloc(prev_neighbor);
    bool next_alloc = extract_alloc(next_neighbor);
    size_t prev_size = extract_size(prev_neighbor);
    size_t next_size = extract_size(next_neighbor);

    size_t self_size = extract_size(header);
    size_t new_size;

    if (prev_alloc && next_alloc)
    {
        link_blk(header);
        return header;
    }
    else if (prev_alloc && !next_alloc)
    {
        remove_blk(next_neighbor);
        new_size = self_size + next_size;
        write_header(header, new_size, false);
        write_footer(header, new_size, false);
        link_blk(header);
        return header;
    }
    else if (!prev_alloc && next_alloc)
    {
        remove_blk(prev_neighbor);
        new_size = self_size + prev_size;
        write_header(prev_neighbor, new_size, false);
        write_footer(prev_neighbor, new_size, false);
        link_blk(prev_neighbor);
        return prev_neighbor;
    }
    else
    {
        remove_blk(prev_neighbor);
        remove_blk(next_neighbor);
        new_size = self_size + prev_size + next_size;
        write_header(prev_neighbor, new_size, false);
        write_footer(prev_neighbor, new_size, false);
        link_blk(prev_neighbor);
        return prev_neighbor;
    }
}

static void remove_blk(void *header)
{
    void *prev = *(header_to_prev(header));
    void *next = *(header_to_next(header));
    if (prev)
        *(header_to_next(prev)) = next;
    if (next)
        *(header_to_prev(next)) = prev;
    *(header_to_prev(header)) = NULL;
    *(header_to_next(header)) = NULL;
}

static void link_blk(void *header)
{
    size_t size = extract_size(header);
    int index = find_size_class_index(size - DSIZE);
    void *next = size_class_start[index].next;
    size_class_start[index].next = header;
    *(header_to_prev(header)) = &(size_class_start[index]);
    *(header_to_next(header)) = next;
    if (next != NULL)
        *(header_to_prev(next)) = header;
}

static int find_size_class_index(size_t size)
{
    /* size class (payload size):
     * |    0 -   16 |
     * |   17 -   64 |
     * |   65 -  128 |
     * |  129 -  256 |
     * |  257 -  512 |
     * |  513 - 2048 |
     * | 2049 - 4096 |
     * | 4097 -    ∞ |
     */
    if (size <= UPPER_0)
        return 0;
    else if (size > UPPER_0 && size <= UPPER_1)
        return 1;
    else if (size > UPPER_1 && size <= UPPER_2)
        return 2;
    else if (size > UPPER_2 && size <= UPPER_3)
        return 3;
    else if (size > UPPER_3 && size <= UPPER_4)
        return 4;
    else if (size > UPPER_4 && size <= UPPER_5)
        return 5;
    else if (size > UPPER_5 && size <= UPPER_6)
        return 6;
    else
        return 7;
}

static size_t pack(size_t size, bool alloc)
{
    return alloc ? (size | 1) : size;
}

static size_t extract_size(void *ptr)
{
    return (*(word_t *)ptr) & SIZE_MASK;
}

static bool extract_alloc(void *ptr)
{
    return (*(word_t *)ptr) & ALLOC_MASK;
}

static void write_header(void *header, size_t size, bool alloc)
{
    *((word_t *)header) = pack(size, alloc);
}

static void write_footer(void *header, size_t size, bool alloc)
{
    word_t *footer =
        (word_t *)(((char *)header) + extract_size(header) - WSIZE);
    *footer = pack(size, alloc);
}

static void *header_to_payload(void *header)
{
    return ((char *)header) + WSIZE;
}

static void *payload_to_header(void *payload)
{
    return ((char *)payload) - WSIZE;
}

static void **header_to_prev(void *header)
{
#ifdef DEBUG
    dbg_assert(!extract_alloc(header));
#endif
    return (void **)(((char *)header) + WSIZE);
}

static void **header_to_next(void *header)
{
#ifdef DEBUG
    dbg_assert(!extract_alloc(header));
#endif
    return (void **)(((char *)header) + WSIZE + DSIZE);
}

static void *header_next_neighbor(void *header)
{
    return ((char *)header) + extract_size(header);
}

static void *header_prev_neighbor(void *header)
{
    word_t prev_size = extract_size(((char *)header) - WSIZE);
    return ((char *)header) - prev_size;
}
