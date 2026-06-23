# Worker A: LSE Islands

## Attempt 1

Status: `RECLASSIFIED`

Worker A exit condition is met: `fx`, `minikube`, `kubeseal`, and `nu` all reach resolver and/or `_main` without LSE island pool crashes or LSE pool exhaustion.

Changed files:

- `src/loader.c`
- `src/loader.h`
- `src/dylib_loader.c`
- `src/machismo.c`
- `docs/external-fix-workers/lse-islands.md`

Patch summary:

- Main executable loader now computes the reserved VM span from the maximum segment end instead of the last segment command visited.
- Main and dylib loaders skip zero-VM-size segments, avoiding file-backed `__DWARF` mappings over reserved branch-island pool pages.
- File-backed segment mappings are capped to `vmsize`.
- Main executable adjacent pool increased from 2 MB to 4 MB.
- Main LSE island pool increased from 512 KB to 2 MB.

Rebuild:

- Command: `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build-arm64 --parallel'`
- Result: success.

Affected-target rerun:

- Command: `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail; tmp=/tmp/worker-a-lse.txt; : > "$tmp"; awk -F"|" "/^(nu)\\|/ {print}" tests/external/worker_manifests/worker-1.txt >> "$tmp"; awk -F"|" "/^(fx)\\|/ {print}" tests/external/worker_manifests/worker-2.txt >> "$tmp"; awk -F"|" "/^(minikube|kubeseal)\\|/ {print}" tests/external/worker_manifests/worker-3.txt >> "$tmp"; MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST="$tmp" MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-a-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-a-attempt1 MACHGATE_EXTERNAL_TIMEOUT=20 bash tests/test_external_macho_cli.sh'`
- Harness result: `0/4 external Mach-O CLI probes passed, 4 failed`.
- Worker A result: all four are reclassified past LSE.

Target evidence:

| Target | Status | Evidence | Next bucket |
| --- | --- | --- | --- |
| `nu` | `RECLASSIFIED` | `tests/external/logs/worker-a-attempt1/nu.err`: `machismo: patched 20302 ARMv8.1 LSE atomics with LDXR/STXR islands`, `resolver: done`, `machismo: calling _main at 0x100339e70`; no `isa_emul: island pool exhausted` | Missing CoreFoundation/CommonCrypto shim surface plus NULL `_main` crash |
| `fx` | `RECLASSIFIED` | `tests/external/logs/worker-a-attempt1/fx.err`: `machismo: patched 726 ARMv8.1 LSE atomics with LDXR/STXR islands`, `resolver: done`, `syscall_gate: patched 1 Darwin syscalls`, `machismo: calling _main at 0x100078200`; QEMU SIGSEGV is `si_addr=NULL` | Missing process-region shims plus NULL `_main` crash |
| `minikube` | `RECLASSIFIED` | `tests/external/logs/worker-a-attempt1/minikube.err`: `machismo: patched 3625 ARMv8.1 LSE atomics with LDXR/STXR islands`, `resolver: done`, `syscall_gate: patched 2 Darwin syscalls`, `machismo: calling _main at 0x100083fe0`; QEMU SIGSEGV is `si_addr=NULL` | Missing CoreFoundation/process-region shims plus NULL `_main` crash |
| `kubeseal` | `RECLASSIFIED` | `tests/external/logs/worker-a-attempt1/kubeseal.err`: `machismo: patched 1380 ARMv8.1 LSE atomics with LDXR/STXR islands`, `resolver: done`, `machismo: calling _main at 0x100085a10`; QEMU SIGSEGV is `si_addr=NULL` | Missing CoreFoundation shims plus NULL `_main` crash |

Attempt conclusion:

- `LSE-ISLAND-CRASH` is fixed for `fx`, `minikube`, and `kubeseal`.
- `LSE-POOL-EXHAUSTED` is fixed for `nu`.
- Remaining failures are later startup/shim-surface crashes outside Worker A ownership.

Verification:

- `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh'`
- Result: `27/27 passed, 0 failed`.
- Note: running `tests/run_tests.sh` without `BUILD_DIR=/work/build-arm64` in this Docker mount used stale `/work/build` CMake cache metadata from `/home/kapablanka/repos/machgate/build` and failed before executing meaningful test binaries.
