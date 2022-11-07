/* avoid multiple includes */
#ifndef __CSAPP_H__
#define __CSAPP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ; /* defined by libc */

#define MAXLINE 8192 /* max text line length */

void unix_error(char *msg);
void app_error(char *msg);

pid_t Fork(void);

char *Fgets(char *s, int size, FILE *stream);

#endif // !__CSAPP_H__
