/*
 * mm.c
 *
 * final submission for malloclab
 */
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

typedef uint32_t word_t;
static const size_t WSIZE = sizeof(word_t);
static const size_t DSIZE = 2 * WSIZE;
// header (4B) prev (8B) next (8B)
// (footer(4B) only for free blocks / padding(4B))
static const size_t MIN_BLOCK_SIZE = 3 * DSIZE;
static const int SIZE_CLASS_NUMBER = 8;
static const size_t CHUNKSIZE = (1 << 12); // 4KB

static const word_t ALLOC_MASK = 0x1;
static const word_t PREV_ALLOC_MASK = 0x2;
static const word_t SIZE_MASK = ~(word_t)0x7;

typedef struct
{
    void *next;
} size_class_head;

static const size_t UPPER_0 = 32;
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
static size_t calc_real_size(size_t size);
static void *first_fit(size_t size);
static void place(void *header, size_t size);

static size_t pack(size_t size, bool prev_allc, bool alloc);
static size_t extract_size(void *ptr);
static bool extract_prev_alloc(void *ptr);
static bool extract_alloc(void *ptr);
static void write_header(void *header, size_t value, bool prev_alloc,
                         bool alloc);
static void write_footer(void *header, size_t value, bool prev_alloc,
                         bool alloc);
static void *header_to_payload(void *header);
static void *payload_to_header(void *payload);
static void **header_to_prev(void *header);
static void **header_to_next(void *header);
static void *header_next_neighbor(void *header);
static void *header_prev_neighbor(void *header);
static inline size_t max_size(size_t a, size_t b);

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
     * prologue header (4B)
     * padding (4B)
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
    *((word_t *)ptr) = 0;
    ptr += WSIZE;
    // prologue
    write_header(ptr, DSIZE, true, true);
    ptr += WSIZE;
    *((word_t *)ptr) = 0;
    ptr += WSIZE;
    // epilogue header
    write_header(ptr, 0, true, true);

    if (extend_heap(CHUNKSIZE) == NULL)
        return -1;
    return 0;
}

// return pointer to payload
void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size_t real_size = calc_real_size(size);

    void *header;
    if ((header = first_fit(real_size)) != NULL)
    {
#ifdef DEBUG
        dbg_ensures(!extract_alloc(header));
#endif
        place(header, real_size);
#ifdef DEBUG
        dbg_ensures(extract_alloc(header));
#endif
        return header_to_payload(header);
    }

    if ((header = extend_heap(max_size(CHUNKSIZE, real_size))) == NULL)
        return NULL;
    place(header, real_size);
    return header_to_payload(header);
}

