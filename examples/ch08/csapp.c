#include "csapp.h"

void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void app_error(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

pid_t Fork()
{
    pid_t pid;

    if ((pid = fork()) < 0)
        unix_error("Fork error");

    return pid;
}

char *Fgets(char *s, int size, FILE *stream)
{
    char *rptr;

    if ((rptr = fgets(s, size, stream)) == NULL && ferror(stream))
        app_error("Fgets error");

    return rptr;
}

void Pause(void)
{
    (void)pause();
    return;
}

void Kill(pid_t pid, int signum)
{
    int rc;
    if ((rc = kill(pid, signum)) < 0)
        unix_error("Kill error");
}
