# Executable Imaging and Guest Execution

This bounded context turns a validated ARM64 Mach-O file into a process image
that can enter guest code on an ARM64 Linux host. It does not decide native
library policy and does not implement graphical compatibility.

## Glossary

**File slice**
: The thin ARM64 image selected directly or from a universal file. All file
  offsets are interpreted relative to this slice.

**Mapped segment**
: A segment with nonzero virtual size and initial protection. Its file bytes
  are copied into guest virtual memory and the remaining virtual range is
  zero-filled.

**File-only segment**
: A zero-virtual-size, zero-protection segment whose bytes are retained for
  offline consumers such as debuggers. It is validated against the file slice
  but does not participate in virtual-memory sizing.

**Guest instruction adaptation**
: A narrowly defined rewrite of a guest instruction whose Darwin meaning needs
an explicit Linux-host adapter. Adaptations operate only on declared executable
sections before final memory protection.

**Darwin thread context**
: Isolated single-thread storage returned by adapted `TPIDRRO_EL0` reads. It is
separate from Linux `TPIDR_EL0`, so guest pthread/TSD accesses cannot overwrite
the host runtime's thread-control block.

**Process launch context**
: The initial stack pointer and the contiguous argument, environment, and
  apple vectors presented to an `LC_MAIN` entry point.

**Initializer offset table**
: An `S_INIT_FUNC_OFFSETS` section whose 32-bit entries locate guest static
  initializers relative to the mapped Mach-O header. It is the offset-based
  counterpart of an `S_MOD_INIT_FUNC_POINTERS` table.

**Apple vector**
: The null-terminated vector following the environment vector. MachGate
  supplies `executable_path=<absolute path>` because Darwin runtimes use it to
  identify the launched image.

**TLV resolver entry contract**
: The Darwin thread-local-variable resolver convention. The resolver returns
  the variable address in `x0` while preserving `x1` through `x18` and the
  guest link register.

**TLV storage image**
: The per-thread allocation described by initialized and zero-fill Mach-O TLV
  sections. Alignment padding between those sections belongs to the storage
  image but has no initialized bytes to copy.

## Invariants

- File bounds and virtual-memory bounds are validated separately.
- A file-only segment is never mapped merely because it contains bytes.
- Instruction adaptations preserve the destination register encoded by the
  guest instruction and do not expose writable host thread-control storage.
- The argument vector is immediately followed by its null terminator, the
  environment vector and terminator, then the apple vector and terminator.
- The initial stack is 16-byte aligned and begins with `argc` followed by the
  same argument pointers passed in registers.
- Static initializer entries resolve inside the mapped guest image and execute
  on the guest stack before the main entry point.
- TLV implementation helpers may use the host C ABI, but the public resolver
  entry restores the stronger Darwin register contract before returning.
- TLV allocation copies only the initialized section and zero-fills both its
  alignment padding and the declared zero-fill section.

## Layering

The domain objects are the guest image and process launch context. The loader
is the executable-imaging application service. Instruction adaptation is a
separate domain service invoked after mapping and before final protection.
Guest-entry assembly and the TLV resolver entry are infrastructure adapters at
the Darwin-to-Linux ABI boundary.
