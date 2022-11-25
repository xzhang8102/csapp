#include "csapp.h"

void sigint_handler(int sig)
{
    Sio_putl(getpid());
    Sio_puts(": sigint received.\n");
    return;
}

int main(int argc, char *argv[])
{
    /* 
     * both parent and child would register this handler
     */
    Signal(SIGINT, sigint_handler);
    pid_t pid;
    if ((pid = Fork()) == 0)
        while (1)
            Sleep(1);

    printf("parent pid: %d, child pid: %d.\n", getpid(), pid);
    while (1)
        Sleep(1);
    return 0;
}
