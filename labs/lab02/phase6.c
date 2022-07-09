#include <stdio.h>

void explode()
{
  printf("Boom!\n");
}

void phase_6(char *input)
{
  // from (%rsp) - (%rsp + 0x17)
  int arr[6];
  int n = sscanf(input, "%d %d %d %d %d %d", &arr[0], &arr[1], &arr[2], &arr[3], &arr[4], &arr[5]);
  if (n <= 5)
  {
    explode();
  }
  for (int i = 0; i < 6; i++)
  {
    if ((unsigned int)arr[i] - 1 <= 5)
    {
      for (int j = i + 1; j < 6; j++)
      {
        if (arr[j] == arr[i])
        {
          explode();
        }
      }
    }
    else
    {
      explode();
    }
  }
  // <phase_6+95>
  for (int *p = arr; p < p + 6; p++)
  {
    *p = 7 - *p;
  }
  // static array
  static long data[12];          // Addr
  data[0] = 0x000000010000014c;  // 0x6032d0
  data[1] = (long)&data[2];      // 0x6032d8
  data[2] = 0x00000002000000a8;  // 0x6032e0
  data[3] = (long)&data[4];      // 0x6032e8
  data[4] = 0x000000030000039c;  // 0x6032f0
  data[5] = (long)&data[6];      // 0x6032f8
  data[6] = 0x00000004000002b3;  // 0x603300
  data[7] = (long)&data[8];      // 0x603308
  data[8] = 0x00000005000001dd;  // 0x603310
  data[9] = (long)&data[10];     // 0x603318
  data[10] = 0x00000006000001bb; // 0x603320
  data[11] = 0;                  // 0x603328
  // <phase_6+123>
  // from %rsp+0x20
  long *ptr[6];
  for (int i = 0; i != 6; i++)
  {
    if (arr[i] <= 1)
    {
      ptr[i] = data;
    }
    else
    {
      // int x = 1;
      // long *y = data;
      // do
      // {
      //   y = (long *)*(y + 1);
      //   x++;
      // } while (x != arr[i]);
      // ptr[i] = y;
      ptr[i] = (long *)data[(arr[i] - 1) * 2];
    }
  }
  // <phase_6+183>
  long *base = ptr[0]; // %rbx
  long *rcx = base;
  long *rdx;
  for (int i = 1;;) // %rax
  {
    rdx = base + i;         // mov (%rax),%rdx
    *(rcx + 1) = (long)rdx; // mov %rdx,0x8(%rcx)
    i++;
    if (i == 6)
      break;
    rcx = rdx;
  }
  // <phase_6+222>
  *(rdx + 1) = 0;
  int cnt = 5;
  do
  {
    int n = *(base + 1);
    if ((int)*base < n)
    {
      explode();
      break;
    }
    base += 1;
    cnt--;
  } while (cnt != 0);
}