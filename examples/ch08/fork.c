#include "csapp.h"

int main()
{
  pid_t pid;
  int x = 1;

  pid = Fork();

  if (pid == 0) // child
  {
    printf("child : x=%d\n", ++x);
    exit(0);
  }

  printf("parent: x=%d\n", --x);
  exit(0);
}

/**
 * output (non-deterministic):
 * child : x=2
 * parent: x=0
 *
 * Call once, return twice
 * The `fork` system call creates two separate processes.
 *
 * Concurrent execution
 * We can never make assumptions about the interleaving of the instructions in
 * different processes.
 *
 * Duplicate but separate address spaces:
 * Both parent and child process have the same user stack, the same local
 * variable values, the same heap, the same global variable values and the same
 * code. `x` has a value of 1 in both process when `Fork` returns. However,
 * since the parent and the child are *separate* processes, they each have their
 * own private address spaces and any subsequent changes that a process or child
 * makes to `x` are private and are not reflected in the memory of other
 * process.
 *
 * Shared files.
 * Both processes output through the same `stdout` and the reason is that
 * the child inherits all of the parent's open files.
 *
 */
