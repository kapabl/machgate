# Public implementation references for MachGate

## Purpose

MachGate uses Apple open-source repositories, LLVM/LLD, public ABI documents,
and behavior tests as independent semantic references for guest ABI
translation. The sibling reference checkout is for provenance and deletion
decisions only; it is not an implementation source.

Do not vendor large source trees or copy implementation text. Record the exact
upstream file and revision used to establish a number, layout, flag, calling
convention, or observable behavior.

Use the references below to derive:

- Darwin syscall numbers and calling convention;
- Darwin errno, flag, and structure layouts;
- guest-visible filesystem and directory behavior;
- Mach trap behavior;
- pthread and thread-local behavior;
- Mach-O thread-local-variable metadata and descriptor behavior; and
- explicitly required loader and runtime behavior.

## Primary references

### Apple open-source distribution index

[distribution-macOS](https://github.com/apple-oss-distributions/distribution-macOS)
indexes the Apple open-source components published for macOS releases. Pin the
component revision used for a contract decision rather than relying on the
index's moving default branch.

### XNU

[XNU](https://github.com/apple-oss-distributions/xnu) is the primary source for
the guest kernel ABI. Relevant files include:

- [`bsd/kern/syscalls.master`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/syscalls.master)
  and [`bsd/sys/syscall.h`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/syscall.h)
  for syscall numbers and prototypes;
- [`bsd/sys/errno.h`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/errno.h)
  for guest errno values;
- [`bsd/sys/stat.h`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/stat.h)
  for guest `stat` layouts and flags;
- [`bsd/sys/dirent.h`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/dirent.h)
  for guest directory-entry layouts and record rules;
- [`bsd/sys/mount.h`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/mount.h)
  for guest filesystem-statistics and mount-facing layouts;
- [`bsd/vfs/vfs_syscalls.c`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/vfs/vfs_syscalls.c)
  for observable VFS syscall semantics;
- [`bsd/sys/fcntl.h`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/fcntl.h),
  [`bsd/sys/mman.h`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/mman.h),
  and [`bsd/sys/event.h`](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/event.h)
  for guest flags and public structures; and
- [`osfmk/mach/arm/syscall_sw.h`](https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/mach/arm/syscall_sw.h)
  and [`libsyscall/custom/SYS.h`](https://github.com/apple-oss-distributions/xnu/blob/main/libsyscall/custom/SYS.h)
  for the ARM64 syscall boundary.

The process-local guest semaphore adapter and exported NDR descriptor are
derived from XNU revision `f6217f891ac0bb64f3d375211650a4c1ff8ca1ea`:

- [`osfmk/mach/semaphore.h`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/osfmk/mach/semaphore.h)
  defines the guest function contracts;
- [`osfmk/mach/kern_return.h`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/osfmk/mach/kern_return.h)
  defines the guest result values; and
- [`osfmk/mach/ndr.h`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/osfmk/mach/ndr.h)
  together with
  [`osfmk/mach/arm/ndr_def.h`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/osfmk/mach/arm/ndr_def.h)
  defines the exact eight-byte ARM NDR record.

`mach_sync_compat.c` implements semaphore handles as process-local adapter
identities. It does not implement Mach ports, messages, or cross-process IPC.

The focused task-policy adapter uses the same pinned XNU revision:

- [`osfmk/mach/task_policy.h`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/osfmk/mach/task_policy.h)
  defines `task_policy_set`, `TASK_CATEGORY_POLICY`, the one-word category
  policy count, and `TASK_DEFAULT_APPLICATION`; and
- [`osfmk/mach/kern_return.h`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/osfmk/mach/kern_return.h)
  defines the explicit invalid-argument and unsupported results.

MachGate validates the observed current-task category request and returns
`KERN_NOT_SUPPORTED`. It does not claim policy support or perform Mach IPC.

Darwin ARM64 syscall wrappers place the syscall number in `x16`, arguments in
`x0`, `x1`, and subsequent registers, execute `svc #0x80`, and report failure
with carry set. Treat XNU structures as the guest contract and Linux kernel
headers as the host contract; convert between them explicitly.

The current `filesystem_metadata_compat.c`, `file_abi.c`, and
`file_operations_compat.c` module boundaries cover the extracted filesystem
metadata and File Operations capabilities. Their existence does not imply that
every XNU VFS operation is supported.

### Libc

[Libc](https://github.com/apple-oss-distributions/Libc) is a semantic reference
for libc syscall wrappers, errno behavior, POSIX compatibility behavior, file
and path behavior, stdio and process behavior, and guest-visible structure
expectations.

### libpthread

[libpthread](https://github.com/apple-oss-distributions/libpthread) defines the
guest pthread contract. The current contract review is pinned to revision
`42d026df5b07825070f60134b980a1ec2552dfee`. Use its
[`pthread.h`](https://github.com/apple-oss-distributions/libpthread/blob/42d026df5b07825070f60134b980a1ec2552dfee/include/pthread/pthread.h)
for public types and declarations, and consult the focused sources for observed
semantics:

- [`pthread_mutex.c`](https://github.com/apple-oss-distributions/libpthread/blob/42d026df5b07825070f60134b980a1ec2552dfee/src/pthread_mutex.c);
- [`pthread_cond.c`](https://github.com/apple-oss-distributions/libpthread/blob/42d026df5b07825070f60134b980a1ec2552dfee/src/pthread_cond.c);
- [`pthread_rwlock.c`](https://github.com/apple-oss-distributions/libpthread/blob/42d026df5b07825070f60134b980a1ec2552dfee/src/pthread_rwlock.c); and
- [`pthread_tsd.c`](https://github.com/apple-oss-distributions/libpthread/blob/42d026df5b07825070f60134b980a1ec2552dfee/src/pthread_tsd.c).

Use these files to establish guest opaque-object layouts, initializer
signatures, return values, and synchronization behavior without copying their
implementation. The synchronization module boundary is complete:
`pthread_sync_compat.c` contains mutex, condition-variable, and read-write-lock
adapters. Pthread identity, thread-specific data, once, attributes, lifecycle,
and extensions still require further separation from `libsystem_shim.c`.

### libdispatch

[libdispatch](https://github.com/apple-oss-distributions/libdispatch) is the
public implementation reference for explicitly supported dispatch object,
queue, semaphore, once, time, and data behavior. `dispatch_compat.c` is now a
dedicated compiled capability module, but it remains a focused compatibility
surface rather than a claim of complete libdispatch support.

### libplatform

[libplatform](https://github.com/apple-oss-distributions/libplatform) is a
reference for low-level platform runtime behavior, locks, atomics, and
platform-specific runtime support when a fixture-backed requirement reaches
that boundary.

### Libnotify

[Libnotify](https://github.com/apple-oss-distributions/Libnotify) defines the
guest notification API contract. The fail-closed notification adapters use
[`notify.h`](https://github.com/apple-oss-distributions/Libnotify/blob/715d461778f6b93c821d99390a0078bd6f6d8c04/notify.h)
at revision `715d461778f6b93c821d99390a0078bd6f6d8c04` for declarations and status
values. They do not report registration success because MachGate does not own
a notification service.

### dyld and Libsystem

[dyld](https://github.com/apple-oss-distributions/dyld) is a reference for
fixture-backed requirements involving dynamic Mach-O loading, dyld ABI
behavior, chained fixups, and libSystem startup assumptions. The focused loader
does not claim dyld compatibility.

[Libsystem](https://github.com/apple-oss-distributions/Libsystem) is the
umbrella reference for libSystem composition, not a requirement to reproduce
the complete framework surface.

## LLVM and LLD Mach-O TLV references

LLVM and LLD provide independent, public evidence for the Mach-O thread-local
variable format consumed by `thread_local_compat.c`:

- [`llvm/include/llvm/BinaryFormat/MachO.h`](https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/BinaryFormat/MachO.h)
  defines the Mach-O thread-local section-type constants;
- [`llvm/lib/CodeGen/AsmPrinter/AsmPrinter.cpp`](https://github.com/llvm/llvm-project/blob/main/llvm/lib/CodeGen/AsmPrinter/AsmPrinter.cpp)
  records the emitted TLV descriptor shape and initializer association;
- [`llvm/lib/MC/MCParser/DarwinAsmParser.cpp`](https://github.com/llvm/llvm-project/blob/main/llvm/lib/MC/MCParser/DarwinAsmParser.cpp)
  records the Darwin assembly directives for thread data, zero-fill storage,
  descriptors, pointers, and initialization functions; and
- [`lld/MachO/SyntheticSections.cpp`](https://github.com/llvm/llvm-project/blob/main/lld/MachO/SyntheticSections.cpp)
  records LLD's Mach-O TLV descriptor and pointer-section treatment.

Use LLVM/LLD as a format and observable-behavior reference, then validate the
MachGate adapter with local fixtures. Thread-local/TLV extraction into
`thread_local_compat.c` is complete as a module boundary; that status does not
claim exhaustive Mach-O TLV compatibility.

LLVM's
[`libunwind/include/unwind.h`](https://github.com/llvm/llvm-project/blob/6cff1c56d1678e3d7eb70650d1fe138eac80c05e/libunwind/include/unwind.h)
at revision `6cff1c56d1678e3d7eb70650d1fe138eac80c05e` is the public
interface contract for Compatibility Runtime's standard `_Unwind_*` adapter
surface.

## ARM64 variadic ABI references

The Compatibility Runtime variadic adapters are derived from LLVM revision
`6cff1c56d1678e3d7eb70650d1fe138eac80c05e` and the public Arm procedure-call
standard:

- [`llvm/lib/Target/AArch64/AArch64CallingConvention.td`](https://github.com/llvm/llvm-project/blob/6cff1c56d1678e3d7eb70650d1fe138eac80c05e/llvm/lib/Target/AArch64/AArch64CallingConvention.td)
  defines the Darwin PCS rule that places anonymous variadic arguments in
  eight-byte stack slots;
- [`clang/lib/Basic/Targets/AArch64.cpp`](https://github.com/llvm/llvm-project/blob/6cff1c56d1678e3d7eb70650d1fe138eac80c05e/clang/lib/Basic/Targets/AArch64.cpp)
  defines Darwin AArch64 `va_list` as a character pointer; and
- [AAPCS64](https://github.com/ARM-software/abi-aa/blob/2025Q1/aapcs64/aapcs64.rst)
  defines the Linux-side argument-list state and permits an all-stack argument
  list with unused register-save areas.

`variadic_abi_compat.c` constructs host argument-list state from the guest
stack or guest character-pointer list. It does not pass a Darwin `va_list`
directly to a Linux variadic implementation. `file_operations_abi_entry.S`
applies the same stack rule to the variadic `fcntl` and `ioctl` entry points
before calling their fixed-signature translation adapters.

### CoreFoundation and Security

[CoreFoundation](https://github.com/apple-oss-distributions/CF) defines the
guest-facing CoreFoundation types and ownership rules used by the focused
compatibility surface. The current review is pinned to revision
`dc54c6bb1c1e5e0b9486c1d26dd5bef110b20bf3`:

- [`CFBase.h`](https://github.com/apple-oss-distributions/CF/blob/dc54c6bb1c1e5e0b9486c1d26dd5bef110b20bf3/CFBase.h);
- [`CFString.h`](https://github.com/apple-oss-distributions/CF/blob/dc54c6bb1c1e5e0b9486c1d26dd5bef110b20bf3/CFString.h);
- [`CFNumber.h`](https://github.com/apple-oss-distributions/CF/blob/dc54c6bb1c1e5e0b9486c1d26dd5bef110b20bf3/CFNumber.h)
  for the distinct Boolean singleton contracts;
- [`CFRunLoop.h`](https://github.com/apple-oss-distributions/CF/blob/dc54c6bb1c1e5e0b9486c1d26dd5bef110b20bf3/CFRunLoop.h)
  for the default-mode string constant, without implying run-loop support; and
- [`CFError.h`](https://github.com/apple-oss-distributions/CF/blob/dc54c6bb1c1e5e0b9486c1d26dd5bef110b20bf3/CFError.h)
  for error-domain, creation, and code-query contracts.

[Security](https://github.com/apple-oss-distributions/Security) defines the
guest-facing Security contracts used by the focused random-byte and trust
settings adapters. The current review is pinned to revision
`db15acbe6a7f257a859ad9a3bb86097bfe0679d9`:

- [`base/SecRandom.h`](https://github.com/apple-oss-distributions/Security/blob/db15acbe6a7f257a859ad9a3bb86097bfe0679d9/base/SecRandom.h); and
- [`trust/headers/SecTrustSettings.h`](https://github.com/apple-oss-distributions/Security/blob/db15acbe6a7f257a859ad9a3bb86097bfe0679d9/trust/headers/SecTrustSettings.h).

These references define narrow guest contracts, not an intent to reproduce the
full CoreFoundation or Security frameworks.

## Reference rule

Implement and validate one coherent capability family at a time. Record the
exact public source file and pinned revision used to derive each number,
structure, flag, or semantic choice in the commit message or test note.

Apple sources define the guest-side contract. LLVM/LLD sources define the
relevant object-format and toolchain behavior. Linux manual pages and kernel
headers define the host-side contract. Keep every conversion explicit when two
ABIs use similar names but different layouts or values.

## Recorded ABI facts

The following public guest constants are currently used to reject unsupported
Mach operations without reporting false success. They were verified at XNU
revision `f6217f891ac0bb64f3d375211650a4c1ff8ca1ea`:

| Fact | Value | Public source |
| --- | ---: | --- |
| `KERN_NOT_SUPPORTED` | 46 | `osfmk/mach/kern_return.h` |
| `KERN_RESOURCE_SHORTAGE` | 6 | `osfmk/mach/kern_return.h` |
| `KERN_INVALID_ARGUMENT` | 4 | `osfmk/mach/kern_return.h` |
| `KERN_ABORTED` | 14 | `osfmk/mach/kern_return.h` |
| `KERN_OPERATION_TIMED_OUT` | 49 | `osfmk/mach/kern_return.h` |
| `MACH_SEND_MSG` | `0x00000001` | `osfmk/mach/message.h` |
| `MACH_RCV_MSG` | `0x00000002` | `osfmk/mach/message.h` |
| `MACH_SEND_INVALID_DEST` | `0x10000003` | `osfmk/mach/message.h` |
| `MACH_RCV_INVALID_NAME` | `0x10004002` | `osfmk/mach/message.h` |
| `TASK_CATEGORY_POLICY` | 1 | `osfmk/mach/task_policy.h` |
| `TASK_DEFAULT_APPLICATION` | 7 | `osfmk/mach/task_policy.h` |
| `TASK_CATEGORY_POLICY_COUNT` | 1 | `osfmk/mach/task_policy.h` |
| `FIONBIO` | `0x8004667e` | `bsd/sys/filio.h` |
