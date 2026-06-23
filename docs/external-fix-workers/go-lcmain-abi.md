# Worker F: Go LC_MAIN ABI

Scope: Darwin LC_MAIN startup ABI for `fzf`, `gum`, `shfmt`, and original
`yq`.

## Current status

- STOPPED after Attempt 4 because all four Go LC_MAIN targets reclassified
  with stronger evidence.
- No target passes yet.
- The launch ABI mismatch is fixed or ruled out for these targets: LC_MAIN now
  enters on the guest stack, and traced vectors prove `argv`, `envp`, and
  `apple` are contiguous in the Darwin layout Go expects.
- The remaining crash is after valid LC_MAIN setup, still at each target's
  shared Go runtime instruction `0x3980001b` with `x0=NULL`.
- Working hypothesis: Go's Darwin ARM64 startup receives `argc` and `argv` in
  registers, then walks `argv`, `envp`, and `apple` as one contiguous Darwin
  process vector. MachGate built that vector on the guest stack but still passed
  the original host `argv`/`envp` arrays to LC_MAIN.

## Attempt 1: reproduce current Go LC_MAIN crash

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc '
set -u
cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-arm64 --target machismo --parallel
manifest=/tmp/worker-f-go-lcmain.txt
: > "$manifest"
awk -F"|" "/^(fzf|gum|shfmt)\|/ {print}" tests/external/worker_manifests/worker-2.txt >> "$manifest"
awk -F"|" "/^yq\|/ {print}" tests/external/arm64_macho_cli_manifest.txt >> "$manifest"
MACHGATE_RUN_EXTERNAL=1 MACHGATE_TRACE_SIGNALS=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST="$manifest" MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-f-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-f-attempt1 MACHGATE_EXTERNAL_TIMEOUT=12 bash tests/test_external_macho_cli.sh
'
```

Result: FAILED-1, 0/4 passed.

Evidence:

- Logs: `tests/external/logs/worker-f-attempt1/`
- `fzf`: `machismo: calling _main at 0x100081810`, then PC
  `0x100066420`, instruction `0x3980001b`, `x0=NULL`.
- `gum`: `machismo: calling _main at 0x10007b260`, then PC
  `0x10005f1e0`, instruction `0x3980001b`, `x0=NULL`.
- `shfmt`: `machismo: calling _main at 0x10007bf90`, then PC
  `0x1000624d0`, instruction `0x3980001b`, `x0=NULL`.
- `yq`: `machismo: calling _main at 0x100086050`, then PC
  `0x10006a6d0`, instruction `0x3980001b`, `x0=NULL`.
- Local Go source check: `runtime.sysargs` on Darwin skips `argv`, then
  `envp`, then reads `argv[n+1]` as `executable_path`. That requires a
  contiguous Darwin vector, not only separate C-call arguments.

Changed files: none for Attempt 1.

Next suspected cause: `setup_stack64` builds the contiguous guest vector but
leaves `lr->argv`, `lr->envp`, and `lr->applep` pointing at host arrays or a
static helper array, so LC_MAIN receives a non-Darwin vector.

## Attempt 2: pass guest-contiguous vectors to LC_MAIN

Patch:

- Updated `src/stack.c` to keep the original host `argv`/`envp` only while
  copying strings and pointers into the guest stack.
- After constructing the stack vectors, `lr->argv`, `lr->envp`, and
  `lr->applep` now point at the guest stack's contiguous Darwin vector.

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc '
set -u
cmake --build build-arm64 --target machismo --parallel
manifest=/tmp/worker-f-go-lcmain.txt
: > "$manifest"
awk -F"|" "/^(fzf|gum|shfmt)\|/ {print}" tests/external/worker_manifests/worker-2.txt >> "$manifest"
awk -F"|" "/^yq\|/ {print}" tests/external/arm64_macho_cli_manifest.txt >> "$manifest"
MACHGATE_RUN_EXTERNAL=1 MACHGATE_TRACE_SIGNALS=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST="$manifest" MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-f-attempt2 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-f-attempt2 MACHGATE_EXTERNAL_TIMEOUT=12 bash tests/test_external_macho_cli.sh
'
```

Result: FAILED-2, 0/4 passed.

Evidence:

- Logs: `tests/external/logs/worker-f-attempt2/`
- All four targets still fault at the same PCs and instruction `0x3980001b`
  with `x0=NULL`.
- The recorded `sp` remains in the host call stack region, which means the
  remaining mismatch is the LC_MAIN entry handoff itself.

Changed files:

- `src/stack.c`
- `docs/external-fix-workers/go-lcmain-abi.md`

Next suspected cause: `start_thread` still calls LC_MAIN as a normal host C
function. Go startup needs the Darwin guest stack installed before entering the
LC_MAIN entrypoint.

## Attempt 3: branch into LC_MAIN on the guest stack

Patch:

