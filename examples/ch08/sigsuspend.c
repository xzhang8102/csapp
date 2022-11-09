#include "csapp.h"

volatile sig_atomic_t pid;

void sigchld_handler(int s)
{
    int olderrno = errno;
    pid = Waitpid(-1, NULL, 0);
    errno = olderrno;
}

void sigint_handler(int s) {}

int main(int argc, char *argv[])
{
    sigset_t mask, prev;
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);

    while (1)
    {
        /* in case SIGCHLD is sent before `pid = 0` */
        Sigprocmask(SIG_BLOCK, &mask, &prev);
        if (Fork() == 0)
            exit(0);

        pid = 0;
        /* using a plain loop to watch `pid` is wasteful */
        // while (!pid)
        //     ;
        while (!pid)
            Sigsuspend(&prev); /* unblock SIGCHLD, this operation is atomic */

        /* optionally unblock SIGCHLD */
        Sigprocmask(SIG_SETMASK, &prev, NULL);

        printf("."); // do some work after receiving SIGCHLD
    }
    return 0;
}
