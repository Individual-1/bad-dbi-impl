# Bad DBI Implementation Design Notes

## Rough Initial Design

* Have DBI framework parent fork off the target binary and stop execution at entry point

* Initially translate everything without caching (extremely slow)

* Eventually use DyanamoRIO model of caching instrumented basic blocks

* Use LLVM disasm module to parse instructions to find basic blocks

* Potentially use LLVM to do asm -> IL for easier transforms

* Need to instrument all returns such that they return to a dispatcher instead of native program

## General Operation

1. In parent program, parse arguments and figure out what instrumentation is needed

1. Fork and stop execution of the child process before calling execve

1. Once child program is loaded into memory, read its code segments and run each basic block through instrumentation before actually executing

1. Eventually cache these basic blocks and do things like rewriting static jumps

## Specific Things

### Setting up our dispatcher

1. Read out first X bytes of child's rsp and back it up

1. Write some bootstrap code into there that sets up a region of memory to write our dispatcher to

1. Have it run the bootstrap then jump to some init code in the dispatcher that signals the parent to write the original code back in

1. Set the dispatcher to start executing (with its translation functions) from the original child rsp

### Bootstrap for dispatcher

1. Find a free region of memory to dump dispatcher in

1. mmap it to be writeable and write our code in, then mprotect to make it executable

### Handling control-flow instructions

#### ret

Might need slightly different code for near vs far returns, but in general should be similar method.

* Expect an address at `rsp`, change the ret to a jmp to dispatcher and use `[rsp]` as our next target

#### jmp and related

We could calculate out the jmp target and push it to stack and use the same code as for the ret handler? If it is a static jump target maybe we can bypass dispatcher and just write in the target address in our cache (if we have implemented an instrumented cache).

#### loop/loopcc

We could potentially treat this the same as a static jmp, except make sure we are saving and reloading context? Might not need to treat it any differently.

## Useful Resources

* To be added
