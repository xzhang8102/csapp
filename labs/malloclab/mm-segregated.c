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
 * |    1 -   16 |
 * |   17 -   64 |
 * |   65 -  128 |
 * |  129 -  256 |
 * |  257 -  512 |
 * |  513 - 2048 |
 * | 2049 - 4096 |
 * | 4097 -    ∞ |
 */

// function declaration
static word_t pack(size_t size, bool alloc);
static word_t extract_size(void *ptr);
static word_t extract_alloc(void *ptr);
static void write_header(void *header, word_t value);
static void write_footer(void *header, word_t value);
static void *header_to_payload(void *header);
static void *payload_to_header(void *payload);
static void *header_to_prev(void *header);
static void *header_to_next(void *header);
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
    if (size_class_start == NULL)
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
    write_header(ptr, pack(DSIZE, true));
    write_footer(ptr, pack(DSIZE, true));
    ptr += DSIZE;
    // epilogue header
    write_header(ptr, pack(0, true));
}

// helper definitions

static word_t pack(size_t size, bool alloc)
{
    return alloc ? (size | 1) : size;
}

static word_t extract_size(void *ptr)
{
    return (*(word_t *)ptr) & SIZE_MASK;
}

static word_t extract_alloc(void *ptr)
{
    return (*(word_t *)ptr) & ALLOC_MASK;
}

static void write_header(void *header, word_t value)
{
    *((word_t *)header) = value;
}

static void write_footer(void *header, word_t value)
{
    word_t *footer = (word_t *)(((char *)header) + extract_size(header) - WSIZE);
    *footer = value;
}

static void *header_to_payload(void *header)
{
    return ((char *)header) + WSIZE;
}

static void *payload_to_header(void *payload)
{
    return ((char *)payload) - WSIZE;
}

static void *header_to_prev(void *header)
{
#ifdef DEBUG
    dbg_assert(!extract_alloc(header));
#endif
    return ((char *)header) + WSIZE;
}

static void *header_to_next(void *header)
{
#ifdef DEBUG
    dbg_assert(!extract_alloc(header));
#endif
    return ((char *)header) + WSIZE + DSIZE;
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
