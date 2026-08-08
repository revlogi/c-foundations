# C Foundations

A growing collection of reusable foundational interfaces for C. The project
started as exercises inspired by David R. Hanson's *C Interfaces and
Implementations* and currently focuses on exception and memory-management
interfaces.

> [!NOTE]
> This project is in early development. Interfaces and implementations may
> change as the library grows and receives broader testing.

## Contents

- `exceptions/`: a `setjmp`/`longjmp` exception implementation, tests, and API
  redesign notes.
- `memory/`: production and checking memory allocators. The checking allocator
  records allocation metadata, detects invalid frees, and fills fresh memory
  with a recognizable byte pattern.

The checking allocator is intended for development and learning. It is not a
replacement for the platform allocator or tools such as AddressSanitizer.

## Build and test

The test suite requires a C11 compiler and POSIX process APIs:

```sh
make test
```

Remove generated files with:

```sh
make clean
```
