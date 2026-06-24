# Host glibc Bypass Failure Class

MachGate has three different call paths that can affect guest-visible behavior:

1. Darwin syscall path: guest Mach-O code executes `svc #0x80`, and MachGate's
   syscall patcher dispatches on the Darwin syscall number in `x16`.
2. Imported libSystem path: guest Mach-O code calls imported Darwin symbols,
   and MachGate resolves those symbols to `libsystem_shim.so`.
3. Host glibc path: code running inside the Linux `machgate` process calls
   glibc or another host library directly.

The third path is the failure class. It is not a separate process or a separate
binary. It is the same Linux process, but the call bypasses both MachGate
translation layers and therefore observes Linux/glibc semantics instead of
Darwin semantics.

## glibc vs dylib Context

glibc is a Linux userspace C library. In the ARM64 Linux container it is an
ELF shared object, usually named something like:

- `libc.so.6`
- `/lib/aarch64-linux-gnu/libc.so.6`
- `/usr/lib64/libc.so.6`

It is not a Mach-O dylib.

Darwin's equivalent surface is `libSystem.dylib`, which is a Mach-O dynamic
library on macOS. Mach-O binaries import symbols such as `_mmap`, `_munmap`,
`_pthread_create`, and `_sigaction` from Darwin libraries. MachGate resolves
many of those imported Mach-O symbols to `libsystem_shim.so`, a Linux ELF
shared object that implements Darwin-shaped behavior.

So the process contains both worlds:

- guest code and guest dylib metadata: Mach-O / Darwin ABI
- loader, shims, and host libraries: Linux ELF / glibc ABI

`LD_PRELOAD` belongs to the Linux ELF dynamic loader. It does not patch Mach-O
dylib loading directly. In Loop M, MachGate re-execs the Linux `machgate`
process with `LD_PRELOAD=libmachgate_vm_interpose.so`, causing glibc-side
`mmap()` and `munmap()` calls in the host process to resolve to MachGate's
interposer first.

That is why the Delta fix is specifically a host/glibc interposer, not a new
Mach-O dylib shim.

## Why This Matters

MachGate intentionally mixes a guest Mach-O image, loader code, shim code, and
host shared libraries inside one Linux process. That means host-side calls can
touch state that is also guest-visible:

- VM mappings and page permissions
- pthread and signal state
- process and `/proc` inspection
- file descriptors, pipes, and child-process state
- malloc/free and allocator-owned mappings
- dynamic loader handles and symbol lookup

If a host call only affects host-private state, glibc behavior is fine. If it
affects guest-visible state or Darwin ABI semantics, it may need translation.

## Delta Evidence

The accepted Loop M Delta fix is the current concrete example.

Hard evidence:

- Pre-fix failing traces repeatedly showed a helper-thread teardown path:
  process inspection, `mmap(NULL, 384)`, then `munmap(addr, 4096)`.
- Failing runs correlated with host/QEMU `munmap(..., 4096)` returning
  `EINTR`, followed by target `SIGSEGV`.
- Passing runs correlated with Darwin-shaped invalid-unmap behavior returning
  `EINVAL`.
- `delta` source explains why `delta --version` reaches this path:
  `.source-context/delta-0.19.2/src/main.rs:65` starts
  `start_determining_calling_process_in_thread()` before version handling.
- `.source-context/delta-0.19.2/src/utils/process.rs:61-79` creates the
  `find_calling_process` helper thread.
- Targeted post-fix gate:
  `tests/external/logs/loop-m-delta-vm-interpose-attempt{1..5}/` passed
  `5 / 5`, each with stdout `delta 0.19.2`.
- Post-fix regression suite: `28 / 28` project tests passed.

Current status boundary:

- Delta is targeted-fixed.
- Delta is not yet promoted by a fresh 57-binary strict full-corpus run.

## How Delta Reaches glibc

Delta does not link against glibc directly. The downloaded Delta artifact is an
ARM64 Mach-O binary and imports Darwin/libSystem symbols.

The bridge to glibc is MachGate itself:

1. Delta starts a helper before version handling:
   `.source-context/delta-0.19.2/src/main.rs:65` calls
   `start_determining_calling_process_in_thread()`.
2. That function creates a Rust thread named `find_calling_process`:
   `.source-context/delta-0.19.2/src/utils/process.rs:61-79`.
