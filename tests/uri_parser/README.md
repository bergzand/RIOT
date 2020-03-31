# Overview

This folder contains a test application for fuzz testing the uri parser with
[AFL].

## How to test

1. Install [AFL].
2. Create a directory structure for the test output:
   `mkdir -p   fuzz/{inputs,outputs}`.
3. Build with `make CC=afl-gcc`.
4. Supply some test vectors in the fuzz/inputs directory. 
5. Start the fuzzer with:
   `afl-fuzz -i fuzz/inputs -o fuzz/outputs -- bin/native/tests_uri_parser.elf`

[AFL]: https://github.com/google/AFL
