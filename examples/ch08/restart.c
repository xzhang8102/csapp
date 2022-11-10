#include "csapp.h"

sigjmp_buf buf;

void handler(int sig)
{
    siglongjmp(buf, 1);
    /*
     * cause sigsetjmp to run again and return 1
     *
     * Be careful to call only async-signal-safe
     * functions in any code reachable from a
     * siglongjmp
     */
}

int main(int argc, char *argv[])
{
    if (!sigsetjmp(buf, 1))
    {
        /*
         * installing handler after the `sigsetjmp` call
         * if not, there would be the risk of the handler
         * running before the initial `sigsetjmp` call
         */
        Signal(SIGINT, handler);
        Sio_puts("starting\n");
    }
    else
        Sio_puts("restarting\n");

    while (1)
    {
        Sleep(1);
        Sio_puts("processing...\n");
    }
    /* the unsafe `exit` is unreachable */
    exit(0);
}
