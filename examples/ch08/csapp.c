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

handler_t *Signal(int signum, handler_t *handler);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
        unix_error("Sigprocmask error");
    return;
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
        unix_error("Sigemptyset error");
    return;
}

void Sigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0)
        unix_error("Sigfillset error");
}

void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
        unix_error("Sigaddset error");
    return;
}

void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
        unix_error("Sigdelset error");
    return;
}

int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
        unix_error("Sigismember error");
    return rc;
}

int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); // always return -1
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

/* private function for sio */
// return length of a string (from K&R)
static size_t sio_strlen(char s[])
{
    size_t i = 0;
    while (s[i++] != '\0')
        ;
    return i;
}

// reverse a string (from K&R)
static void sio_reverse(char s[])
{
    int c, i, j;
    for (i = 0, j = sio_strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

// convert long to base b string (from K&R)
static void sio_ltoa(long v, char s[], int b)
{
    int c, i = 0;
    int neg = v < 0;
    if (neg)
        v = -v;

    do
    {
        s[i++] = ((c = v % b) < 10) ? c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
        s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

ssize_t sio_puts(char s[])
{
    return write(STDOUT_FILENO, s, sio_strlen(s));
}

ssize_t sio_putl(long v)
{
    char s[128];
    sio_ltoa(v, s, 10);
    return sio_puts(s);
}

void sio_error(char s[])
{
    sio_puts(s);
    _exit(1);
}

ssize_t Sio_putl(long v)
{
    ssize_t n;
    if ((n = sio_putl(v)) < 0)
        sio_error("Sio_putl error");
    return n;
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;
    if ((n = sio_puts(s)) < 0)
        sio_error("Sio_puts error");
    return n;
}

void Sio_error(char s[])
{
    sio_error(s);
}
