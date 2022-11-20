# Attack Lab

## Description
<details><summary>click</summary>
Files:

    ctarget

Linux binary with code-injection vulnerability.  To be used for phases
1-3 of the assignment.

    rtarget

Linux binary with return-oriented programming vulnerability.  To be
used for phases 4-5 of the assignment.

     cookie.txt

Text file containing 4-byte signature required for this lab instance.

     farm.c

Source code for gadget farm present in this instance of rtarget.  You
can compile (use flag -Og) and disassemble it to look for gadgets.

     hex2raw

Utility program to generate byte sequences.  See documentation in lab
handout.
</details>

## Part 1

> **Note**  
> It seems that `%rsp` can only be set to `0x5561dca0` when calling required functions, otherwise segmentation fault may occur.

### Level 1
Use buffer overflow in `Gets()` to overwrite the return address of `getbuf()`.

### Level 2
Push the return address to `touch2` during runtime.

### Level 3
Change `%rdi` to `%rsp` which points to the required string.
Push the return address to `touch3` during runtime.

## Part 2

### Level 2
Overflow buffer with byte sequences: gadget 1's address, cookie value, gadget 2's address, touch2's address

### Level 3
The tricky part is finding out there is a `add_xy` function in `farm.c`.
And the rest is calculating `%rdi` to point to the required string literal.

I solved this problem based on the solutions from [Exely/CSAPP-Labs](https://github.com/Exely/CSAPP-Labs/tree/master/labs/attack) and [Sorosliu1029/CSAPP-Labs](https://github.com/Sorosliu1029/CSAPP-Labs/tree/master/attack-lab).
