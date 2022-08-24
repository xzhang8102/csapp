#include "cachelab.h"
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ADDRESS_BITS 64

static const char *help_msg = "Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
                              "   -h: Optional help flag that prints usage info\n"
                              "   -v: Optional verbose flag that displays trace info\n"
                              "   -s: <s>: Number of set index bits (S = 2^s is the number of sets)\n"
                              "   -E: <E>: Associativity (number of lines per set)\n"
                              "   -b: <b>: Number of block bits (B = 2^b is the block size)\n"
                              "   -t: <tracefile>: Name of the valgrind trace to replay\n";

static long hit, miss, eviction = 0;

typedef struct cache_line_ele
{
    long tag_id;
    struct cache_line_ele *next;
} cache_line_ele;

typedef struct cache_line_head
{
    int total;
    int capacity;
    cache_line_ele *data;
} cache_line_head;

int main(int argc, char **argv)
{
    // parse command line options
    if (argc <= 1)
    {
        printf("%s", help_msg);
        exit(EXIT_FAILURE);
    }
    int set_bits, cache_line_num, block_bits = 0;
    char *trace_path;
    size_t trace_path_len;

    int opt = 0;
    int verbose = 0;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("%s", help_msg);
            exit(EXIT_SUCCESS);
        case 'v':
            verbose = 1;
            break;
        case 's':
            set_bits = atoi(optarg);
            if (!set_bits)
            {
                printf("Invalid value for -%c\n%s", opt, help_msg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'E':
            cache_line_num = atoi(optarg);
            if (!cache_line_num)
            {
                printf("Invalid value for -%c\n%s", opt, help_msg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'b':
            block_bits = atoi(optarg);
            if (!block_bits)
            {
                printf("Invalid value for -%c\n%s", opt, help_msg);
                exit(EXIT_FAILURE);
            }
            break;
        case 't':
            trace_path_len = strlen(optarg);
            trace_path = (char *)malloc(trace_path_len + 1);
            if (trace_path)
            {
                strcpy(trace_path, optarg);
            }
            else
            {
                printf("malloc of trace file path failed.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            printf("%s", help_msg);
            exit(EXIT_FAILURE);
        }
    }
    // init cache
    int set_num = 0x1 << set_bits;
    cache_line_head **cache = (cache_line_head **)malloc(set_num * sizeof(cache_line_head *));
    if (!cache)
    {
        printf("malloc of cache failed.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < set_num; i++)
    {
        cache_line_head *head = (cache_line_head *)malloc(sizeof(cache_line_head));
        if (!head)
        {
            printf("malloc of cache line failed.\n");
            exit(EXIT_FAILURE);
        }
        head->total = cache_line_num;
        head->capacity = 0;
        head->data = NULL;
        cache[i] = head;
    }
    // init mask
    unsigned long mask_tag, mask_set = 0x1;
    mask_set = (0x1 << set_bits) - 1;
    mask_tag = (0x1 << (ADDRESS_BITS - set_bits - block_bits)) - 1;

    // scan trace file
    FILE *trace_file = fopen(trace_path, "r");
    if (!trace_file)
    {
        fprintf(stderr, "cannot open %s for reading\n", trace_path);
        exit(EXIT_FAILURE);
    }
    int buffer_size = 255;
    char buffer[buffer_size];
    while (fgets(buffer, buffer_size, trace_file))
    {
        if (buffer[0] != ' ')
            continue;
        if (verbose)
            buffer[strlen(buffer) - 1] = ' ';
        long addr = strtol(&buffer[3], NULL, 16);
        long tag_id = (addr >> (set_bits + block_bits)) & mask_tag, set_id = (addr >> block_bits) & mask_set;
        assert(set_id < set_num);
        char mode = buffer[1];
        if (cache[set_id]->capacity == 0)
        {
            // cold miss
            miss++;
            if (mode == 'M')
                hit++;
            if (verbose)
            {
                strcat(buffer, "miss");
                if (mode == 'M')
                    strcat(buffer, " hit");
            }
            cache_line_ele *new_ele = (cache_line_ele *)malloc(sizeof(cache_line_ele));
            if (!new_ele)
            {
                fprintf(stderr, "malloc for new cache line element failed\n");
                exit(EXIT_FAILURE);
            }
            new_ele->tag_id = tag_id;
            new_ele->next = NULL;
            cache[set_id]->capacity++;
            cache[set_id]->data = new_ele;
        }
        else
        {
            // walk through cache line elements to find a match of tag id
            cache_line_ele *tail = cache[set_id]->data;
            cache_line_ele *match = NULL;
            while (tail)
            {
                if (tail->tag_id == tag_id)
                    match = tail;
                if (tail->next == NULL)
                    break;
                tail = tail->next;
            }
            if (match)
            {
                // hit and move the element to the tail
                hit++;
                if (mode == 'M')
                    hit++;
                if (verbose)
                {
                    strcat(buffer, "hit");
                    if (mode == 'M')
                        strcat(buffer, " hit");
                }
                if (match != tail)
                {
                    cache_line_ele *tmp = cache[set_id]->data;
                    cache_line_ele *prev = NULL;
                    while (tmp != match)
                    {
                        prev = tmp;
                        tmp = tmp->next;
                    }
                    if (prev == NULL)
                        cache[set_id]->data = match->next;
                    else
                        prev->next = match->next;
                    match->next = NULL;
                    tail->next = match;
                }
            }
            else
            {
                cache_line_ele *new_ele = (cache_line_ele *)malloc(sizeof(cache_line_ele));
                if (!new_ele)
                {
                    fprintf(stderr, "malloc for new cache line element failed\n");
                    exit(EXIT_FAILURE);
                }
                new_ele->tag_id = tag_id;
                new_ele->next = NULL;
                if (cache[set_id]->capacity < cache[set_id]->total)
                {
                    // no match but there is enough space to add
                    miss++;
                    if (mode == 'M')
                        hit++;
                    if (verbose)
                    {
                        strcat(buffer, "miss");
                        if (mode == 'M')
                            strcat(buffer, " hit");
                    }
                    tail->next = new_ele;
                    cache[set_id]->capacity++;
                }
                else
                {
                    // eviction
                    miss++;
                    eviction++;
                    if (mode == 'M')
                        hit++;
                    if (verbose)
                    {
                        strcat(buffer, "miss eviction");
                        if (mode == 'M')
                            strcat(buffer, " hit");
                    }
                    cache_line_ele *head = cache[set_id]->data;
                    if (head == tail)
                    {
                        free(head);
                        cache[set_id]->data = new_ele;
                    }
                    else
                    {
                        cache[set_id]->data = head->next;
                        free(head);
                        tail->next = new_ele;
                    }
                }
            }
        }
        if (verbose)
            printf("%s\n", buffer);
    }

    // clean up
    free(trace_path);
    for (int i = 0; i < set_num; i++)
    {
        cache_line_ele *ptr = cache[i]->data;
        while (ptr)
        {
            cache_line_ele *next = ptr->next;
            free(ptr);
            ptr = next;
        }
        free(cache[i]);
    }
    free(cache);
    fclose(trace_file);
    printSummary(hit, miss, eviction);
    return 0;
}
