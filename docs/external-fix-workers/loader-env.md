# Worker E Loader And Environment Retry

## Scope

Owned buckets:

- `LOADER-MMAP-RANGE`: `nomad`
- `ENV-BLOCKED`: `ninja`, `cmake`, `sqlite3`, `duckdb`, `node`, `bun`, `protoc`, `nvim`

Owned source/write scope used:

- `src/loader.c`
- `src/dylib_loader.c`
- `docs/external-fix-workers/loader-env.md`

No canonical manifest edits were made.

## Status

| Bucket | Attempts | Status | Result |
| --- | ---: | --- | --- |
| `LOADER-MMAP-RANGE` | 2 / 5 | `RECLASSIFIED` | `nomad` no longer fails initial loader reservation; it reaches resolver, syscall patching, and `_main`, then joins the Go/CoreFoundation/null-segv style bucket. |
| `ENV-BLOCKED` | 2 / 5 | `FIXED` | Worker 5 was rerun in ARM64 Docker and converted from host environment failures to real pass/fail data: 1 pass, 7 failures. |

## Attempts

### Attempt 1

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail
cmake --build build-arm64 --parallel
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/worker_manifests/worker-4.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-e-nomad-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-e-nomad-attempt1 MACHGATE_EXTERNAL_TIMEOUT=20 bash tests/test_external_macho_cli.sh'
```

Result:

- Reproduced `nomad` in valid ARM64 Docker.
- `tests/external/logs/worker-e-nomad-attempt1/nomad.err`: `Cannot mmap anonymous memory range: Cannot allocate memory`
- `tests/external/logs/worker-e-nomad-attempt1/nomad.qemu-strace`: `mmap(0x0000000100000000,2097152,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0) = -1 errno=12`

Diagnosis:

- `nomad` has a trailing `__DWARF` `LC_SEGMENT_64` with `vmaddr=0`, `vmsize=0`, and file-backed debug sections.
- The executable loader was including zero-sized non-`__PAGEZERO` segments in the VM reservation span calculation.
- Concurrent edits already changed the span calculation to track the maximum segment end and capped file-backed mappings to `min(filesize, vmsize)`. The remaining loader defect was that zero-sized debug segments still participated in pass 1.

### Attempt 2

Patch:

- `src/loader.c`: skip zero-sized segments when computing the executable segment reservation.
- `src/dylib_loader.c`: apply the same zero-sized segment skip to dylib reservation span calculation.

Build notes:

- Full `cmake --build build-arm64 --parallel` is currently blocked outside Worker E scope by `src/shim/libsystem_shim.c`:
  - conflicting type for `CFDataCreate`
  - prior implicit declaration from `CFStringCreateExternalRepresentation`
- Loader verification used:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail
cmake --build build-arm64 --target machismo --parallel
{ sed -n "1,4p" tests/external/worker_manifests/worker-4.txt; grep "^nomad|" tests/external/worker_manifests/worker-4.txt; } > /tmp/worker-e-nomad-only.txt
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-e-nomad-only.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-e-nomad-attempt2 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-e-nomad-attempt2 MACHGATE_EXTERNAL_TIMEOUT=20 bash tests/test_external_macho_cli.sh'
```

Result:

- This temp manifest included `terraform` plus `nomad`; only the `nomad` row is relevant to Worker E.
- `nomad` reclassified from loader mmap failure to runtime failure.
- `tests/external/logs/worker-e-nomad-attempt2/nomad.qemu-strace` shows:
  - `mmap(0x0000000100000000,157990912,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0) = 0x0000000100000000`
  - segment mappings for `__TEXT`, `__DATA_CONST`, `__DATA`, and `__LINKEDIT`
  - resolver loads `CoreFoundation` and `libSystem.B.dylib`
  - `resolver: done — 172 binds resolved, 0 stubbed, 47 failed, 558805 rebases, 0 ctor/dtor ABI adapters, 2 variadic thunks`
  - `syscall_gate: patched 1 Darwin syscalls`
  - `machismo: calling _main at 0x10008be60 (argc=2)`
  - final failure: `SIGSEGV si_addr=NULL`

Classification:

- `LOADER-MMAP-RANGE` is `RECLASSIFIED`.
- The next blocker for `nomad` is no longer loader range mapping. It is the same later startup/shim class as large Go-style binaries: missing CoreFoundation symbols plus null segv after `_main`.

### Worker 5 Environment Retest

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail
cmake --build build-arm64 --target machismo --parallel
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/worker_manifests/worker-5.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-e-worker5-attempt2 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-e-worker5-attempt2 MACHGATE_EXTERNAL_TIMEOUT=30 bash tests/test_external_macho_cli.sh'
```

Result:

| Target | Status | Evidence |
| --- | --- | --- |
| `ninja` | PASS | Prints `1.13.2`; reaches `_main` and exits 0. |
| `cmake` | FAIL 139 | Runs 326 static constructors, then `SIGSEGV si_addr=NULL`. |
| `sqlite3` | FAIL 1 | Reaches `_main`; unresolved malloc-zone/fsctl/gethostuuid symbols; prints `Error: out of memory`. |
| `duckdb` | FAIL 139 | Reaches `_main`; large EH frame/trampoline setup; segfaults during/after runtime startup. |
| `node` | FAIL 124 | Times out at 30s; logs show many LSE island range warnings and missing CoreFoundation symbols. |
| `bun` | FAIL 139 | Runs one C++ static initializer, then `SIGSEGV si_addr=NULL`; missing CommonCrypto/Block symbols. |
| `protoc` | FAIL 139 | Runs 5 static constructors, then `SIGSEGV si_addr=0x10`. |
| `nvim` | FAIL 139 | Crashes during EH-frame registration path with `SIGSEGV si_addr=0x00000001cb08c8dc`; missing Mach/semaphore/spawn symbols. |

Summary:

- Worker 5 is no longer `ENV-BLOCKED`.
- Converted rows: 1 pass, 7 real MachGate failures.
- Logs: `tests/external/logs/worker-e-worker5-attempt2/`
- Work dir: `tests/external/work/worker-e-worker5-attempt2/`

## Changed Files

- `src/loader.c`
- `src/dylib_loader.c`
- `docs/external-fix-workers/loader-env.md`

Observed concurrent changes in the same owned area and preserved them:

- `src/loader.h`: pool padding changed from 2 MiB to 4 MiB.
- `src/loader.c`: segment span changed to max segment end; file-backed segment mapping capped to `min(filesize, vmsize)`; zero-sized segment handling added in mapping pass.
- `src/dylib_loader.c`: zero-sized segment handling added in mapping pass; file-backed mapping capped to `min(filesize, vmsize)`.

## Next Blocker

Worker E exit criteria are met:

- `nomad` reaches resolver and `_main`, so it is no longer a loader mmap/range failure.
- Worker 5 rows have valid ARM64 Docker pass/fail data.

Remaining blockers are outside Worker E scope:

- Full rebuild is blocked by `src/shim/libsystem_shim.c` compile errors in current workspace state.
- `nomad`, `cmake`, `bun`, and several Worker 5 failures need shim/startup work.
- `node` still shows large-binary LSE island range warnings.
