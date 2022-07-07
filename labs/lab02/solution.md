# Solution

## Phase 1
Use GDB to print the second function input from `%rsi` which is the answer.

## Phase 2
Check the 8 arguments passed to `sscanf`.

## Phase 3
The inputs needs to be two integers and the first one is no larger than 7. The relationships between these two can be inferred from jump table.

## Phase 4
Follow the logic flow and find out the way out from the `func4` recursive call. (I am wondering if modifying values directly in registers and memory is cheating.)
