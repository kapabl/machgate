# Worker B: SYSCALL-ISLAND-RANGE

## Status

- Bucket: `SYSCALL-ISLAND-RANGE`
- Targets: `k9s`, `tilt`, `vault`, `consul`, `boundary`, `tofu`, `pulumi`
- Attempts used: 2 / 5
- Current result: `RECLASSIFIED`
- Exit condition met: syscall gate patching no longer aborts with island out-of-range for all seven targets.

## Changed Files

- `src/syscall/syscall_gate.c`
- `docs/external-fix-workers/syscall-islands.md`

## Attempt 1: Reproduce

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc '
set -e
mkdir -p tests/external/work/worker-b tests/external/logs/worker-b-attempt-1
awk -F"|" '\''BEGIN{want["k9s"]=1;want["tilt"]=1;want["vault"]=1;want["consul"]=1;want["boundary"]=1;want["tofu"]=1;want["pulumi"]=1} /^#/ || NF < 5 {next} want[$1] {print}'\'' tests/external/worker_manifests/worker-3.txt tests/external/worker_manifests/worker-4.txt > tests/external/work/worker-b/syscall-island-range-manifest.txt
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/work/worker-b/syscall-island-range-manifest.txt MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-b/attempt-1 MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-b-attempt-1 MACHGATE_EXTERNAL_TIMEOUT=20 bash tests/test_external_macho_cli.sh
'
```

Result:

- `k9s`: `FAILED-1`, aborts at `syscall_gate: island at 0x109b12000 too far from 0x1000d7910`
- `tilt`: direct run reaches `SIGSEGV`; qemu-strace rerun aborts at `syscall_gate: island at 0x1088c4000 too far from 0x1000ec1bc`
- `vault`: `FAILED-1`, aborts at `syscall_gate: island at 0x11c89c000 too far from 0x1000a8900`
- `consul`: `FAILED-1`, aborts at `syscall_gate: island at 0x10afc0000 too far from 0x1000d7080`
- `boundary`: `FAILED-1`, aborts at `syscall_gate: island at 0x10dec4000 too far from 0x100083374`
- `tofu`: `FAILED-1`, aborts at `syscall_gate: island at 0x1089d2000 too far from 0x1000a4adc`
- `pulumi`: `FAILED-1`, aborts at `syscall_gate: island at 0x108717000 too far from 0x1000d83cc`

Logs:

- `tests/external/logs/worker-b-attempt-1/k9s.err`
- `tests/external/logs/worker-b-attempt-1/tilt.err`
- `tests/external/logs/worker-b-attempt-1/vault.err`
- `tests/external/logs/worker-b-attempt-1/consul.err`
- `tests/external/logs/worker-b-attempt-1/boundary.err`
- `tests/external/logs/worker-b-attempt-1/tofu.err`
- `tests/external/logs/worker-b-attempt-1/pulumi.err`
- `tests/external/logs/worker-b-attempt-1/*.qemu-strace`

Patch:

- Replaced the single global syscall island cursor with a small registry of island pools.
- Kept the loader-adjacent tail pool as an optional pool.
- Added lazy `MAP_FIXED_NOREPLACE` allocation of extra 256 KiB RWX pools within ARM64 direct-branch range of each `svc #0x80` site.
- Made each patch select a pool whose current island address is reachable from the instruction being patched.

Build:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'cmake --build build-arm64 --target machismo --parallel'
```

Result: `machismo` target rebuilt successfully.

Note: `cmake --build build-arm64 --parallel` is currently blocked by an unrelated `src/shim/libsystem_shim.c` compile error in `CFStringCreateExternalRepresentation` / `CFDataCreate`, outside Worker B scope.

## Attempt 2: Rerun Affected Targets

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc '
set -e
mkdir -p tests/external/logs/worker-b-attempt-2
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/work/worker-b/syscall-island-range-manifest.txt MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-b/attempt-2 MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-b-attempt-2 MACHGATE_EXTERNAL_TIMEOUT=20 bash tests/test_external_macho_cli.sh
'
```

Result: `0/7` probes passed, but all seven moved out of `SYSCALL-ISLAND-RANGE`.

- `k9s`: `RECLASSIFIED`, `syscall_gate: patched 2 Darwin syscalls`, reaches `_main`, then `SIGSEGV si_addr=NULL`
- `tilt`: `RECLASSIFIED`, no syscall gate range abort remains; crashes earlier/later in the startup path with `SIGSEGV si_addr=0x0000000179a548dc` in qemu-strace
- `vault`: `RECLASSIFIED`, `syscall_gate: patched 1 Darwin syscalls`, reaches `_main`, then `SIGSEGV si_addr=NULL`
- `consul`: `RECLASSIFIED`, `syscall_gate: patched 2 Darwin syscalls`, reaches `_main`, then `SIGSEGV si_addr=NULL`
- `boundary`: `RECLASSIFIED`, `syscall_gate: patched 3 Darwin syscalls`, reaches `_main`, then `SIGSEGV si_addr=NULL`
- `tofu`: `RECLASSIFIED`, `syscall_gate: patched 1 Darwin syscalls`, reaches `_main`, then `SIGSEGV si_addr=NULL`
- `pulumi`: `RECLASSIFIED`, `syscall_gate: patched 1 Darwin syscalls`, reaches `_main`, then `SIGSEGV si_addr=NULL`

Verification grep:

```sh
rg -n "syscall_gate:.*(too far|no island|failed|aborting)" tests/external/logs/worker-b-attempt-2 || true
```

Result: no matches.

Logs:

- `tests/external/logs/worker-b-attempt-2/k9s.err`
- `tests/external/logs/worker-b-attempt-2/tilt.err`
- `tests/external/logs/worker-b-attempt-2/vault.err`
- `tests/external/logs/worker-b-attempt-2/consul.err`
- `tests/external/logs/worker-b-attempt-2/boundary.err`
- `tests/external/logs/worker-b-attempt-2/tofu.err`
- `tests/external/logs/worker-b-attempt-2/pulumi.err`
- `tests/external/logs/worker-b-attempt-2/*.qemu-strace`

## Regression Tests

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'bash tests/fixtures/build_fixtures.sh && BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh'
```

Result: `27/27 passed, 0 failed`.

## Next Blocker

Worker B is done for this bucket. The exact next blocker for these targets is runtime startup failure after the syscall island range abort is removed:

- `k9s`, `vault`, `consul`, `boundary`, `tofu`, and `pulumi`: `_main` is entered, then `SIGSEGV si_addr=NULL`.
- `tilt`: `SIGSEGV si_addr=0x0000000179a548dc` in qemu-strace before any remaining syscall-gate range failure appears.

These belong to the Go startup/null-SIGSEGV and shim-surface buckets, not the syscall gateway branch-island range bucket.
