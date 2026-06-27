# MachGate loader contract

MachGate must behave like a loader/runtime bootstrapper before it behaves like a
syscall translator. C++ binaries expose this contract quickly because their
static constructors execute a large pre-main program.

## Required Pre-Main Contract

Before any guest initializer or entrypoint runs, MachGate must:

1. Map every Mach-O segment at its guest virtual address with the correct file
   contents, zero-fill, and final protections.
2. Apply every rebase/fixup needed by the main executable.
3. Load every required Mach-O or native mapped dependency.
4. Apply every rebase/fixup needed by loaded Mach-O dependencies.
5. Resolve bind and lazy-bind targets, or install a correct lazy binder.
6. Apply CPU-compatibility code patches before guest code can execute.
7. Initialize the Darwin runtime surface used by constructors:
   - pthread object signatures
   - libc allocator defaults
   - TLV/TLS metadata
   - exception-frame registration
   - libSystem shim state
8. Run dependency image initializers in dependency order.
9. Run each image's initializer phases in dyld-compatible order:
   - `LC_ROUTINES`
   - `__mod_init_func`
   - `__init_offsets`
10. Enter `_main` or `LC_MAIN` only after all required initializers complete.

If any slot, vtable, callback table, allocator pointer, pthread object, TLV
descriptor, or guard variable is still uninitialized when constructors run, that
is a loader/startup bug. A crash in libc++ or Catch2 is then evidence of the
contract violation, not proof that the crashed object should be repaired in a
signal handler.

## Current Enforcement

- Main executable fixups are resolved before main initializers.
- Loaded Mach-O dylib fixups are resolved before dylib initializers.
- CPU compatibility patches are applied before guest execution.
- Main runtime bootstrap now runs before any Mach-O initializer:
  - Darwin pthread data fixups
  - Darwin libc allocator default slots
  - TLV image metadata setup
  - main executable EH-frame registration
- Signal diagnostics print initializer context and LR/LR-4 instruction windows
  for null-branch crashes during pre-main execution.

## Known Remaining Gaps

- Dylib initializer ordering is still load-order based, not dependency
  bottom-up.
- Dylib initializer calls still use a narrower ABI than dyld's full initializer
  argument contract.
- Lazy binding is mostly pre-resolved; a true dyld lazy binder is not fully
  emulated.
- Main fixup resolution is still coupled to the configured dylib map for bind
  resolution. Rebase-only execution without a map should be audited separately.
- Authenticated/arm64e chained pointer formats are not fully supported.
- Classic section relocation records are not walked.
- EH-frame registration for loaded Mach-O dylibs remains disabled because the
  current `_dl_find_object` interposition tracks a single main image range.

## Debug Rule

For `pc=(nil)` crashes during initializers, inspect `lr - 4` first. If it is an
indirect branch or call, trace how the target register was loaded and identify
the slot. The fix should explain why that slot was not initialized before the
initializer ran.
