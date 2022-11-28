/**
 * 15-213 Proclab Student's Solution File
 * @author <your name here>
 * @andrewid <andrew ID here>
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* The type for signal handler functions
 * You can use this. */
typedef void (*sighandler_t)(int);


/*****************************************************************************\
*******************************************************************************
**  Timer Puzzle
*******************************************************************************
\*****************************************************************************/
/**
 *  This function reads a line from stdin, waiting indefinitely until a newline
 *  character is encountered.
 *  You are expected to call this to actually read a line from stdin
 *
 *  @param buf Buffer to hold the line
 *
 *  @param buf_size Size of the buffer
 *
 *
 *  @return -1 if interrupted by a signal or if there is an error
 *          otherwise the number of characters read, including the newline
 *          character (if present), but excluding the null-terminator.
 *
 *  @remarks  This function will read everything from STDIN until the newline
 *            character is encountered.
 *            The buffer will contain the newline character 
 *            and null-terminator.
 *            However, if there is insufficient space, then 
 *            the buffer will store a truncated version of the line read,
 *            and the newline character will *not* be present.
 *            However, in any case, the buffer will still be null-terminated.
 *                   
 *  */
ssize_t mygets(char* buf, int buf_size);

/**
 * Use this to set the alarm signal handler.
 * @param alarm_handler  Address of the Signal handler for SIGALRM
 * @return SIG_ERR on error; otherwise the address of the previous handler
 */
sighandler_t set_alarm_handler(sighandler_t alarm_handler)
{
    struct sigaction action, old_action;

    action.sa_handler = alarm_handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    
    /* Make sure you do not specify SA_RESTART,
    SA_RESTART will cause IO functions to be restarted when
    they are interrupted by signals. */
    action.sa_flags = 0;  

    if (sigaction(SIGALRM, &action, &old_action) < 0)
    {
        return SIG_ERR;
    }
    
    return old_action.sa_handler;
}

volatile sig_atomic_t timeout = 0;

void alarm_handler(int sig)
{
    timeout = 1;
    return;
}

/**
 * Reads a line from STDIN within a timeout period.
 *
 * @param buf Pointer to buffer to hold the line read from STDIN,
 *        including the newline character.
 *
 * @param buf_size Size of the buffer, including the space for holding the
 *        newline and null-terminator character. If there is insufficient 
 *        space, then buffer will store a truncated version of the line. 
 *        In whatever case, buffer must be null-terminated.
 *
 * @param timeout_secs Number of seconds before timing out;
 *        Can be zero if no timeout is desired, in which case this
 *        function should wait indefinitely  for a line of input.
 *
 * @return -1 if interrupted by a signal or if there is an error.
 *         Number of characters read (including the newline char if present
 *            but excluding the null-terminator) if a line is read 
 *             before timing out.
 *         0 if timeout happened before finishing reading a line.
 *
 *  @remarks Note that reading a line means read everything from STDIN until
 *           the newline character is encountered. 
 *           So if 'timeout_secs' seconds has passed *before* a newline char is
 *           encountered, then this function must return 0.
 *           If a newline char is encountered within 'timeout_secs' seconds,
 *           then this function must return the number of characters read. 
 *
 *           IMPORTANT!!!: This function should restore the original alarm 
 *           signal handler before returning!
 *
 * */
ssize_t tgets(char* buf, int buf_size, int timeout_secs)
{
    ssize_t num_read = 0;
    
    /* TODO: set up your alarm handler here */
    sighandler_t old_handler = set_alarm_handler(alarm_handler);
    
    /* Use this to read a line */
    alarm(timeout_secs);
    num_read = mygets(buf, buf_size);
    
    /* TODO: restore alarm handler */
    set_alarm_handler(old_handler);
    /* TODO: check the return value of mygets */
    if (num_read < 0) {
        if (timeout) {
            return 0;
        }
        return num_read;
    }
    /* TODO: change the return value  */
    return num_read;
}

/*****************************************************************************\
*******************************************************************************
**  Racer Puzzle
*******************************************************************************
\*****************************************************************************/