void mm_free(void *payload)
{
    if (payload == NULL)
        return;
    void *header = payload_to_header(payload);
    size_t size = extract_size(header);
    bool prev_alloc = extract_prev_alloc(header);
    write_header(header, size, prev_alloc, false);
    write_header(header, size, prev_alloc, false);
    coalesce(header);
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
    void *header = payload_to_header(old_payload);
    if (!extract_alloc(header))
        return NULL;

    size_t real_size = calc_real_size(size);
    size_t curr_size = extract_size(header);

    if (curr_size >= real_size)
    {
        place(header, real_size);
        return old_payload;
    }
    else
    {
        bool prev_alloc = extract_prev_alloc(header);
        void *next_neighbor = header_next_neighbor(header);
        bool next_alloc = extract_alloc(next_neighbor);
        size_t next_size = extract_size(next_neighbor);
        size_t test_size = curr_size;
        void *prev_neighbor;
        size_t prev_size;

        if (!prev_alloc)
        {
            prev_neighbor = header_prev_neighbor(header);
            prev_size = extract_size(prev_neighbor);
            test_size += prev_size;
        }
        if (!next_alloc)
            test_size += next_size;

        if (test_size >= real_size)
        {
            if (prev_alloc && !next_alloc)
            {
                remove_blk(next_neighbor);
                write_header(header, test_size, prev_alloc, true);
                place(header, real_size);
                return old_payload;
            }
            else if (!prev_alloc)
            {
                remove_blk(prev_neighbor);
                if (!next_alloc)
                    remove_blk(next_neighbor);
                header = prev_neighbor;
                bool alloc = extract_prev_alloc(header);
                write_header(header, test_size, alloc, true);
                place(header, real_size);
                memcpy(header_to_payload(header), old_payload,
                       curr_size - WSIZE);
                return header_to_payload(header);
            }
            else
                return NULL;
        }
        else
        {
            void *new_payload;
            if ((new_payload = mm_malloc(size)) == NULL)
                return NULL;
            memcpy(new_payload, old_payload, curr_size - WSIZE);
            mm_free(old_payload);
            return new_payload;
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
    int list_cnt = 0, free_cnt = 0, i = 0;
    void *ptr;
    while (i < SIZE_CLASS_NUMBER)
    {
        for (ptr = size_class_start[i].next; ptr != NULL;
             ptr = *(header_to_next(ptr)))
        {
            list_cnt++;
        }
        i++;
    }

    void *prologue_header =
        ((char *)size_class_start) + SIZE_CLASS_NUMBER * DSIZE + WSIZE;
    for (ptr = header_next_neighbor(prologue_header); extract_size(ptr) != 0;
         ptr = header_next_neighbor(ptr))
    {
        if (!extract_alloc(ptr))
            free_cnt++;
    }
    assert(list_cnt == free_cnt);
}

// return the header of a free block
static void *extend_heap(size_t size)
{
#ifdef DEBUG
    dbg_ensures(size > MIN_BLOCK_SIZE);
#endif
    void *ptr;
    size_t real_size = DSIZE * ((size + DSIZE - 1) / DSIZE);
    if ((ptr = mem_sbrk(real_size)) == (void *)-1)
        return NULL;
    // | epilogue header (4B) | ptr
    void *header = ((char *)ptr) - WSIZE;
    bool prev_alloc = extract_prev_alloc(header);
    write_header(header, real_size, prev_alloc, false);
    write_footer(header, real_size, prev_alloc, false);
    // new epilogue header
    void *epilogue_header = header_next_neighbor(header);
    write_header(epilogue_header, 0, false, true);

    return coalesce(header);
}

// coalesce a free block
// return the header of a free block
static void *coalesce(void *header)
{
#ifdef DEBUG
    dbg_assert(!extract_alloc(header));
#endif
    void *next_neighbor = header_next_neighbor(header);
    bool next_alloc = extract_alloc(next_neighbor);
    size_t next_size = extract_size(next_neighbor);

    bool prev_alloc = extract_prev_alloc(header);

    size_t new_size = extract_size(header);

    if (prev_alloc && next_alloc)
    {
        link_blk(header);
        return header;
    }
    else if (prev_alloc && !next_alloc)
    {
        remove_blk(next_neighbor);
        new_size += next_size;
        write_header(header, new_size, prev_alloc, false);
        write_footer(header, new_size, prev_alloc, false);
        link_blk(header);
        // affect new neighbor
        next_neighbor = header_next_neighbor(header);
        write_header(next_neighbor, extract_size(next_neighbor), false, extract_alloc(next_neighbor));
        return header;
    }
    else if (!prev_alloc && next_alloc)
    {
        void *prev_neighbor = header_prev_neighbor(header);
        size_t prev_size = extract_size(prev_neighbor);
        remove_blk(prev_neighbor);
        new_size += prev_size;
        bool alloc = extract_prev_alloc(prev_neighbor);
        write_header(prev_neighbor, new_size, alloc, false);
        write_footer(prev_neighbor, new_size, alloc, false);
        link_blk(prev_neighbor);
        return prev_neighbor;
    }
    else
    {
        void *prev_neighbor = header_prev_neighbor(header);
        size_t prev_size = extract_size(prev_neighbor);
        remove_blk(prev_neighbor);
        remove_blk(next_neighbor);
        new_size += prev_size + next_size;
        bool alloc = extract_prev_alloc(prev_neighbor);
        write_header(prev_neighbor, new_size, alloc, false);
        write_footer(prev_neighbor, new_size, alloc, false);
        link_blk(prev_neighbor);
        // affect new neighbor
        next_neighbor = header_next_neighbor(header);
        write_header(next_neighbor, extract_size(next_neighbor), false, extract_alloc(next_neighbor));
        return prev_neighbor;
    }
}

static void *first_fit(size_t size)
{
    int index = find_size_class_index(size);
    void *ptr;
    while (index < SIZE_CLASS_NUMBER)
    {
        for (ptr = size_class_start[index].next; ptr != NULL;
             ptr = *(header_to_next(ptr)))
        {
            if (!extract_alloc(ptr) && extract_size(ptr) >= size)
                return ptr;
        }
        index++;
    }

    return NULL;
}

static void place(void *header, size_t size)
{
    bool allocated = extract_alloc(header);
    if (!allocated)
        remove_blk(header);
    size_t curr_size = extract_size(header);
    bool prev_alloc = extract_prev_alloc(header);
    if (curr_size < size)
        return;
    if (curr_size - size < MIN_BLOCK_SIZE)
    {
        write_header(header, curr_size, prev_alloc, true);
        if (!allocated)
        {
            // current block change from a free block to an allocated one
            // affect its next neighbor
            void *next_neighbor = header_next_neighbor(header);
            write_header(next_neighbor, extract_size(next_neighbor), true, extract_alloc(next_neighbor));
        }
    }
    else
    {
        write_header(header, size, prev_alloc, true);

        header = header_next_neighbor(header);
        write_header(header, curr_size - size, true, false);
        write_footer(header, curr_size - size, true, false);
        coalesce(header);
    }
}

static void remove_blk(void *header)
{
    void *prev = *(header_to_prev(header));
    void *next = *(header_to_next(header));
    if (prev)
    {
        if (prev < (void *)(size_class_start + SIZE_CLASS_NUMBER))
            *((void **)prev) = next;
        else
            *(header_to_next(prev)) = next;
    }
    if (next)
        *(header_to_prev(next)) = prev;
    *(header_to_prev(header)) = NULL;
    *(header_to_next(header)) = NULL;
}

static void link_blk(void *header)
{
    size_t size = extract_size(header);
    int index = find_size_class_index(size);
    void *first = size_class_start[index].next;
    if (first == NULL || extract_size(first) >= size)
    {
        size_class_start[index].next = header;
        *(header_to_prev(header)) = &(size_class_start[index]);
        *(header_to_next(header)) = first;
        if (first != NULL)
            *(header_to_prev(first)) = header;
    }
    else
    {
        void *prev = first, *curr = *(header_to_next(first));
        for (; curr != NULL; prev = curr, curr = *(header_to_next(curr)))
        {
            if (extract_size(curr) >= size)
                break;
        }
        *(header_to_next(prev)) = header;
        *(header_to_prev(header)) = prev;
        *(header_to_next(header)) = curr;
        if (curr != NULL)
            *(header_to_prev(curr)) = header;
    }
}

static int find_size_class_index(size_t size)
{
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

static size_t calc_real_size(size_t size)
{
    size_t min_payload = 2 * DSIZE + WSIZE;
    if (size <= min_payload)
        return MIN_BLOCK_SIZE;
    else
        return MIN_BLOCK_SIZE + DSIZE * ((size - min_payload + DSIZE - 1) / DSIZE);
}

static size_t pack(size_t size, bool prev_alloc, bool alloc)
{
    size = prev_alloc ? (size | 2) : size;
    size = alloc ? (size | 1) : size;
    return size;
}

static size_t extract_size(void *ptr)
{
    return (*((word_t *)ptr)) & SIZE_MASK;
}

static bool extract_prev_alloc(void *ptr)
{
    return (*((word_t *)ptr)) & PREV_ALLOC_MASK;
}

static bool extract_alloc(void *ptr)
{
    return (*((word_t *)ptr)) & ALLOC_MASK;
}

static void write_header(void *header, size_t size, bool prev_alloc, bool alloc)
{
    *((word_t *)header) = pack(size, prev_alloc, alloc);
}

static void write_footer(void *header, size_t size, bool prev_alloc, bool alloc)
{
#ifdef DEBUG
    dbg_assert(!extract_alloc(header));
#endif
    word_t *footer =
        (word_t *)(((char *)header) + extract_size(header) - WSIZE);
    *footer = pack(size, prev_alloc, alloc);
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
    return (void **)(((char *)header) + WSIZE);
}

static void **header_to_next(void *header)
{
    return (void **)(((char *)header) + WSIZE + DSIZE);
}

static void *header_next_neighbor(void *header)
{
    return ((char *)header) + extract_size(header);
}

static void *header_prev_neighbor(void *header)
{
#ifdef DEBUG
    dbg_assert(!extract_prev_alloc(header));
#endif
    word_t prev_size = extract_size(((char *)header) - WSIZE);
    return ((char *)header) - prev_size;
}

static inline size_t max_size(size_t a, size_t b)
{
    return a > b ? a : b;
}
