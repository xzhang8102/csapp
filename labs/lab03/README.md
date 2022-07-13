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
Change the return address to `touch2` during runtime.

### Level 3
Change the return address to `touch3` during runtime.
Exploit `push %rbx` from `touch3` to save string literal on stack.
