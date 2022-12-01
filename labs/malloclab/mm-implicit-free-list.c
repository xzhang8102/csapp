/*
 * mm-implicit.c
 * implicit free list with first fit allocation strategy
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

#define WSIZE 4             // word and header/footer size
#define DSIZE 8             // double word size
#define CHUNKSIZE (1 << 11) // extend heap by this amount (bytes)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)                                                      \
    ((size) | (alloc)) // pack a size and allocated bit into a word

// read and write a word at address p
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// given block ptr bp, compute address of its header and footer
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

static char *heap_listp = NULL;
static char *next_fit_start = NULL;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/*
 * mm_init - Called when a new trace starts.
 */
int mm_init(void)
{
    if ((heap_listp = (mem_sbrk(4 * WSIZE))) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // epilogue header
    heap_listp += (2 * WSIZE);
    // heap_listp can be viewed as a block (payload size = 0) pointer
    next_fit_start = heap_listp;
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

void *mm_malloc(size_t size)
{
    size_t asize;      // adjusted block size
    size_t extendsize; // amount to extendheap if no fit
    char *bp;

    if (size == 0)
        return NULL;

    // block size is at least 16 bytes with 8 bytes of header and footer
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;

    place(bp, asize);
    return bp;
}

void mm_free(void *ptr)
{
    if (ptr == NULL)
        return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

void *mm_realloc(void *old_ptr, size_t size)
{
    if (old_ptr == NULL)
        return mm_malloc(size);
    if (size == 0)
    {
        free(old_ptr);
        return NULL;
    }
    if (!GET_ALLOC(HDRP(old_ptr)))
        return NULL;

    size_t asize;
    // size is at least 16 bytes with 8 bytes of header and footer
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    size_t old_size = GET_SIZE(HDRP(old_ptr));
    if (asize <= old_size)
    {
        place(old_ptr, asize);
        return old_ptr;
    }
    else
    {
        void *next_blk = NEXT_BLKP(old_ptr);
        void *prev_blk = PREV_BLKP(old_ptr);
        size_t new_size = old_size;
        if (!GET_ALLOC(HDRP(next_blk)))
            new_size += GET_SIZE(HDRP(next_blk));
        if (!GET_ALLOC(HDRP(prev_blk)))
            new_size += GET_SIZE(HDRP(prev_blk));
        void *new_ptr;
        if (new_size >= asize)
        {
            new_ptr = coalesce(old_ptr);
            if (new_ptr != old_ptr)
                memcpy(new_ptr, old_ptr, old_size - DSIZE);
            place(new_ptr, asize);
            return new_ptr;
        }
        else
        {
            new_ptr = mm_malloc(asize);
            memcpy(new_ptr, old_ptr, old_size - DSIZE);
            free(old_ptr);
            return new_ptr;
        }
    }
}

void *mm_calloc(size_t nmemb, size_t size)
{
    size_t bytes = nmemb * size;
    void *newptr = mm_malloc(bytes);
    if (newptr != NULL)
        memset(newptr, 0, bytes);
    return newptr;
}

void mm_checkheap(int verbose)
{
    if (verbose > 0)
    {
        void *brk = mem_sbrk(0);
        char *epilogue_header = (char *)brk - WSIZE;
        printf("%p --> start of the heap\n", mem_heap_lo());
        printf("%p --> prologue header\n", heap_listp - WSIZE);
        printf("%p --> prologue footer\n\n", heap_listp);
        char *bp;
        for (bp = heap_listp + DSIZE; bp != brk; bp = NEXT_BLKP(bp))
        {
            printf("header(4 bytes): size: %d | alloc: %d\n",
                   GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
            printf("---------------------------------\n"
                   "| %p --> payload start |\n"
                   "---------------------------------\n",
                   bp);
            printf("footer(4 bytes): size: %d | alloc: %d\n\n",
                   GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)));
        }
        printf("%p --> epilogue header\n", epilogue_header);
        printf("%p --> end   of the heap\n", brk);
        printf("total heap size: %zu bytes\n", mem_heapsize());
        printf("\n");
    }
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epilogue header

    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) // case 1
        return bp;
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    if ((next_fit_start > (char *)bp) && (next_fit_start < NEXT_BLKP(bp)))
        next_fit_start = bp;
    return bp;
}

static void *find_fit(size_t asize)
{
    char *curr = next_fit_start;

    for (; GET_SIZE((HDRP(next_fit_start))) > 0;
         next_fit_start = NEXT_BLKP(next_fit_start))
        if (!GET_ALLOC(HDRP(next_fit_start)) && (asize <= GET_SIZE(HDRP(next_fit_start))))
            return next_fit_start;

    for (next_fit_start = heap_listp; next_fit_start < curr; next_fit_start = NEXT_BLKP(next_fit_start))
        if (!GET_ALLOC(HDRP(next_fit_start)) && (asize <= GET_SIZE(HDRP(next_fit_start))))
            return next_fit_start;

    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    if (csize < asize)
        return;
    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
