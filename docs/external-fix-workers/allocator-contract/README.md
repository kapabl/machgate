# Allocator Runtime Contract Audit

Purpose: close the private C++ `Core.UnitTest` post-`_main` allocator failure generically, not one crash at a time.

Current private failure shape:

- all `707 / 707` static constructors complete
- MachGate reaches `_main`
- guest aborts in `Memory.cpp(884) trackDeallocate`
- message: `rest=0, size=72`
- `MACHGATE_TRACE_ALLOC_SIZE=72` on `v0.3.26` did not show a matching shim allocation event before the abort

Generic contract target:

All allocation/free paths reachable from Mach-O guest code, mapped Apple-ABI `libc++.so.1`, mapped `libc++abi.so.1`, `libsystem_shim.so`, Darwin malloc zones, C++ operator new/delete, and Darwin `_libc_default_*` callback tables must enter one MachGate ownership model.

Running external audits:

- Grok: broad allocator/runtime contract audit with parallel hypotheses
- Claude: source-level codebase audit and missing tests
- Agy: independent Google-agent audit of the same contract

Reports are written next to this file and under `/tmp/machgate-agent-reports/allocator-contract/`.

## Applied result

The first completed implementation pass fixed the generic allocator ABI gaps
identified by the audits:

- direct `memalign`, `aligned_alloc`, and `valloc` now enter the shim ledger
- flat Mach-O allocator lookup prefers mapped libSystem instead of host
  `RTLD_DEFAULT`
- deferred C++ operator binds use the same MachGate allocation hooks as normal
  binds
- Darwin malloc-zone query and callback symbols are exported explicitly
- `tests/test_allocator_export_surface.sh` verifies the versioned export
  surface
- `tests/test_libsystem_shim.sh` now exercises malloc-zone allocation and query
  paths

Verification completed:

- native shim tests pass
- native allocator export test passes
- ARM64 Docker suite passes `30 / 30`

Next external validation target: private `Core.UnitTest` with the next release
tarball.