- Updated the ARM64 LC_MAIN path in `src/machismo.c` to load `x0=argc`,
  `x1=argv`, `x2=envp`, `x3=applep`, set `x29=0`, set `x30` to an
  `_exit(status)` return helper, install `sp=lr->stack_top`, and branch to the
  LC_MAIN entrypoint.

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc '
set -u
cmake --build build-arm64 --target machismo --parallel
manifest=/tmp/worker-f-go-lcmain.txt
: > "$manifest"
awk -F"|" "/^(fzf|gum|shfmt)\|/ {print}" tests/external/worker_manifests/worker-2.txt >> "$manifest"
awk -F"|" "/^yq\|/ {print}" tests/external/arm64_macho_cli_manifest.txt >> "$manifest"
MACHGATE_RUN_EXTERNAL=1 MACHGATE_TRACE_SIGNALS=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST="$manifest" MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-f-attempt3 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-f-attempt3 MACHGATE_EXTERNAL_TIMEOUT=12 bash tests/test_external_macho_cli.sh
'
```

Result: RECLASSIFIED, 0/4 passed.

Evidence:

- Logs: `tests/external/logs/worker-f-attempt3/`
- All four targets still fault at instruction `0x3980001b` with `x0=NULL`.
- The recorded stack pointer changed from host stack addresses like
  `0x7...e250` to guest stack addresses:
  - `fzf`: `sp=0xfffffbe20`
  - `gum`: `sp=0xfffffbe20`
  - `shfmt`: `sp=0xfffffbe10`
  - `yq`: `sp=0xfffffbe20`
- The entry handoff mismatch is fixed enough to prove the target is executing
  on the guest stack, but the Go runtime failure remains.

Changed files:

- `src/machismo.c`
- `src/stack.c`
- `docs/external-fix-workers/go-lcmain-abi.md`

Next suspected cause: verify the actual LC_MAIN vector contents. If
`argv[argc+1]` reaches `envp` and then `apple[0]=executable_path`, the
remaining crash is likely not the launch ABI but a later Go runtime assumption
or shim/syscall path.

## Attempt 4: trace LC_MAIN vector contents

Patch:

- Added gated `MACHGATE_TRACE_LCMAIN=1` diagnostics in `src/machismo.c`.
- The trace logs LC_MAIN stack top, direct `argv`/`envp`/`applep`, derived
  `envp`/`applep` from `argv + argc + 1`, and representative strings.

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc '
set -u
cmake --build build-arm64 --target machismo --parallel
manifest=/tmp/worker-f-go-lcmain.txt
: > "$manifest"
awk -F"|" "/^(fzf|gum|shfmt)\|/ {print}" tests/external/worker_manifests/worker-2.txt >> "$manifest"
awk -F"|" "/^yq\|/ {print}" tests/external/arm64_macho_cli_manifest.txt >> "$manifest"
MACHGATE_RUN_EXTERNAL=1 MACHGATE_TRACE_SIGNALS=1 MACHGATE_TRACE_LCMAIN=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST="$manifest" MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-f-attempt4 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-f-attempt4 MACHGATE_EXTERNAL_TIMEOUT=12 bash tests/test_external_macho_cli.sh
'
```

Result: RECLASSIFIED-STOP, 0/4 passed.

Evidence:

- Logs: `tests/external/logs/worker-f-attempt4/`
- All four targets report the same valid vector shape:
  - `stack=0xfffffbeb0`
  - `argv=0xfffffbec0`
  - `envp=0xfffffbed8`
  - `applep=0xfffffbf70`
  - derived `envp=0xfffffbed8`
  - derived `applep=0xfffffbf70`
- `apple0` is the expected `executable_path=...` string for each target.
- All four still fault at the original Go runtime PC/instruction:
  - `fzf`: PC `0x100066420`, instruction `0x3980001b`, `x0=NULL`.
  - `gum`: PC `0x10005f1e0`, instruction `0x3980001b`, `x0=NULL`.
  - `shfmt`: PC `0x1000624d0`, instruction `0x3980001b`, `x0=NULL`.
  - `yq`: PC `0x10006a6d0`, instruction `0x3980001b`, `x0=NULL`.

Changed files:

- `src/machismo.c`
- `src/stack.c`
- `docs/external-fix-workers/go-lcmain-abi.md`

Verification:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc '
set -u
cmake --build build-arm64 --target machismo --parallel
BUILD_DIR=/work/build-arm64 bash tests/fixtures/build_fixtures.sh
BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh
'
```

Result: `27/27 passed, 0 failed`.

## Next blocker

Worker F's launch ABI hypothesis is exhausted. The next owner should treat the
Go failures as a later runtime/shim problem after valid LC_MAIN setup. The
common crash remains in early Go runtime startup near the `internal/cpu` /
`GODEBUG` path; qemu traces show no target output and no post-entry translated
Darwin syscall before the NULL dereference.
