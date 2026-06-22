# MachGate syscall translation plan

## Goal

Implement Darwin-to-Linux syscall translation for static ARM64 Mach-O executables that issue Darwin syscalls directly with `svc #0x80`.

Keep the existing Machismo loader, resolver, trampoline system, and executable-section patching model intact.

## Scope

Initial target:

- guest format: ARM64 Mach-O
- host architecture: ARM64 / aarch64
- host OS: Linux
- workload: command-line executables

Initial implementation scope:

- single guest thread
- no GUI
- no Mach IPC
- no child process support
- no signal emulation
- no x86-64 support

## Existing repo hooks to reuse

The syscall translation path should follow patterns already present in this repo:

- `src/machismo.c` already performs a pre-execution scan of executable sections
- `src/isa_emul.c` already patches matching ARM64 instructions into direct branches to nearby islands
- `src/loader.c` and `src/dylib_loader.c` already reserve adjacent executable pool space for branch islands
- `src/shim/libsystem_shim.c` already contains Darwin-to-Linux translation logic for flags and structure layouts that should be reused later

Do not introduce a separate loader architecture for syscall handling.

## Design

### Detection

Before guest execution, scan executable instruction sections of the main Mach-O image for Darwin syscall instructions:

- match `svc #0x80`
- ignore non-instruction sections

### Patching

For each matched `svc #0x80`:

1. allocate a nearby branch island from the existing adjacent pool
2. replace the original 4-byte `svc` with a direct `b` instruction to the island
3. have the island return to the instruction immediately after the patched `svc`

Use `b`, not `bl`, so guest `x30` is preserved.

### Island responsibilities

The island should be a narrow ABI boundary:

- preserve guest-visible register state as needed
- preserve NZCV
- switch to a private host stack if needed
- call a C dispatcher with guest register state
- restore state
- branch back to guest PC + 4

The island should not contain per-syscall translation logic beyond marshaling control into C.

### Dispatcher responsibilities

Add a C dispatcher that:

- reads the Darwin syscall number from guest `x16`
- reads arguments from guest `x0`, `x1`, `x2`, and so on
- dispatches to a small Linux backend
- writes results back to guest state
- sets Darwin success/failure semantics

Darwin syscall result rules:

- success: carry clear, result in `x0`
- failure: carry set, Darwin errno in `x0`

For unsupported syscalls:

- log the Darwin syscall number
- log guest PC
- log argument registers
- return Darwin `ENOSYS`

## Execution plan

Implement syscall translation in small verified groups. Each group must have
one or more real ARM64 Mach-O fixtures that execute raw Darwin syscalls with
`x16` and `svc #0x80`. Do not count a syscall as translated until its fixture
passes under the ARM64 Linux container.

For every group:

1. download/read the exact Apple reference files needed for that group
2. add or extend real Mach-O assembly fixtures
3. add shell tests that assert observable behavior
4. implement the minimum translation logic for that group
5. build locally
6. run the targeted tests in the ARM64 Docker/QEMU environment
7. fix failures before moving to the next group

After all groups below pass:

1. run the full ARM64 test suite
2. fix every broken test
3. rerun the full suite until it passes

Do not start deferred syscall families until the initial table below is
complete and fully tested.

### Group 0: syscall gate foundation

### Milestone 1: failing Darwin exit fixture

Add a new static ARM64 Mach-O fixture:

- `tests/fixtures/darwin_exit42.s`

Behavior:

- place Darwin `SYS_exit` in `x16`
- place `42` in `x0`
- execute `svc #0x80`
- do not rely on imported libSystem symbols

Add a new test:

- `tests/test_darwin_exit42.sh`

The test should prove that current `machismo` does not yet translate the Darwin syscall path.

Keep existing Linux-syscall fixtures unchanged:

- `tests/fixtures/exit42.s`
- `tests/fixtures/exit0.s`
- `tests/fixtures/hello_write.s`

They remain useful regression coverage for the current execution path.

### Milestone 2: minimal syscall gateway for exit

Add a new module:

- `src/syscall/syscall_gate.c`
- `src/syscall/syscall_gate.h`

Wire it from `src/machismo.c` so the patch pass runs before guest code executes.

Implement only:

- Darwin `exit`

Do not implement:

- `write`
- `read`
- `open`
- `mmap`
- signals
- threads
- Mach traps

