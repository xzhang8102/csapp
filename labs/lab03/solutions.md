# Attack Lab

## Part 1

### Level 1
Use buffer overflow in `Gets()` to overwrite the return address of `getbuf()`.

### Level 2
Change the return address to `touch2` during runtime.

### Level 3
Change the return address to `touch3` during runtime.
Exploit `push %rbx` from `touch3` to save string literal on stack.
