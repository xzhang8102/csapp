#include "csapp.h"
#define N 4

int main()
{
  int status, i;
  pid_t pid;

  // Parent creates N children
  for (i = 0; i < N; i++)
    if ((pid = Fork()) == 0)
      return 100+i;

  // Parent reaps N children in no particular order
  while ((pid = waitpid(-1, &status, 0)) > 0)
    if (WIFEXITED(status))
      printf("child %d terminated normally with exit status=%d\n", pid,
             WEXITSTATUS(status));
    else
      printf("child %d terminated abnormally\n", pid);

  // if the error is not `ECHILD`, report the error
  if (errno != ECHILD)
    unix_error("waitpid error");

  return 0;
}
