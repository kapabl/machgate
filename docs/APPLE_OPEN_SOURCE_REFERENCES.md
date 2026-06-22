# Apple open-source references for MachGate

## Purpose

MachGate uses Apple open-source Darwin/macOS repositories as semantic references for Darwin-to-Linux syscall translation.

Do not vendor large Apple source trees into MachGate unless a specific file is needed.

Use these repositories to derive:

- Darwin syscall numbers
- Darwin syscall calling convention
- Darwin errno values
- Darwin structure layouts
- Darwin flag values
- Mach trap behavior
- pthread and thread syscall behavior
- dyld and Mach-O runtime behavior

## Primary references

### distribution-macOS

Repository:

https://github.com/apple-oss-distributions/distribution-macOS

Use this as the macOS open-source distribution index. It points to the Apple open-source components for a macOS release.

### xnu

Repository:

https://github.com/apple-oss-distributions/xnu

Use this for:

- `bsd/kern/syscalls.master`
- `bsd/sys/syscall.h`
- `bsd/sys/errno.h`
- `bsd/sys/stat.h`
- `bsd/sys/fcntl.h`
- `bsd/sys/mman.h`
- `bsd/sys/event.h`
- `osfmk/mach/arm/syscall_sw.h`
- `libsyscall/custom/SYS.h`
- Mach traps
- BSD syscall prototypes
- ARM64 syscall ABI references

Important syscall ABI detail:

Darwin ARM64 syscall wrappers use:

- syscall number in `x16`
- arguments in `x0`, `x1`, `x2`, etc.
- `svc #0x80`
- carry clear on success
- carry set on failure

### Libc

Repository:

https://github.com/apple-oss-distributions/Libc

Use this for:

- libc syscall wrappers
- errno behavior
- POSIX compatibility behavior
- file and path behavior
- stdio and process behavior
- Darwin structure expectations visible to libc users

### libpthread

Repository:

https://github.com/apple-oss-distributions/libpthread

Use this later for:

- pthread initialization
- `bsdthread_*`
- thread-local behavior
- mutex and condition variable semantics
- thread syscalls

Do not implement this before single-threaded CLI syscalls work.

### libplatform

Repository:

https://github.com/apple-oss-distributions/libplatform

Use this later for:

- low-level platform runtime behavior
- locks
- atomics
- platform-specific runtime support

### dyld

Repository:

https://github.com/apple-oss-distributions/dyld

Use this later for:

- dynamic Mach-O loading
- dyld ABI behavior
- chained fixups
- loader behavior
- libSystem startup assumptions

MachGate should preserve Machismo's existing dyld-related work first. Do not rewrite dynamic loading early.

### Libsystem

Repository:

https://github.com/apple-oss-distributions/Libsystem

Use this as the umbrella reference for libSystem composition.

## Initial translation priority

Use Apple source only for the first syscalls needed by real fixtures:

1. `exit`
2. `write`
3. `read`
4. `open`
5. `close`
6. `lseek`
7. `fstat`
8. `mmap`
9. `munmap`
10. `mprotect`

Do not start by implementing:

- signals
- threads
- kqueue
- sockets
- fork
- execve
- posix_spawn
- Mach messages
- Mach ports

## Rule

When implementing a Darwin syscall, record the exact Apple source file used as reference in the commit message or test note.
