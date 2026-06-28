# Allocator Contract Findings

This file records the actionable findings from the parallel allocator/runtime
ownership loop.

## Confirmed gaps fixed in this pass

- Direct alignment allocation symbols were missing as first-class exports:
  `memalign`, `aligned_alloc`, and `valloc`.
- Flat Mach-O lookup could bind allocator-like symbols through host
  `RTLD_DEFAULT` instead of the mapped libSystem shim.
- Deferred C++ binds did not route through the MachGate operator new/delete
  hooks.
- Several Darwin malloc-zone APIs existed only as private callback helpers and
  were not part of the explicit versioned export surface.
- There was no focused test that failed when allocator ABI exports disappeared.

## Remaining risk areas

- Native mapped `libc++.so.1` and `libc++abi.so.1` can still perform internal
  ELF allocations. The current fix improves symbol availability and Mach-O
  binding, but it does not rebuild libc++ against a shim-only allocator.
- The malloc-zone ABI is intentionally minimal. It covers default-zone queries
  and allocation callbacks, not the full Darwin introspection API.
- CoreFoundation/Foundation allocator surfaces are still limited because the
  current target is command-line binaries with Foundation skipped by policy.

## Verification

- `BUILD_DIR=/home/kapablanka/repos/machgate/build bash tests/test_libsystem_shim.sh`
- `BUILD_DIR=/home/kapablanka/repos/machgate/build bash tests/test_allocator_export_surface.sh`
- ARM64 Docker:
  `BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh`

The ARM64 Docker suite passed `30 / 30`.
