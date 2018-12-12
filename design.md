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

## Useful Resources

* To be added
