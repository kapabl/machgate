# Worker I: Worker 5 Real Failures

Scope: classify the seven real Worker 5 failures using current SIGSEGV
diagnostics and QEMU logs.

Targets:

- `cmake`
- `duckdb`
- `node`
- `bun`
- `protoc`
- `nvim`
- `sqlite3`

Owned write scope used:

- `docs/external-fix-workers/worker5-real-failures.md`

Source changes made by Worker I: none.

## Current status

Status: `RECLASSIFIED` after 2 / 5 attempts.

Exit condition met: every target has a concrete failure class and next fix
owner. No Worker I source edit was made because the likely fixes overlap active
Worker F, Worker G, Worker H, or Worker A ownership.

| Target | Attempt 2 result | Class | Next owner |
| --- | --- | --- | --- |
| `cmake` | fail 139 | C++ static constructor SIGSEGV, `x8=NULL`, before `_main` | Worker H for unresolved CF/shim surface first; then Worker G-style TLV/constructor ABI if CF is not sufficient |
| `duckdb` | fail 139 | post-`_main` low-address write SIGSEGV at `addr=0x508` | Worker H or new C++ runtime owner for skipped `libc++.1.dylib`/runtime ABI |
| `node` | timeout 124 | loader/runtime pre-`_main` timeout with many LSE branch-island range warnings plus CF/Security imports | Worker A for LSE range/pool strategy, then Worker H for CF/Security shim surface |
| `bun` | fail 139 | LSE pool exhausted plus CommonCrypto/Blocks missing; C++ initializer NULL SIGSEGV | Worker A for LSE pool exhaustion; Worker H for CommonCrypto/Blocks/libresolv stubs |
| `protoc` | fail 139 | C++ static constructor SIGSEGV at `addr=0x10`, `x0=NULL`, `x8=NULL` | Worker H/new C++ runtime owner for skipped `libc++.1.dylib`/constructor ABI |
| `nvim` | fail 139 | post-`_main` Mach/semaphore/process shim surface now narrowed to `sendmsg_x`, `sscanf_l`, `task_policy_set`, `thread_info`, `vm_deallocate`; SIGSEGV at `addr=0x210` | Worker H |
| `sqlite3` | fail 1 | reaches `_main`, prints `Error: out of memory`; malloc-zone/fsctl/gethostuuid/srandomdev imports unresolved | Worker H |

## Attempt 1: reproduce seven failures on current build

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail
cmake --build build-arm64 --target machismo --parallel
awk '\''BEGIN{FS=OFS="|"} /^#/ {print; next} /^$/ {print; next} $1 != "ninja" {print}'\'' tests/external/worker_manifests/worker-5.txt > /tmp/worker-i-seven.txt
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-i-seven.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-i-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-i-attempt1 MACHGATE_EXTERNAL_TIMEOUT=30 bash tests/test_external_macho_cli.sh'
```

Result: `FAILED-1`, `0/7 external Mach-O CLI probes passed`.

Logs:

- `tests/external/logs/worker-i-attempt1/`

Per-target result:

- `cmake`: fail 139. Runs 326 static constructors, then `SIGSEGV si_addr=NULL`.
- `sqlite3`: fail 1. Reaches `_main`; unresolved `malloc_*`,
  `malloc_zone_*`, `fsctl`, `gethostuuid`, `srandomdev`, and `atexit`; prints
  `Error: out of memory`.
- `duckdb`: fail 139. Runs constructors, reaches `_main`, then
  `SIGSEGV si_addr=0x508`.
- `node`: timeout 124. Does not reach `_main`; logs many
  `isa_emul: island out of B range` entries, then missing CoreFoundation
  imports before timeout.
- `bun`: fail 139. Reports `isa_emul: island pool exhausted after 32774
  patches`, missing CommonCrypto/Blocks imports, then C++ initializer
  `SIGSEGV si_addr=NULL`.
- `protoc`: fail 139. Runs 5 static constructors, then
  `SIGSEGV si_addr=0x10`.
- `nvim`: fail 139. Reaches `_main`, then `SIGSEGV`; missing Mach/process
  shim surface remains.

Changed files:

- None.

Next suspected cause:

- Rerun with `MACHGATE_TRACE_SIGNALS=1` to attach guest PC/register context to
  the five SIGSEGV rows. Do not patch source until ownership is clear.

## Attempt 2: rerun with signal diagnostics

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail
cmake --build build-arm64 --target machismo --parallel
awk '\''BEGIN{FS=OFS="|"} /^#/ {print; next} /^$/ {print; next} $1 != "ninja" {print}'\'' tests/external/worker_manifests/worker-5.txt > /tmp/worker-i-seven.txt
MACHGATE_RUN_EXTERNAL=1 MACHGATE_TRACE_SIGNALS=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-i-seven.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-i-attempt2 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-i-attempt2 MACHGATE_EXTERNAL_TIMEOUT=30 bash tests/test_external_macho_cli.sh'
```

Result: `RECLASSIFIED`, `0/7 external Mach-O CLI probes passed`.

Logs:

- `tests/external/logs/worker-i-attempt2/`

Evidence by target:

- `cmake`: constructor crash before `_main`.
  - `resolver: symbol 'kCFAllocatorDefault' not found`
  - `resolver: done -- 548 binds resolved, 2 stubbed, 3925 failed`
  - `machismo: running 326 static constructors from __mod_init_func`
  - `machismo: SIGSEGV code=1 addr=(nil) pc=0x10037aeec lr=0x10037ae54`
  - `machismo: regs ... x8=(nil)`
  - `machismo: guest context ... fileoff=0x37aeec insn=0xf87a7909 prev=0xf9400268 next=0xb5fff5a9`

