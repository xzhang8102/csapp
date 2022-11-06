#include "csapp.h"
#define N 4

int main()
{
  int status, i;
  pid_t pid[N], retpid;

  // Parent creates N children
  for (i = 0; i < N; i++)
    if ((pid[i] = Fork()) == 0)
      return 100 + i;

  // Parent reaps N children in order
  i = 0;
  while ((retpid = waitpid(pid[i++], &status, 0)) > 0)
    if (WIFEXITED(status))
      printf("No.%d child (pid = %d) terminated normally with exit status=%d\n",
             i, retpid, WEXITSTATUS(status));
    else
      printf("No.%d child (pid = %d) terminated abnormally\n", i, retpid);

  // if the error is not `ECHILD`, report the error
  if (errno != ECHILD)
    unix_error("waitpid error");

  return 0;
}