/* helper functions */
void printAletter();


/* @param pid - the pid of the child process*/
void racer1(int pid)
{
    int status;
    if (waitpid(pid, &status, 0))
    {
        if (WIFEXITED(status)) {
            /*do not remove this line*/
            printAletter();
        }
    }
}

void setup_race2()
{
    sigset_t mask_chld;
    sigaddset(&mask_chld, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask_chld, NULL);
}

/* @param pid - the pid of the child process*/
void racer2(int pid)
{
    sigset_t mask;
    /*do not remove this line*/
    printAletter();
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    waitpid(pid, NULL, 0);
}

/*****************************************************************************\
*******************************************************************************
**  Decipher Puzzle
*******************************************************************************
\*****************************************************************************/

void decipher(const char* encrypted_words[])
{
    char **ptr;
    char *argv[3];
    extern char **environ;
    argv[0] = "./decipher";
    argv[2] = NULL;
    for (ptr = (char **)encrypted_words; *ptr != NULL; ptr++) {
        pid_t pid;
        pid = fork();
        if (pid < 0)
            exit(1);
        if (pid == 0) {
            argv[1] = *ptr;
            if (execve(argv[0], argv, environ) < 0)
              exit(1);
        } else
            waitpid(pid, NULL, 0);
    }
}


/*****************************************************************************\
*******************************************************************************
**  Counter Puzzle
*******************************************************************************
\*****************************************************************************/
void counter(int num_direct_children)
{
    int total;
    int status;
    if (num_direct_children == 0) {
        exit(1);
    }
    total = 1;
    while (wait(&status) > 0) {
        if (WIFEXITED(status)) {
            total += WEXITSTATUS(status);
        }
    }
    exit(total);
}

/*****************************************************************************\
*******************************************************************************
**  Reaper Puzzle
*******************************************************************************
\*****************************************************************************/

/*
 * safe_printf - async-signal-safe wrapper for printf
 */
void safe_printf(const char *format, ...);


/*note: this is a handler for SIGCHLD*/
void reaper(int sig)
{
    pid_t pid;
    int status;
    sigset_t mask, prev;

    sigfillset(&mask);
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
        sigprocmask(SIG_BLOCK, &mask, &prev);
        if (WIFEXITED(status))
            safe_printf("Job (%d) exited with status %d\n", pid,
                        WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            safe_printf("Job (%d) terminated by signal %d\n", pid,
                        WTERMSIG(status));
        else if (WIFSTOPPED(status))
            safe_printf("Job (%d) stopped by signal %d\n", pid,
                        WSTOPSIG(status));
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }
}

/*****************************************************************************\
*******************************************************************************
**  Shower Puzzle
*******************************************************************************
\*****************************************************************************/

/** You need to call this in each of your signal handler */
void signal_received(int signum);

volatile sig_atomic_t hit;

void sig_handler(int sig)
{
    signal_received(sig);
    hit = 1;
}

typedef void handler_t(int sig) ;

handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if (sigaction(signum, &action, &old_action) < 0)
        fprintf(stderr, "Signal error");
    return old_action.sa_handler;
}

/** 
 * Setup your signal handlers here.
 *
 */
void shower_setup(void)
{ 
    sigset_t mask;
    hit = 0;
    Signal(SIGALRM, sig_handler);
    Signal(SIGUSR1, sig_handler);
    Signal(SIGUSR2, sig_handler);
    Signal(SIGCONT, sig_handler);
    Signal(SIGCHLD, sig_handler);
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void shower_helper(int sig)
{
    sigset_t tmp;
    sigfillset(&tmp);
    sigdelset(&tmp, sig);
    while (!hit)
        sigsuspend(&tmp);
    hit = 0;
}

/**
 * Block off the appropriate signals
 */
void shower_run(void)
{
    shower_helper(SIGALRM);
    shower_helper(SIGUSR1);
    shower_helper(SIGUSR2);
    shower_helper(SIGCONT);
    shower_helper(SIGCHLD);
}
