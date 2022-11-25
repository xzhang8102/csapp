#include "csapp.h"

void mmapcopy(int fd, size_t size)
{
    char *bufp;
    bufp = (char *)Mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    Write(1, bufp, size);
    return;
}

int main(int argc, char *argv[])
{
    struct stat stat;
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    int fd;
    fd = Open(argv[1], O_RDONLY, 0);
    fstat(fd, &stat);
    mmapcopy(fd, stat.st_size);
    return 0;
}