3. The helper uses `sysinfo` process inspection:
   `.source-context/delta-0.19.2/src/utils/process.rs:264-276` owns the
   `sysinfo::System` state, and lines `409-419` refresh process data.
4. Delta's Darwin `pthread_create` import resolves to MachGate's
   `libsystem_shim.so`.
5. MachGate's pthread shim calls the real host pthread implementation with
   `dlsym(RTLD_NEXT, "pthread_create")` and then
   `real_pthread_create(...)` in `src/shim/libsystem_shim.c`.
6. That creates a real Linux/glibc pthread inside the same `machgate` process.
   Any glibc-internal VM operations, host-library VM operations, or host-side
   teardown for that thread can call glibc `mmap` / `munmap`.

The exact internal C stack for Delta's tiny `mmap(NULL,384)` /
`munmap(addr,4096)` pair was not captured because direct host `strace` is not
usable in this QEMU-user/binfmt setup. The evidence for the bypass is:

- `MACHGATE_TRACE_SHIM=1` did not show the failing pair entering the shim
  `mmap` / `munmap` wrapper.
- syscall-gate tracing did not identify it as a patched Darwin `svc #0x80`
  syscall.
- QEMU traces still showed the operation and its `EINTR`/`SIGSEGV` failure
  correlation.
- A process-wide `LD_PRELOAD` interposer for host `mmap` / `munmap` fixed the
  targeted Delta gate `5 / 5`.

So the proven claim is not "Delta links glibc". The proven claim is "Delta
causes MachGate to enter host-backed pthread/runtime behavior, and the failing
VM operation bypassed the Darwin syscall and libSystem-shim paths until the
host glibc interposer caught it."

## Accepted Fix Pattern

Loop M added a narrow VM interposer:

- `libmachgate_vm_interpose.so`
- one-time loader re-exec with `LD_PRELOAD`
- `mmap()` and `munmap()` wrappers
- forwarding to exported MachGate VM functions:
  - `machgate_darwin_mmap`
  - `machgate_darwin_munmap`
- preservation of `LD_PRELOAD` across Mach-O guest re-exec
- installed-layout lookup for `../lib/libmachgate_vm_interpose.so`

This catches host/glibc VM calls that bypass imported libSystem symbols and
routes them through MachGate's Darwin-aware VM tracker.

## Diagnostic Signs

Suspect this class when traces show:

- A behavior is not visible in syscall-gate tracing.
- A behavior is not visible in `libsystem_shim` tracing.
- QEMU or host traces still show the operation.
- The operation occurs in a helper thread, callback, unwinder, allocator,
  process-inspection library, or host-mapped runtime.
- The syscall result has Linux semantics where Darwin-shaped behavior is
  expected.

For Delta, the key discriminator was:

- Expected Darwin-shaped path: invalid oversized unmap returns `EINVAL`.
- Bypass path: host/QEMU path returned `EINTR` and the helper thread crashed.

## Fix Rules

Do not interpose broad host APIs by default.

Use this escalation order:

1. Prove the bypass with logs or traces.
2. Identify whether the state is guest-visible.
3. Prefer fixing the existing Darwin syscall or libSystem shim path if the call
   can be routed there.
4. Add a host interposer only when the call demonstrably bypasses both existing
   translation layers.
5. Keep the interposer narrow and state-specific.
6. Add a targeted repeat gate for the failing binary.
7. Run the normal project tests.
8. Promote only after the full external corpus is rerun, if the doc claims
   strict full-corpus status.

## APIs Most Likely To Hit This Class

High risk:

- `mmap`, `munmap`, `mprotect`
- `pthread_create`, `pthread_*`
- `sigaltstack`, `sigaction`, signal masks
- `fork`, `execve`, `wait*`, pipe and fd handoff
- `readlink`, `readlinkat`, `/proc` inspection
- `malloc`, `free`, allocator internals
- `dlopen`, `dlsym`, special dynamic-loader handles

Lower risk:

- Pure host-private helper allocations that never escape to guest-visible state
- Host-only logging
- Host-only build/test harness calls

## Documentation Requirement

When this class is found, update the relevant live status doc with:

- binary name and command
- exact bypass evidence
- source-code explanation if available
- logs used as proof
- accepted or rejected fix
- targeted repeat count
- project-test result
- full-corpus promotion status