For `exit`, it is acceptable for the backend to terminate the host process directly with the translated exit status.

### Milestone 3: Darwin write fixture and backend

Add a new static ARM64 Mach-O fixture:

- `tests/fixtures/darwin_write_stdout.s`

Behavior:

- use Darwin syscall convention
- write a fixed string to stdout
- exit with status 0

Add a new test that checks:

- stdout contents
- zero exit status

Implement only:

- Darwin `write`

### Group 1: descriptor basics

Implement and test:

1. `read`
2. `close`

Fixture behavior:

- read fixed bytes from stdin into guest memory
- write them back to stdout
- close a descriptor and verify the return path succeeds

Expected Darwin references:

- `xnu/bsd/kern/syscalls.master`
- `xnu/bsd/sys/errno.h`
- `xnu/libsyscall/custom/SYS.h`

### Group 2: file open and seek

Implement and test:

1. `open`
2. `lseek`

Fixture behavior:

- open a test data file using Darwin `O_RDONLY`
- seek to a known offset
- read bytes from that offset
- write the bytes to stdout
- close the descriptor

Expected Darwin references:

- `xnu/bsd/kern/syscalls.master`
- `xnu/bsd/sys/fcntl.h`
- `xnu/bsd/sys/errno.h`

### Group 3: file metadata

Implement and test:

1. `fstat`

Fixture behavior:

- open a known test data file
- call `fstat`
- read Darwin-layout `struct stat` fields from guest memory
- verify at least `st_size`
- exit nonzero on mismatch

Expected Darwin references:

- `xnu/bsd/kern/syscalls.master`
- `xnu/bsd/sys/stat.h`
- `xnu/bsd/sys/errno.h`

Reuse or extract existing Darwin stat conversion logic from:

- `src/shim/libsystem_shim.c`

### Group 4: memory mapping

Implement and test:

1. `mmap`
2. `munmap`
3. `mprotect`

Fixture behavior:

- map anonymous writable memory
- write bytes into it
- protect it read-only
- write the bytes to stdout
- unmap it
- exit zero

Expected Darwin references:

- `xnu/bsd/kern/syscalls.master`
- `xnu/bsd/sys/mman.h`
- `xnu/bsd/sys/errno.h`

### Initial table completion target

The initial syscall table is complete only when all of these translations are
implemented and covered by real Mach-O fixtures:

- `exit`
- `write`
- `read`
- `open`
- `close`
- `lseek`
- `fstat`
- `mmap`
- `munmap`
- `mprotect`

Unsupported syscalls must still:

- log the Darwin syscall number
- log guest PC
- log argument registers
- return Darwin `ENOSYS`

### Deferred syscall families

Defer:

- signals
- threads
- kqueue
- sockets
- fork
- execve
- posix_spawn
- Mach messages
- Mach ports

## File-level plan

Expected first-round changes:

- `tests/fixtures/build_fixtures.sh`
- `tests/test_darwin_exit42.sh`
- `src/syscall/syscall_gate.c`
- `src/syscall/syscall_gate.h`
- `src/machismo.c`
- `CMakeLists.txt`

Expected second-round changes for `write`:

- `tests/fixtures/darwin_write_stdout.s`
- new `write` test script
- `src/syscall/syscall_gate.c`

## Reuse guidance

Do not duplicate Darwin/Linux ABI translation code unnecessarily.

When later syscalls need flag or structure conversion, reuse or extract logic from:

- `src/shim/libsystem_shim.c`

This is especially relevant for:

- `O_*` flag translation
- `AT_*` flag translation
- Darwin `struct stat` layout conversion

## Verification plan

For each milestone:

1. build fixtures
2. add a failing test
3. implement the minimum behavior needed to pass that test
4. run the targeted test
5. run the full test suite

Commands:

```sh
bash tests/fixtures/build_fixtures.sh
cmake --build build --verbose -j1
bash tests/run_tests.sh
```

## Reference sources

Use the Apple open-source references listed in:

- `docs/APPLE_OPEN_SOURCE_REFERENCES.md`

Primary syscall ABI sources:

- `apple-oss-distributions/xnu`
- `bsd/kern/syscalls.master`
- `bsd/sys/syscall.h`
- `bsd/sys/errno.h`
- `libsyscall/custom/SYS.h`

When implementing a syscall, record the exact Apple source file used in the commit message or test note.
