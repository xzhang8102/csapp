#include "csapp.h"

void sigint_handler(int sig)
{
    printf("Caught SIGINT (signum = %d)!\n", sig);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        unix_error("signal error");

    Pause();
    return 0;
}
