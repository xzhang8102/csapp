#include "csapp.h"

int main(int argc, char *argv[])
{
    pid_t pid;
    if ((pid = Fork()) == 0)
    {
        Pause();
        printf("control should never reach here!\n");
        exit(0);
    }

    /*
     * pid > 0, kill sends signum to pid
     * pid == 0, kill sends signum to the process group of the calling process, including the calling process itself
     * pid < 0, kill sends signum to every process in process group with |pid|
     */
    Kill(pid, SIGKILL);
    return 0;
}
