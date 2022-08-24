#include "cachelab.h"
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ADDRESS_BITS 64

static const char *help_msg = "Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
                              "   -h: Optional help flag that prints usage info\n"
                              "   -v: Optional verbose flag that displays trace info\n"
                              "   -s: <s>: Number of set index bits (S = 2^s is the number of sets)\n"
                              "   -E: <E>: Associativity (number of lines per set)\n"
                              "   -b: <b>: Number of block bits (B = 2^b is the block size)\n"
                              "   -t: <tracefile>: Name of the valgrind trace to replay\n";

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

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("%s", help_msg);
            exit(EXIT_SUCCESS);
        case 'v': /* TODO: debug mode */
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

    // scan trace file
    FILE *trace_file = fopen(trace_path, "r");
    if (!trace_file)
    {
        fprintf(stderr, "cannot opt %s for reading\n", trace_path);
        exit(EXIT_FAILURE);
    }
    int buffer_size = 255;
    char buffer[buffer_size];
    while (fgets(buffer, buffer_size, trace_file))
    {
        printf("%s", buffer);
    }
    printSummary(0, 0, 0);
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
    fclose(trace_file);
    return 0;
}