- `duckdb`: reaches `_main`, then low-address write fault.
  - `resolver: done -- 2262 binds resolved, 0 stubbed, 5153 failed`
  - `machismo: calling _main at 0x100028a2c (argc=2)`
  - `machismo: SIGSEGV code=1 addr=0x508 pc=0x100026eb4 lr=0x100027f70`
  - `machismo: regs x0=(nil) x1=0xfffffbec0 ...`
  - `machismo: guest context ... fileoff=0x26eb4 insn=0xf9028728 prev=0xf9400128 next=0xd0012328`

- `node`: timeout before `_main`.
  - Repeated `isa_emul: island out of B range at ...`
  - `machismo: patched 3289 ARMv8.1 LSE atomics with LDXR/STXR islands`
  - Missing CF imports include `CFArraySetValueAtIndex`,
    `CFDictionaryContainsKey`, `CFDictionaryCreate`, `CFDictionaryGetValue`,
    `CFEqual`, `CFNumberGetValue`, `CFStringGetCString`,
    `CFStringGetMaximumSizeForEncoding`, `CFTimeZoneCopyDefault`,
    `kCFAllocatorDefault`, `kCFBooleanTrue`, `kCFTypeArrayCallBacks`,
    `kCFTypeDictionaryKeyCallBacks`, and `kCFTypeDictionaryValueCallBacks`.
  - QEMU log ends with `SIGALRM` timeout and `kill(..., SIGTERM)`.

- `bun`: C++ initializer crash with two independent preconditions.
  - `isa_emul: island pool exhausted after 32774 patches`
  - Missing imports: `CCCryptorCreateWithMode`, `CCCryptorReset`,
    `CCCryptorUpdate`, `CCHmacFinal`, `CCHmacInit`, `CCHmacUpdate`,
    `CCKeyDerivationPBKDF`, `NDR_record`, `_Block_release`,
    `_NSConcreteMallocBlock`
  - `machismo: running 1 C++ static initializers from __init_offsets`
  - `machismo: SIGSEGV code=1 addr=(nil) pc=0x1007d0a5c lr=0x7af9589253e4`
  - `machismo: regs x0=(nil) x1=0x100000000 ... x8=(nil)`
  - `machismo: guest context ... fileoff=0x7d0a5c insn=0xf9400134 prev=0x927df129 next=0xeb14011f`

- `protoc`: constructor crash before `_main`.
  - `resolver: done -- 146 binds resolved, 0 stubbed, 1510 failed`
  - `machismo: running 5 static constructors from __mod_init_func`
  - `machismo: SIGSEGV code=1 addr=0x10 pc=0x100270050 lr=0x1002700d0`
  - `machismo: regs x0=(nil) x1=0x2c18 x2=(nil) ... x8=(nil)`
  - `machismo: guest context ... fileoff=0x270050 insn=0xf9400915 prev=0xf9456508 next=0xf9000ff5`

- `nvim`: reaches `_main`, then narrowed Mach/process shim crash.
  - Remaining missing imports: `sendmsg_x`, `sscanf_l`, `task_policy_set`,
    `thread_info`, `vm_deallocate`
  - `resolver: done -- 402 binds resolved, 0 stubbed, 12 failed`
  - `machismo: calling _main at 0x1002e2430 (argc=2)`
  - `machismo: SIGSEGV code=1 addr=0x210 pc=0x1000bf248 lr=0x10039ce80`
  - `machismo: regs x0=0x10067a478 x1=0x100205bb0 x2=(nil) ... x8=(nil)`
  - `machismo: guest context ... fileoff=0xbf248 insn=0xf9410909 prev=0xf9400668 next=0xf9002a69`

- `sqlite3`: clean non-SIGSEGV failure.
  - Missing imports: `atexit`, `fsctl`, `gethostuuid`,
    `malloc_create_zone`, `malloc_default_zone`, `malloc_set_zone_name`,
    `malloc_size`, `malloc_zone_free`, `malloc_zone_malloc`,
    `malloc_zone_realloc`, `srandomdev`
  - `resolver: done -- 182 binds resolved, 0 stubbed, 11 failed`
  - `machismo: calling _main at 0x10000f844 (argc=2)`
  - `Error: out of memory`

Changed files:

- None.

Why no source patch:

- `src/isa_emul.c` and LSE pool/range strategy are Worker A-owned.
- `src/machismo.c` and stack/LC_MAIN ABI are Worker F-owned.
- `src/shim/libsystem_shim.c` is actively touched by Worker G and Worker H.
- The remaining source-level fixes are not narrow or non-overlapping from
  Worker I's scope.

Next suspected cause:

- `node`: fix large-binary LSE branch-island range/pool behavior first, then
  rerun to expose later CF/Security behavior.
- `bun`: fix LSE pool exhaustion and CommonCrypto/Blocks shims before treating
  the initializer crash as primary.
- `sqlite3` and `nvim`: Worker H shim surface can directly re-run these after
  adding the listed symbols.
- `cmake`, `duckdb`, and `protoc`: need C++ runtime/constructor investigation
  after shim surface is reduced; skipped `libc++.1.dylib` and thousands of
  unresolved C++ binds make a Worker I source patch unsafe.
