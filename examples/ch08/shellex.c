#include "csapp.h"
#define MAXARGS 128

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int main(int argc, char *argv[])
{
    char cmdline[MAXLINE];

    while (1)
    {
        // Read
        printf("> ");
        Fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0);

        // Evaluate
        eval(cmdline);
    }
}

void eval(char *cmdline)
{
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return;

    if (!builtin_command(argv))
    {
        if ((pid = Fork()) == 0)
            if (execve(argv[0], argv, environ) < 0)
            {
                printf("%s: command not found.\n", argv[0]);
                exit(0);
            }

        if (!bg)
        {
            int status;
            if (waitpid(pid, &status, 0) < 0)
                unix_error("waitfg: waitpid error");
        }
        else
            printf("%d %s\n", pid, cmdline);
    }

    return;
}

int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "quit"))
        exit(0);
    if (!strcmp(argv[0], "&"))
        return 1;
    return 0;
}

int parseline(char *buf, char **argv)
{
    char *delim;
    int argc = 0;
    int bg = 0;

    buf[strlen(buf) - 1] = ' ';
    while (*buf && (*buf == ' ')) // ignore leading spaces
        buf++;

    while ((delim = strchr(buf, ' ')))
    {
        if (argc == MAXARGS - 1)
        {
            printf("The number of arguments is beyond limit.\n");
            exit(0);
        }
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' '))
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0)
        return 1;

    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL;

    return bg;
}
