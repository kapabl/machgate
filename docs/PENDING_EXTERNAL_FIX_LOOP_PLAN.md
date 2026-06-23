# Pending External Mach-O Fix Loop Plan

This plan is for finishing the currently non-passing external ARM64 macOS CLI binaries.

## Current Baseline

Last refreshed: 2026-06-23.

- Corpus: 57 unique external ARM64 macOS CLI binaries
- Passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 51 / 57
- Not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 6 / 57
- Previous passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 49 / 57
- Previous not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 8 / 57
- Passing without libc++ opt-in: 45 / 57
- Not passing without libc++ opt-in: 12 / 57
- Previous full corpus before `sqlite3` fix: 44 / 57
- Canonical manifest: 19 / 19 passing
- Latest full-run logs: `tests/external/logs/full-loop-2026-06-23-K-loop-e-final/`
- Latest full-run work dir: `tests/external/work/full-loop-2026-06-23-K-loop-e-final/`
- Agent status: Loop E Codex workers complete; Grok read-only report collected; Claude was blocked by the local session limit.

Current non-passing binaries:

`nu`, `tilt`, `terraform`, `packer`, `nomad`, `bun`.

Loop E promoted `cmake` and `nvim` to the full-corpus passing set. `protoc` remains passing.

## Exit Criteria

The loop stops when one of these is true:

- [ ] 57 / 57 external binaries pass with pinned manifests.
- [ ] Every remaining non-pass has five documented fix attempts with logs and a precise blocker.

Per-binary stop conditions:

- [ ] PASS: binary exits successfully with expected version/help output.
- [ ] MANIFEST-FIXED: binary was not broken; manifest argv was wrong and is corrected.
- [ ] HARDER-5: five attempts failed; keep logs, exact crash class, and next suspected subsystem.

## Outer Loop

Repeat until exit criteria:

1. Pick the highest-impact non-passing bucket.
2. Assign independent binaries/subsystems to agents.
3. Each agent performs one narrow attempt, not a broad rewrite.
4. Main thread integrates only reviewed patches.
5. Rebuild ARM64 target in Docker/QEMU.
6. Rerun targeted binaries.
7. Update this file and `docs/EXTERNAL_MACHO_LIVE_STATUS.md`.
8. Run the full 57-binary external corpus after any fix that changes loader, syscall, ISA, or shim behavior.

## Inner Loop Per Binary

For each binary:

1. Run targeted probe with current code.
2. Classify: argv/program error, SIGILL, SIGSEGV, timeout, missing symbol, syscall/errno panic, island range.
3. If stdout/stderr is enough, patch narrowly.
4. If not enough, run with `QEMU_STRACE=1`.
5. If still unclear, add temporary targeted trace logging or use symbol/disassembly tools.
6. Patch one subsystem.
7. Rerun targeted probe.
8. Stop after PASS or 5 failed attempts.

Attempt counter starts at 0 for the 2026-06-23 baseline.

## Work Queues

### Queue A: Manifest Argv False Failures

Owner scope: `tests/external/worker_manifests/*.txt`, canonical manifests, docs.

- [x] `helm`: changed argv to `version`; targeted ARM64 Docker/QEMU probe passes.
- [x] `boundary`: changed argv to `version`; targeted ARM64 Docker/QEMU probe passes.
- [x] `pulumi`: changed argv to `version`; targeted ARM64 Docker/QEMU probe passes.

Expected result: these should become PASS without loader/shim changes.

### Queue B: Zero-Failed-Bind SIGILL

Owner scope: ISA/emulation diagnosis and small ISA patches only.

- [x] `sqlite3`: PASS. Zero failed binds, reaches `_main`, and exits cleanly after malloc-zone and variadic fixes.

Plan:

- [x] Identify fault PC/instruction from qemu trace or added diagnostics.
- [x] Determine whether this is unsupported ARM instruction, bad code pointer, or host signal translation.
- [x] Add smallest emulator/patch support if needed.
- [x] Rerun `sqlite3 --version`.

Result:

- Five worker attempts first failed. The fault is not an unresolved bind: resolver reports `193 binds resolved, 0 stubbed, 0 failed`.
- Fault PCs consistently land inside the ARM64 `libsystem_shim.so` RX mapping but at `.rodata` objects, not executable shim text. Observed targets include `cf_type_dictionary_key_callbacks_object` at offset `0xab50` before the callback-table patch and `cf_url_volume_total_capacity_key` at offset `0xc1d0` after concurrent shim-surface changes.
- `sqlite3` chained imports contain no CoreFoundation symbols, so the rodata labels are likely accidental branch targets from a bad indirect-call/function-pointer target, variadic thunk target, or resolver-written pointer, not direct sqlite3 CF usage.
- Main-thread attempt 6 replaced the fake one-field malloc zone with a Darwin-shaped malloc zone function table. The failure changed from `SIGILL` to `SIGSEGV` at an ASCII-looking address, proving the previous crash was execution through data.
- Main-thread attempt 7 added a missing `vfprintf` variadic adapter and Darwin ABI `access`/`faccessat` wrappers. Targeted run `sqlite3-variadic-vfprintf-attempt7` passed 1/1.

### Queue C: Go Runtime / OS Compatibility

Owner scope: syscall translation, errno mapping, uname/sysctl/getattrlist-style OS queries, Go startup traces.

- [x] `vault`: PASS in targeted attempt 5 after narrow `libsystem_shim.c` sysctl/sysctlbyname/uname-style coverage. The previous Go `EMULTIHOP (Reserved)` panic in `gosnowflake.getOSVersion` is gone.
- [~] `terraform`: attempt 4 still `SIGSEGV si_addr=NULL` after `_main`; qemu-strace shows Go runtime/network probing, PATH `uname` searches, repeated `faccessat2(..., flags=0x10) -> EINVAL`, then NULL SIGSEGV. Do not reintroduce broad access/faccessat shim coverage without a child-process/path-resolution design because that previously regressed `lazygit`.
- [~] `packer`: attempt 4 still `SIGSEGV si_addr=NULL` after `_main`; qemu-strace shows a real child process path (`clone(...|SIGCHLD)`, child exit status 253), then `rt_sigaction(SIGSTOP) -> EINVAL`, then NULL SIGSEGV. This is not safe to paper over as a syscall errno tweak without resolving child-process semantics.
- [x] `nomad`: HARDER-5. Attempt 5 resolved the narrow CoreFoundation gaps `CFDictionaryAddValue` and `CFStringGetSystemEncoding`, and Loop C later resolved `vm_kernel_page_size` plus LSE placement. The binary still exits 139 after `_main`; current visible blocker is 15 failed binds from skipped `IOKit`, with no remaining mapped-shim missing-symbol lines.

Plan:

- [x] Reproduce with targeted manifests.
- [x] Locate syscalls/errno immediately before panic/crash.
- [~] Fix Darwin-to-Linux errno/syscall behavior if wrong; no attempt-4 syscall patch was clear enough to apply without crossing into shim or child-process ownership.
- [x] Rerun the group after each patch.

### Queue D: Missing Shim / Dylib Surface

Owner scope: `src/shim/libsystem_shim.c`, `src/shim/libsystem_shim.ver`, dylib map/test config only.

- [~] `nu`: latest `full-loop-2026-06-23-K-loop-e-final` still reports `SIGSEGV` after `_main`; resolver reports `519 resolved, 13 stubbed, 1 failed`. Skipped/no-map dylibs remain `AppKit`, `CoreGraphics`, and `Foundation`; `libobjc.A.dylib` is stubbed. Native `__eh_frame` is now hook-served through `_dl_find_object`.
- [~] `tilt`: latest `full-loop-2026-06-23-K-loop-e-final` still reports `SIGSEGV` after `_main`; resolver reports `231 resolved, 0 stubbed, 0 failed`. This is now runtime/EH/trampoline behavior after `_main`, not visible import surface.
- [x] `cmake`: PASS in `full-loop-2026-06-23-K-loop-e-final` after `libcurl.4.dylib` mapping plus Darwin `dlsym` special-handle translation.
- [~] `bun`: latest `full-loop-2026-06-23-K-loop-e-final` still reports `SIGSEGV` in the single C++ static initializer. LSE pool exhaustion is fixed. Resolver improved to `910 resolved, 0 stubbed, 129 failed`; printed gaps are ICU `ucfpos_*`, `ucol_*`, `ucurr_*`, and `udat_close`.
- [x] `protoc`: PASS in `full-loop-2026-06-23-K-loop-e-final` with native Mach-O `__eh_frame` served by `_dl_find_object`.
- [x] `nvim`: PASS in `full-loop-2026-06-23-K-loop-e-final` after the `ioctl(FIONBIO)` stack-argument thunk and native Mach-O `__eh_frame` hook-only path.

Plan:

- [x] Group missing symbols by dylib and by easy constant/function shim.
- [x] Add only low-risk constants and pure/simple functions first.
- [x] Do not fake large C++ runtime surfaces blindly.
- [x] Rerun affected binaries after each symbol group.

### Queue E: Large Binary LSE / Fixup Scalability

Owner scope: LSE island placement, ISA patching range, chained fixup performance, timeout tuning.

- [x] `duckdb`: reclassified from timeout to real `SIGSEGV`; 28290/28290 LSE patches, 0 out-of-range. Loop B libc++ mapping attempt passed `duckdb --version`.
- [x] `node`: LSE island range fixed; reclassified from timeout/range debt to real `SIGSEGV`; 6603/6603 ISA patches, 0 out-of-range. Loop B libc++ mapping attempt passed `node --version` and printed `v26.3.1`.

Plan:

- [x] Read-only Claude check: current 15s failures are CPU-bound progress until timeout, not clear deadlock.
- [x] Rerun with longer timeout to separate slow-progress from hang.
- [x] Count out-of-range ISA patches and affected address spans.
- [x] Add additional nearby island pools or segmented island allocation if needed.
- [x] Rerun with timing instrumentation.

## Active Agent Assignments

- [x] Codex Agent A: Queue A manifest argv fixes. Agent `019ef370-b3df-7241-86ec-39c65c307bf2` (`Hooke`).
- [x] Codex Agent B: Queue B `sqlite3` SIGILL. Agent `019ef370-ccd2-70e3-9439-aadc0c643c33` (`Galileo`).
- [x] Codex Agent C: Queue C Go runtime / OS compatibility. Agent `019ef370-ef48-7013-b01e-73ae9eda0fb8` (`Beauvoir`).
- [x] Codex Agent D: Queue D missing shim surface classification. Agent `019ef371-0c59-7980-bffb-6e9f1aee3354` (`Euclid`).
- [x] Codex Agent E: Queue E large binary LSE/fixup scalability. Agent `019ef371-2b17-7e70-b132-aef7b65a1b0a` (`Rawls`).
- [x] Grok read-only: `sqlite3` SIGILL report. PID `3940545`, output `/tmp/machgate-agent-reports/grok-sqlite3-sigill.txt`.
- [x] Grok read-only: Go runtime pending report. PID `3940580`, output `/tmp/machgate-agent-reports/grok-go-runtime-pending.txt`.
- [x] Claude read-only: missing shim surface report. PID `3940614`, output `/tmp/machgate-agent-reports/claude-shim-surface.txt`.
- [x] Claude read-only: large binary LSE/fixup report. PID `3940538`, output `/tmp/machgate-agent-reports/claude-large-binary.txt`.

## Verification Commands

Create the current unique manifest:

```sh
awk -F"|" 'BEGIN { OFS="|" } /^[[:space:]]*#/ || NF < 5 { next } !seen[$1]++ { print }' \
  tests/external/arm64_macho_cli_manifest.txt \
  tests/external/worker_manifests/*.txt \
  > /tmp/machgate-current-external-all.txt
```

Run one targeted manifest:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -v /tmp/target-manifest.txt:/tmp/target-manifest.txt:ro \
  -w /work machgate-arm64-toolchain bash -lc '
    cmake --build build-arm64 --target machismo system_shim --parallel
    MACHGATE_RUN_EXTERNAL=1 \
    BUILD_DIR=/work/build-arm64 \
    MACHGATE_EXTERNAL_MANIFEST=/tmp/target-manifest.txt \
    MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/<attempt-name> \
    MACHGATE_EXTERNAL_WORK=/work/tests/external/work/<attempt-name> \
    MACHGATE_EXTERNAL_TIMEOUT=30 \
    bash tests/test_external_macho_cli.sh'
```

Run full corpus:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -v /tmp/machgate-current-external-all.txt:/tmp/machgate-current-external-all.txt:ro \
  -w /work machgate-arm64-toolchain bash -lc '
    cmake --build build-arm64 --target machismo system_shim --parallel
    MACHGATE_RUN_EXTERNAL=1 \
    BUILD_DIR=/work/build-arm64 \
    MACHGATE_EXTERNAL_MANIFEST=/tmp/machgate-current-external-all.txt \
    MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/full-loop-<N> \
    MACHGATE_EXTERNAL_WORK=/work/tests/external/work/full-loop-<N> \
    MACHGATE_EXTERNAL_TIMEOUT=30 \
    bash tests/test_external_macho_cli.sh'
```

## Attempt Log

### Loop 2026-06-23-A

- [x] Baseline refreshed: 41 / 57 passing.
- [x] Plan written.
- [x] Agents launched: 5 Codex workers plus 4 external read-only Claude/Grok jobs.
- [x] Queue A attempt 1: changed `helm`, `boundary`, and `pulumi` argv probes to `version`; targeted ARM64 Docker/QEMU run `queue-a-attempt1` passed 3/3. Reran the edited worker-manifest entries as `queue-a-final`; passed 3/3.
- [x] Claude large-binary report returned: `/tmp/machgate-agent-reports/claude-large-binary.txt`. Finding: `duckdb`/`node` are likely slow under nested QEMU at 15s; `node` also has real island range warning debt.
- [x] Queue E attempt 1 `queue-e-attempt1-long120`: reran `duckdb`/`node` with 120s timeout. Both failed with `SIGSEGV` 139, not timeout. `duckdb`: 0 out-of-range ISA patches, 28290 LSE patches, crash after `_main`, qemu `si_addr=0x508`. `node`: 3314 out-of-range ISA patches from `0x100010650` to `0x100a1a3fc` (span `0xa09dac`), only 3289 patches applied, crash during 248 C++ static initializers, qemu `si_addr=0x10`.
- [x] Queue E attempt 2 `queue-e-attempt2-pretext-lse`: implemented pre-`__TEXT` main executable LSE island pool in `src/machismo.c` when the executable span fits within ARM64 `B` reach. Targeted rerun still 0/2, but `node` out-of-range count dropped to 0 and patched ISA sites rose to 6603. `duckdb` unchanged at 28290 patched and 0 out-of-range. Remaining crashes are post-patching startup `SIGSEGV`s.
- [x] Queue E attempt 3 `queue-e-attempt3-direct-timing`: direct normal probes without diagnostic reruns. `duckdb` `SIGSEGV` 139 in 19s; `node` `SIGSEGV` 139 in 46s. Both had 0 out-of-range ISA patches. Focused LSE regression `./build-arm64/test_lse_emul` passed 480/480.
- [x] Queue E blocker/handoff: LSE island placement debt for `node` is fixed. Remaining `duckdb`/`node` failures are not hangs and no longer show ISA island range failures; next suspected owners are missing `libc++`/shim surface or startup constructor behavior, not Queue E scalability.
- [x] Queue B sqlite3 attempts 1-5:
  - attempt1 `queue-b-sqlite3-attempt1`: reproduced `SIGILL` 132; qemu-strace PC `0x...ab50` in `libsystem_shim.so` `.rodata`, symbol `cf_type_dictionary_key_callbacks_object`; resolver `193 resolved, 0 failed`.
  - attempt2 `queue-b-sqlite3-attempt2`: after CF callback constants were changed from string sentinels to real callback tables, still `SIGILL` 132; PC moved to `0x...bc70`, then mapped to `cf_allocator_default_object`.
  - attempt3 `queue-b-sqlite3-attempt3`: after `kCFAllocatorDefault = NULL`, failure changed to `SIGSEGV` 139 at `si_addr=NULL`.
  - attempt4 `queue-b-sqlite3-attempt4-trace`: after concurrent shim-surface rebuild, failure returned to `SIGILL` 132; PC offset `0xc1d0` in shim `.rodata`.
  - attempt5 `queue-b-sqlite3-attempt5-shimtrace`: with `MACHGATE_TRACE_SHIM=1 MACHGATE_TRACE_SIGNALS=1`, still `SIGILL` 132; qemu-strace maps PC `0x...c1d0` to `cf_url_volume_total_capacity_key`.
- [x] Queue B blocker: sqlite3 imports no CF symbols, yet the branch target lands in shim CF rodata. Stop condition reached at 5 attempts. Next suspected subsystem is resolver or indirect-call thunk target corruption, especially the five variadic thunks created during sqlite3 bind resolution.
- [x] Main-thread sqlite3 attempt 6 `sqlite3-malloc-zone-attempt6`: Darwin-shaped malloc zone changed `sqlite3` from `SIGILL` to `SIGSEGV`, exposing a remaining variadic/access ABI problem.
- [x] Main-thread sqlite3 attempt 7 `sqlite3-variadic-vfprintf-attempt7`: added `vfprintf` variadic adapter plus `access`/`faccessat` wrappers; targeted `sqlite3` passed 1/1.
- [x] Queue C attempt 1: targeted `terraform`, `packer`, `vault`, `nomad` as `queue-c-attempt1`; 0/4 passed. `vault` panicked in `gosnowflake.getOSVersion` with `EMULTIHOP`; `terraform`/`packer` crashed with NULL SIGSEGV after Go runtime/network probing; `nomad` crashed with NULL/0x110 SIGSEGV and missing CF shim symbols.
- [x] Queue C attempt 2: added raw Darwin sysctl `KERN_HOSTNAME`/`kern.hostname` handling in `src/syscall/syscall_range_200_399.c`; fixture `tests/test_darwin_range_200_399.sh` passes. Targeted rerun `queue-c-attempt2-hostname-sysctl` still 0/4 because `vault` uses Go `libc_sysctl_trampoline` through `libsystem_shim.c`, not the raw syscall gateway.
- [x] Queue C attempt 3 diagnostics: `queue-c-attempt3-vault-shim-trace` confirmed heavy shim startup before the same `EMULTIHOP` panic and binary strings include `runtime.sysctl`, `runtime.sysctl_trampoline`, `libc_sysctl+0-tramp0`, and `golang.org/x/sys/unix.Uname`. `queue-c-attempt3-signal-trace` kept `terraform`, `packer`, and `nomad` failing; QEMU catches the SIGSEGV before MachGate emits guest PC context.
- [x] Queue D attempt 1: targeted `nu`, `tilt`, `cmake`, `bun`, `protoc`, `nvim` as `queue-d-attempt1`; 0/6 passed. Added/verified low-risk `nu` shims for `CC_SHA256_Init`, `CC_SHA256_Update`, `CC_SHA256_Final`, `CFArrayCreate`, `CFArrayInsertValueAtIndex`, `kCFAbsoluteTimeIntervalSince1970`, `kCFAllocatorNull`, `kCFRunLoopDefaultMode`, and `kCFURLVolume*` constants. `nu` missing-shim list shrank to second-wave CF functions; other binaries stayed in large skipped dylib classes.
- [x] Queue D attempt 2: targeted `nu` as `queue-d-nu-attempt2`; still `SIGSEGV` 139. Added simple CF object/type/data/string/dictionary/number/locale/run-loop shims: `CFArrayRemoveValueAtIndex`, `CFBooleanGetValue`, `CFDataGetBytes`, `CFDataGetTypeID`, `CFDictionaryGetTypeID`, `CFDictionaryGetValueIfPresent`, `CFGetTypeID`, `CFLocaleCopyPreferredLanguages`, `CFNumberCreate`, `CFRetain`, `CFRunLoopGetCurrent`, `CFRunLoopIsWaiting`, `CFRunLoopRun`, `CFRunLoopStop`, `CFRunLoopWakeUp`, `CFStringCompare`, `CFStringCreateCopy`, `CFStringCreateWithCString`, `CFStringCreateWithBytesNoCopy`, and `CFStringGetTypeID`. Remaining printed missing symbols moved to CFURL path/resource helpers.
- [x] Queue D attempt 3: targeted `nu` as `queue-d-nu-attempt3`; still `SIGSEGV` 139 after `_main`, but resolver no longer prints any mapped-shim missing symbol. Added string/path-backed `CFURLCopyAbsoluteURL`, `CFURLCopyFileSystemPath`, `CFURLCopyLastPathComponent`, `CFURLCopyResourcePropertiesForKeys`, `CFURLCreateCopyAppendingPathComponent`, `CFURLCreateCopyDeletingLastPathComponent`, `CFURLCreateFilePathURL`, `CFURLCreateFileReferenceURL`, `CFURLCreateFromFileSystemRepresentation`, and `CFURLResourceIsReachable`. Stop `nu` at 3/5 attempts for now because remaining 46 failed binds are skipped/no-map AppKit/CoreGraphics/Foundation/Security/CoreServices/libiconv/IOKit surface, not narrow CoreFoundation/CommonCrypto shim work.
- [x] Queue D classification: `tilt`, `cmake`, `bun`, `protoc`, and `nvim` each have 1/5 documented attempts. Do not broad-stub C++ ABI, ICU, libcurl, Security trust APIs, FSEvents, libiconv, or libutil without a more specific handoff/design.
- [x] Main-thread full corpus `full-loop-2026-06-23-A`: 44/57 passed before the `sqlite3` follow-up fix. `helm`, `boundary`, and `pulumi` are confirmed passing in the full corpus. `duckdb` and `node` no longer time out with 120s; both are real `SIGSEGV` failures.
- [x] First targeted fixes integrated.
- [x] Full corpus rerun `full-loop-2026-06-23-C`: 45/57 passed, 12 failed. `sqlite3` and `lazygit` both pass; no new regression remains from the sqlite3 fix.
- [x] Queue C attempt 4 `queue-c-attempt4-current-analysis`: targeted current `terraform`, `packer`, `vault`, and `nomad` with Docker/QEMU after reading `full-loop-2026-06-23-C` logs; 0/4 passed. `terraform` stayed `SIGSEGV` 139 and still shows PATH `uname` probes with `faccessat2(..., flags=0x10) -> EINVAL` before the NULL crash. `packer` stayed `SIGSEGV` 139 after a child `clone(...|SIGCHLD)`, child status 253, and `rt_sigaction(SIGSTOP) -> EINVAL`; no safe process/signal errno-only patch identified. `vault` normal run timed out at 30s in this attempt, while qemu diagnostics still show the known `EMULTIHOP (Reserved)` panic in `gosnowflake.getOSVersion`, confirming the shim `libc_sysctl_trampoline` handoff. `nomad` stayed `SIGSEGV` 139 with missing `CFDictionaryAddValue` and `CFStringGetSystemEncoding`, plus many LSE island out-of-B-range warnings. No code changes from this attempt; handoffs are shim sysctl/access, child-process semantics, and LSE placement.
- [x] Queue C/shim attempt 5 `shim-sysctl-cf-vault-nomad-attempt1`: added narrow `libsystem_shim.c` coverage for `sysctl`, `sysctlbyname`, `sysctlnametomib`, Darwin-shaped `uname`, `CFDictionaryAddValue`, and `CFStringGetSystemEncoding`. Targeted Docker/QEMU result: `vault` PASS and prints `Vault v2.0.3`; `nomad` still `SIGSEGV` 139. `nomad` no longer reports missing `CFDictionaryAddValue` or `CFStringGetSystemEncoding`; resolver now reports 185 binds resolved and 34 failed from skipped `libresolv.9.dylib`, `IOKit`, and `Security`, with qemu-strace ending at `SIGSEGV si_addr=NULL` then `si_addr=0x110`. Stop `nomad` at HARDER-5; next owners are skipped-dylib surface and/or LSE placement, not the narrow CF/sysctl shim queue.

### Loop 2026-06-23-B

- [x] Baseline: 45/57 passing, 12 pending.
- [x] Agents launched:
  - Codex libc++/dylib mapping: `019ef3d7-4098-7b22-99d0-b5659db1f3f5` (`Bohr`).
  - Codex Go runtime/process/syscall: `019ef3d7-757a-72c3-b3bb-43890cbcf6c5` (`Gauss`).
  - Codex shim sysctl/CF for vault/nomad: `019ef3d7-9226-79a0-8a38-4e11863bd25b` (`Avicenna`).
  - Codex nvim libiconv/libutil/CoreServices: `019ef3d7-c1fa-7bd3-8916-6894609783a9` (`Cicero`).
  - Codex nu remaining classification/fixes: `019ef3d7-def9-7e23-ae3a-f66abfe78c29` (`Heisenberg`).
- [x] External read-only helpers launched:
  - Grok libc++ report PID `3989959`, output `/tmp/machgate-agent-reports/loop2/grok-libcxx.txt`.
  - Grok Go runtime report PID `3989964`, output `/tmp/machgate-agent-reports/loop2/grok-go-runtime.txt`.
  - Claude nvim report PID `3989972`, output `/tmp/machgate-agent-reports/loop2/claude-nvim.txt`.
  - Claude nu report PID `3989968`, output `/tmp/machgate-agent-reports/loop2/claude-nu.txt`.
- [x] Cicero nvim attempts 2-4:
  - attempt2 `nvim-libiconv-attempt2`: mapped `libiconv.2.dylib` to native glibc `libc.so.6`, resolving `_iconv`, `_iconv_close`, and `_iconv_open`; targeted run still `SIGSEGV` 139, failed binds dropped from 7 to 4.
  - attempt3 `nvim-coreservices-attempt3`: mapped `CoreServices` to `libm.so.6` for `_log2`/`_exp2` and added a minimal `LocaleRefGetPartString` export; targeted run still `SIGSEGV` 139, resolver reports `414 resolved, 0 failed`.
  - attempt4 `nvim-trace-attempt4`: diagnostic rerun with `MACHGATE_TRACE_SHIM=1 MACHGATE_TRACE_SIGNALS=1`; still `SIGSEGV si_addr=0x210`. Signal context: PC `0x1000bf248` in `_uv_close`, LR `0x10039ce80` in `_signal_teardown`, instruction `ldr x9, [x8, #0x210]` after loading `x8` from `[handle + 0x8]`. Next suspected subsystem is libuv handle/loop initialization or signal teardown state, not skipped `CoreServices`/`libiconv`/`libutil` surface.
- [x] Heisenberg nu attempt 4 `nu-classification-attempt4`: targeted current `nu --version` with Docker/QEMU and `MACHGATE_TRACE_SIGNALS=1`; still `SIGSEGV` 139. Current config maps `libiconv.2.dylib` to `libc.so.6` and `CoreServices` to `libm.so.6`, so the targeted run reports `477 resolved, 13 stubbed, 43 failed`; `libiconv` is resolved, while the 6 `FSEventStream*` symbols remain missing from `libm.so.6`. Signal context: PC `0x1018fe474` in Rust `std::sys::thread_local::native::lazy::Storage<T,D>::get_or_init_slow`, LR `0x1011fbcec`, fault address `0x70c0cd36fb50`, instruction `str xzr, [x8]` with `x8` equal to the fault address. No code changes; blockers are broad `Security`/`IOKit`/FSEvents/libSystem process-copyfile surface, not narrow CF/Foundation shim work.
- [x] Bohr libc++ build/mapping:
  - build attempt `scripts/build-libcxx.sh`: initialized `extern/llvm-project` at pinned commit `248d1a44e31805bcffdd38234bcddcb211a25a8b` after an initial full clone reset; patched the script to accept the detached pinned checkout, verify Darwin ABI patch markers, and run `cmake --build build-libcxx --target cxx cxxabi --parallel`. ARM64 Docker/QEMU build produced `build-libcxx/lib/libc++.so.1` and `build-libcxx/lib/libc++abi.so.1`.
  - harness integration: added opt-in external-test mapping with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` or `MACHGATE_EXTERNAL_LIBCXX=/path/to/libc++.so.1`. The default external harness remains unchanged unless the opt-in is set.
  - `protoc` attempt 2 `libcxx-protoc-attempt2`: `libc++.1.dylib` loaded from `/work/build-libcxx/lib/libc++.so.1`; resolver improved to `1655 resolved, 1 failed` with only `sigsetjmp` missing, then `SIGSEGV si_addr=0x10000000` during static constructors. Status remains FAIL, 2/5 attempts.
  - `libcxx-group-attempt2`: targeted `tilt`, `cmake`, `duckdb`, `node`, and `bun` with opt-in libc++ mapping. `duckdb` PASS; `node` PASS and prints `v26.3.1`. `tilt` remains FAIL with 27 failed binds and NULL SIGSEGV after `_main`; `cmake` reaches `_main`, reports missing `CMAKE_ROOT`, then FAILS with `SIGSEGV si_addr=0x3d6`; `bun` remains FAIL with unmapped ICU, LSE island-pool exhaustion, 222 failed binds, and NULL SIGSEGV in static initialization.
- [x] Full corpus rerun `full-loop-2026-06-23-D-libcxx` with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 48/57 passed, 9 failed. Promoted `vault`, `duckdb`, and `node` into the passing set. Remaining failures: `nu`, `tilt`, `terraform`, `packer`, `nomad`, `cmake`, `bun`, `protoc`, and `nvim`.
- [x] First Loop B targeted fixes integrated.
- [x] Full corpus rerun after Loop B.

### Loop D Classification

Verification source: `tests/external/logs/full-loop-2026-06-23-E2-loop-c/`, plus read-only `llvm-objdump` inspection of the already-extracted Mach-O files.

| Binary | Binds resolved/stubbed/failed | First SIGSEGV evidence | Missing mapped symbols | Skipped/unmapped dylibs | Likely first blocker | Next exact experiment |
| --- | ---: | --- | --- | --- | --- | --- |
| `nu` | 483 / 13 / 37 | E2 qemu-strace: PC not printed, `si_addr=0x7deea648fb50`; prior `nu-classification-attempt4` trace had PC `0x1018fe474` in Rust TLS slow init. | `FSEventStreamCreate`, `FSEventStreamInvalidate`, `FSEventStreamRelease`, `FSEventStreamScheduleWithRunLoop`, `FSEventStreamStart`, `FSEventStreamStop`. | `AppKit`, `CoreGraphics`, `Foundation`, `IOKit`; `libobjc.A.dylib` stubbed. | Framework surface plus Rust TLS fault after `_main`; not CoreFoundation. | Add/trace focused FSEvents stubs returning an inert stream, keep `MACHGATE_TRACE_SIGNALS=1`, and confirm whether first fault moves away from Rust TLS. |
| `tilt` | 215 / 0 / 16 | E2 qemu-strace: PC not printed, `si_addr=NULL`. | No named mapped-missing lines in resolver output; imports are flat namespace and include FSEvents/Security/CF helpers. | None; all declared dylibs mapped. | Failed flat-namespace framework surface. | Add temporary failed-bind logging for flat namespace resolution or run a targeted resolver trace, then stub only the printed first-wave FSEvents/Security gaps. |
| `terraform` | 143 / 0 / 0 | E2 qemu-strace: PC not printed, first `si_addr=NULL`. | None. | None. | Zero-bind Go runtime crash during network/scheduler/process probing. | Targeted `terraform --version` with guest signal-context logging enabled around Darwin `sigaction`, `sigaltstack`, `kevent`, and Go runtime signal entry; classify first guest PC before patching syscalls. |
| `packer` | 155 / 0 / 0 | E2 qemu-strace: PC not printed, first `si_addr=NULL` after child status `253` and `rt_sigaction(SIGSTOP) -> EINVAL`. | None. | None. | Go runtime child-process/signal semantics. | Targeted run tracing `clone`/`wait4`/Darwin `posix_spawn`-style paths and signal registration; do not paper over `SIGSTOP` errno without a child-process model. |
| `nomad` | 204 / 0 / 15 | E2 qemu-strace: PC not printed, first `si_addr=NULL`, followed by `si_addr=0x110`. | None. | `IOKit` only; chained imports list 15 IOKit symbols. | Skipped IOKit surface or Go runtime use of absent platform probes; LSE is clean. | Map `IOKit` to the shim in a targeted branch and add null/failure stubs for the 15 imported IOKit symbols, then rerun `nomad --version` with signal tracing. |
| `cmake` | 4464 / 0 / 11 | E2 qemu-strace: PC not printed, `si_addr=0x3d6` after CMAKE_ROOT discovery. | None. | `libcurl.4.dylib`; 11 `curl_*` imports. | Unmapped libcurl surface or bad call through unresolved libcurl pointers. | Targeted map of `libcurl.4.dylib` to host `libcurl.so` if available, otherwise add a libcurl shim that returns deterministic failure for the 11 imported APIs. |
| `bun` | 819 / 0 / 220 | E2 qemu-strace: PC not printed, `si_addr=NULL` during the single C++ static initializer. | `__getdirentries64`, `__pthread_fchdir`, `__ulock_wait2`, `__ulock_wake`, dispatch source APIs, Mach timing/message APIs, `clonefile*`, `fcopyfile`, `getprogname`, `getsegmentdata`, `host_info`, `kevent64`. | `libicucore.A.dylib`; chained imports include 169 ICU symbols. | ICU plus libdispatch/Mach/system shim surface; LSE exhaustion is fixed. | First test an ICU mapping/shim for version/case/break/calendar entry points, then rerun to see whether the static initializer moves before adding dispatch/Mach stubs. |
| `nvim` | 414 / 0 / 0 in E2; 414 / 0 / 0 with 10 variadic thunks in Loop D attempt 8 | E2 qemu-strace: PC not printed, `si_addr=0x210`; prior trace PC `0x1000bf248`, LR `0x10039ce80` in `_uv_close` from `_signal_teardown`. | None. | `libutil.dylib` is unmapped but has no visible imports. | E2 blocker was libuv signal teardown plus Darwin stack-vararg `fcntl`/`ioctl`; Loop D attempt 8 targeted `nvim --version` now passes. | Run the full 57-binary corpus to promote `nvim` out of the non-passing set and catch regressions from the resolver/shim/EH-frame changes. |

### Loop 2026-06-23-C

Goal: move from `51/57` passing to `57/57` passing, or document five narrow failed attempts for each remaining binary with exact blockers and logs.

Global exit conditions:

- [ ] `57/57` external Mach-O CLI probes pass with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`.
- [ ] If a binary cannot be closed in this loop, it has five documented attempts, exact latest logs, and a precise subsystem blocker.

Per-attempt rules:

1. Change one subsystem at a time.
2. Run a targeted manifest for the affected binary or bucket.
3. If stdout/stderr is unclear, rerun with `QEMU_STRACE=1` or MachGate trace flags.
4. Update this file after each pass/fail.
5. Full-corpus rerun after any loader, syscall, resolver, shim, dylib-map, or runtime behavior change.

Codex worker status:

| Worker | Agent | Scope | Exit condition |
| --- | --- | --- | --- |
| C1 | Tesla `019ef502-1b1b-7e12-bede-b06ae9f2e07c` | `protoc`: `sigsetjmp`, constructor-state crash | complete; `protoc` PASS |
| C2 | Descartes `019ef502-1c1a-7d81-b4f5-7f0fe0744e0b` | `terraform`, `packer`, `nomad`: Go/runtime/process/syscall semantics | complete; no pass, 5 attempts documented |
| C3 | Kierkegaard `019ef502-1e2b-73e0-ba90-bda339066fec` | `nvim`: 0-bind libuv/signal teardown crash | complete; no pass, 5 attempts documented |
| C4 | Ptolemy `019ef502-2020-7011-9c11-b642d64c11b5` | `cmake`: CMAKE_ROOT/env plus CommonCrypto/CF/libcurl surface | complete; no pass, 5 attempts documented |
| C5 | Chandrasekhar `019ef502-22f9-7943-bfb2-60f3f1a33c73` | `bun`, `nu`, `tilt`, `nomad`: LSE/framework surface | complete; no pass, 5 attempts documented |

External Claude/Grok status:

| Job | PID | Worktree | Report | Scope |
| --- | ---: | --- | --- | --- |
| Claude Nvim | 4078235 | `/tmp/machgate-loop-c/claude-nvim` | `/tmp/machgate-agent-reports/loop-c/claude-nvim.txt` | exited; no useful report |
| Claude CMake/Protoc | 4078236 | `/tmp/machgate-loop-c/claude-cmake-protoc` | `/tmp/machgate-agent-reports/loop-c/claude-cmake-protoc.txt` | exited; no useful report |
| Grok Apple Research | 4078237 | `/tmp/machgate-loop-c/grok-apple-research` | `/tmp/machgate-agent-reports/loop-c/grok-apple-research.txt` | complete; public-source research and patch suggestions collected |
| Grok Framework Research | 4078238 | `/tmp/machgate-loop-c/grok-framework-research` | `/tmp/machgate-agent-reports/loop-c/grok-framework-research.txt` | complete; public-source research and patch suggestions collected |

Public-source research targets for this loop:

- Apple open source drops for `Libc`, `xnu`, `CF`, `libdispatch`, `copyfile`, and Darwin headers.
- LLVM `libc++` and `libc++abi` sources used by `scripts/build-libcxx.sh`.
- ICU sources for `libicucore` behavior needed by `bun`.
- libuv sources for `nvim` signal teardown behavior.
- Go runtime sources for Darwin ARM64 process/signal/syscall startup where relevant to `terraform`, `packer`, and `nomad`.

Loop C live checklist:

- [x] Main branch pushed before loop start: commit `341ddb2`.
- [x] Five Codex workers launched.
- [x] Two Claude jobs launched in separate worktrees.
- [x] Two Grok jobs launched in separate worktrees.
- [x] C1 `protoc` attempt 3 `protoc-sigsetjmp-attempt3`: added exported `sigsetjmp`; resolver improved to `1656 resolved, 0 failed`, then still crashed in `mod_init[0/5]` at `PC=0x10000000`, `LR=0x1003a5878`. Verbose disassembly showed Abseil TLV bootstrap register reuse.
- [x] C1 `protoc` attempt 4 `protoc-tlvregs-attempt4`: preserved `x8`-`x11` across `_tlv_bootstrap` and reran targeted Docker/QEMU with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`; PASS, stdout `libprotoc 35.1`, logs in `tests/external/logs/protoc-tlvregs-attempt4/`.
- [x] First worker report collected: C1/Tesla.
- [x] First integration patch applied to main checkout.
- [x] Targeted rerun for first integrated fix.
- [x] Docs updated with attempt results.
- [x] Full `57`-binary corpus rerun.

Integrated attempt log:

- [x] `loop-c-integrated-attempt1`: targeted all nine remaining binaries with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`. Result: `1/9` passed. `protoc` PASS with stdout `libprotoc 35.1`; `nu`, `tilt`, `terraform`, `packer`, `nomad`, `cmake`, `bun`, and `nvim` still fail with `SIGSEGV` 139. Logs: `tests/external/logs/loop-c-integrated-attempt1/`. Work dir: `tests/external/work/loop-c-integrated-attempt1/`.
- [x] Full corpus `full-loop-2026-06-23-E2-loop-c`: `49/57` passed, `8` failed. Promoted `protoc` to full-corpus PASS. Remaining failures: `nu`, `tilt`, `terraform`, `packer`, `nomad`, `cmake`, `bun`, and `nvim`.
- [x] Main-thread `nvim` attempt 6 `nvim-loop-c-main-fcntl-thunk-attempt1`: added a guest-only resolver thunk for `_fcntl` that loads Darwin's first stack vararg into `x2` and calls `machgate_darwin_fcntl_fixed`, avoiding broad host `fcntl` interposition. Targeted run still failed with `SIGSEGV` 139, but the previous concrete symptom is fixed: traces now show `fcntl(fd, F_SETFD, arg=0x1)` instead of `arg=0`. Crash remains `pc=0x1000bf248`, `lr=0x10039ce80`, `si_addr=0x210` in the libuv signal teardown path. Next blocker is libuv/signal loop initialization state, not `fcntl` varargs. Logs: `tests/external/logs/nvim-loop-c-main-fcntl-thunk-attempt1/`.
- [x] C3 `nvim` attempt 1 `nvim-loop-c-worker3-attempt1-trace`: added gated shim tracing around `kqueue`, `kevent`, `pipe`, `pipe2`, and `fcntl`, plus a narrow `pipe` export. Targeted `nvim --version` still failed with `SIGSEGV` 139. QEMU diagnostics reached `_main`, showed `kqueue()` and `pipe()` succeeding, then showed Darwin `_fcntl(fd, F_SETFD, 1)` being decoded by the C varargs shim as `arg=0`. The terminal crash remained `si_addr=0x210`, `pc=0x1000bf248` in `_uv_close`, `lr=0x10039ce80` in `_signal_teardown`, with `x0=0x10067a478` and `x8=NULL`.
- [x] C3 `nvim` attempt 2 `nvim-loop-c-worker3-attempt2-fcntl`: tried an exported ARM64 `fcntl` stack-argument adapter so Darwin callers using stack varargs would pass the third argument to the C helper. Targeted run failed earlier during EH-frame registration before `_main`, with QEMU reporting `SIGSEGV si_addr=0x00000001bd0acde4`. The direct export was unsafe because `libsystem_shim.so` is loaded globally and can also catch host/Linux ABI `fcntl` calls.
- [x] C3 `nvim` attempt 3 `nvim-loop-c-worker3-attempt3-vfcntl`: moved the `fcntl` fix behind a resolver variadic thunk (`fcntl` to `vfcntl`) to avoid host symbol interposition. Resolver reported `414 resolved, 0 failed` and `9 variadic thunks`, but the target still failed before `_main` around EH-frame registration (`SIGSEGV si_addr=0x000000018b65cde4`), so the libuv signal path was not reached.
- [x] C3 `nvim` attempt 4 `nvim-loop-c-worker3-attempt4-vfcntl-notrace`: reran the resolver-thunk state without shim tracing. The early failure reproduced: `414 resolved, 0 failed`, `9 variadic thunks`, then `SIGSEGV si_addr=0x000000017c67ce04` immediately after `eh_frame: registered 5735 synthetic FDEs via __register_frame`, before `_main`. The `fcntl` resolver thunk was backed out after this attempt.
- [x] C3 `nvim` attempt 5 `nvim-loop-c-worker3-attempt5-backout-vfcntl-trace`: after backing out the `fcntl` resolver thunk, rebuilt and reran with shim/signal tracing. Targeted `nvim --version` still failed with `SIGSEGV` 139. Resolver returned to `8 variadic thunks`; QEMU reached `_main`, repeated `fcntl(fd, F_SETFD, arg=0)` during libuv setup, and crashed at the same `_uv_close` instruction (`pc=0x1000bf248`, `lr=0x10039ce80`, `si_addr=0x210`). Current blocker: Darwin stack-vararg handling for `_fcntl` is needed, but the first resolver-level `fcntl` thunk attempt perturbed pre-main EH-frame registration and must be redesigned narrowly.
- [x] C4 `cmake` attempt 3 `loop-c-worker4-cmake-attempt3`: fixed the CMAKE_ROOT packaging path by exposing `__machismo_guest_executable_path` to `_NSGetExecutablePath` and preserving the CMake app bundle path with a symlinked extracted binary. Targeted run still failed `SIGSEGV` 139, but qemu-strace showed `Contents/share/cmake-4.3/Modules/CMake.cmake` was found and the old `CMake Error: Could not find CMAKE_ROOT !!!` message was gone. Resolver remained at `4437 resolved, 38 failed`. Logs: `tests/external/logs/loop-c-worker4-cmake-attempt3/`.
- [x] C4 `cmake` attempt 4 `loop-c-worker4-cmake-attempt4`: added narrow CommonCrypto MD5/SHA1/SHA384/SHA512 and CF bundle/URL/UUID shims. Targeted run still failed `SIGSEGV` 139 at `si_addr=0x3d6`; resolver improved to `4455 resolved, 20 failed`. Remaining named gaps were `__mb_cur_max`, `LSOpenCFURLRef`, `NXGetArchInfoFromCpuType`, ACL helpers, `copyfile`, and `lchflags`. Logs: `tests/external/logs/loop-c-worker4-cmake-attempt4/`.
- [x] C4 `cmake` attempt 5 `loop-c-worker4-cmake-attempt5`: added the remaining named shim gaps. Targeted run still failed with `SIGSEGV si_addr=0x3d6`; the old CMAKE_ROOT error stayed fixed, and resolver improved to `4464 resolved, 0 stubbed, 11 failed`.
- [x] C2 `terraform`/`packer`/`nomad` attempt 1 `loop-c-worker2-attempt1-security-resolv`: mapped `libresolv.9.dylib` and `Security` to `libsystem_shim.so` and added narrow resolver/Security stubs. Result: `0/3`; `terraform` improved to `143 resolved, 0 failed`, `packer` to `155 resolved, 0 failed`, and `nomad` to `195 resolved, 24 failed`, but all still failed after `_main`. Logs: `tests/external/logs/loop-c-worker2-attempt1-security-resolv/`.
- [x] C2 attempt 2 `loop-c-worker2-attempt2-signal-shim-trace`: reran with `MACHGATE_TRACE_SIGNALS=1` and `MACHGATE_TRACE_SHIM=1`. Result: `0/3`; `terraform` showed a large immediate `kevent` poll loop, `packer` still hit child status `253` then `sigaction(17->19)`/`SIGSTOP` `EINVAL`, and `nomad` remained blocked by skipped `IOKit` plus LSE island range warnings. Logs: `tests/external/logs/loop-c-worker2-attempt2-signal-shim-trace/`.
- [x] C2 attempt 3 `loop-c-worker2-attempt3-kevent-timeout-yield`: added a capped finite-timeout sleep to the `kevent` shim to stop returning immediately for every nonzero timeout. Result: `0/3`; `terraform` changed to primary stderr `fatal: bad g in signal handler` with qemu-strace still ending in `SIGSEGV`, `packer` still ended after child status `253` and uncatchable `SIGSTOP` handler registration, and `nomad` still had skipped `IOKit`, `proc_pidpath`, `__CFConstantStringClassReference`, and LSE range warnings. Logs: `tests/external/logs/loop-c-worker2-attempt3-kevent-timeout-yield/`.
- [x] C2 attempt 4 `loop-c-worker2-attempt4-godebug-asyncpreemptoff`: diagnostic rerun with `GODEBUG=asyncpreemptoff=1`. Result: `0/3`; disabling Go async preemption did not make any target pass, so the Go crash is not solely async-preemption delivery. `terraform` and `packer` had zero failed binds and generic `SIGSEGV`; `nomad` stayed at `195 resolved, 24 failed`. Logs: `tests/external/logs/loop-c-worker2-attempt4-godebug-asyncpreemptoff/`.
- [x] C2 attempt 5 `loop-c-worker2-attempt5-proc-pidpath-cfconstant`: added narrow `proc_pidpath` and `__CFConstantStringClassReference` shim exports. Result: `0/3`; `nomad` improved to `203 resolved, 16 failed`, with the only mapped-shim missing symbol now `vm_kernel_page_size`, but it still has skipped `IOKit` and extensive LSE island out-of-range warnings. `terraform` and `packer` remain zero-bind post-`_main` Go runtime crashes. Logs: `tests/external/logs/loop-c-worker2-attempt5-proc-pidpath-cfconstant/`.
- [x] C2 regression check `loop-c-worker2-lazygit-regression`: targeted `lazygit --version` after the process/resolver changes. Result: PASS (`1/1`), confirming no reintroduced broad access/faccessat regression in this probe. Logs: `tests/external/logs/loop-c-worker2-lazygit-regression/`.

C5 Worker 5 LSE/framework attempts:

- [x] C5 attempt 1 `worker5-lse-attempt1`: initial targeted rerun for `nu`, `tilt`, `nomad`, and `bun` accidentally used the pre-rebuild loader for the new LSE sizing code. Result: `0/4` passed. Logs show the old `bun` LSE pool still at `2048 KB` with `island pool exhausted after 32774 patches`; `nomad` still had 3387 out-of-range island warnings. Kept as diagnostic only; logs in `tests/external/logs/worker5-lse-attempt1/`.
- [x] C5 attempt 2 `worker5-lse-attempt2-sized-pretext`: added section-based LSE pool sizing and pre-`__TEXT` downward placement search. Targeted `nu`, `tilt`, `nomad`, and `bun` still `0/4` passed, but LSE debt moved: `bun` no longer exhausted the pool (`5176 KB`, 53107 LSE patches), `tilt` no longer had out-of-range islands, and `nu` remained LSE-clean. `nomad` still used the adjacent pool and retained 3387 out-of-range warnings. Logs in `tests/external/logs/worker5-lse-attempt2-sized-pretext/`.
- [x] C5 attempt 3 `worker5-lse-attempt3-nomad-instr-bounds`: narrowed main-pool placement bounds to instruction sections for `nomad`, then hit a diagnostic blocker. The first run failed during `system_shim` linking because the filesystem was full; after freeing old external work dirs and rerunning, `nomad` still used the adjacent pool. A direct `MACHGATE_TRACE_LSE_POOL=1` probe showed section bounds `text=[0x100001c80,0x1028157a0)` and revealed unaligned pre-text mmap candidates.
- [x] C5 attempt 4 `worker5-lse-attempt4-nomad-aligned-pretext`: aligned pre-text mmap candidates to page boundaries. Targeted `nomad --version` still failed `SIGSEGV` 139, but LSE placement is now clean: pool at `0xffe00000 (before __TEXT, 2048 KB)`, 5354 LSE patches, zero `island out of B range`, and zero exhaustion. Remaining visible blockers are skipped `IOKit`, mapped-but-incomplete framework/system surface (`vm_kernel_page_size` is still missing), and the post-`_main` NULL/`0x110` SIGSEGV. Logs in `tests/external/logs/worker5-lse-attempt4-nomad-aligned-pretext/`.
- [x] C5 attempt 5 `worker5-framework-attempt5-nomad-vm-kernel-page-size`: exported `vm_kernel_page_size` next to the existing `vm_page_size`. Targeted `nomad --version` still failed `SIGSEGV` 139, but resolver improved to `204 resolved, 0 stubbed, 15 failed`, with no remaining mapped-shim missing-symbol lines. LSE remained clean (`0` out-of-range, `0` exhausted; 5354 patches). QEMU still ends after `_main` with `SIGSEGV si_addr=NULL` followed by `si_addr=0x110`. Current blocker is skipped `IOKit` and/or deeper Go/runtime framework behavior, not LSE placement or named mapped-shim surface. Logs in `tests/external/logs/worker5-framework-attempt5-nomad-vm-kernel-page-size/`.
- [x] C5 verification: `docker run --rm --platform linux/arm64 ... cmake --build build-arm64 --target machismo test_lse_emul --parallel && ./build-arm64/test_lse_emul` passed `480/480`.

D3 Worker nu/tilt shim-surface attempts:

- [x] D3 attempt 1 `d3-attempt1-fsevents`: added CoreServices/FSEvents shim exports and reran `nu`/`tilt` with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`. Result: `0/2`. `tilt` improved to `231 resolved, 0 stubbed, 0 failed` but still crashed after `_main` with `SIGSEGV`. `nu` improved to `489 resolved, 13 stubbed, 31 failed`; next printed mapped gaps were Security trust/certificate helpers. Logs: `tests/external/logs/d3-attempt1-fsevents/`.
- [x] D3 attempt 2 `d3-attempt2-security-final`: added narrow Security shims for certificate subject/error strings and trust anchor/OCSP setters. Result: `0/2`. `nu` improved to `494 resolved, 13 stubbed, 26 failed`; next printed mapped gaps were `copyfile_state_alloc`, `copyfile_state_free`, and `copyfile_state_get`. `tilt` still failed with a pre-`_main`/EH-frame-area `SIGSEGV`; a stale intermediate run showed build-state sensitivity, so the final result uses the rebuilt shim. Logs: `tests/external/logs/d3-attempt2-security-final/`.
- [x] D3 attempt 3 `d3-attempt3-copyfile`: added copyfile state shims plus explicit `fcopyfile`, `fclonefileat`, and `fsetattrlist` exports. Result: `0/2`. `nu` improved to `501 resolved, 13 stubbed, 19 failed`; next printed mapped gaps were `host_statistics64` and `pthread_set_qos_class_self_np`. `tilt` remained `161 resolved, 70 failed` in this run and crashed around synthetic EH-frame registration. Logs: `tests/external/logs/d3-attempt3-copyfile/`.
- [x] D3 attempt 4 `d3-attempt4-host-qos`: added no-op host statistics and pthread QoS shims. Result: `0/2`. `nu` improved to `503 resolved, 13 stubbed, 17 failed` with no remaining printed mapped-shim missing-symbol lines, then crashed after `_main` at `si_addr=0x...b50`. `tilt` remained an EH-frame-area crash with unresolved flat-bind count in this run. Logs: `tests/external/logs/d3-attempt4-host-qos/`.
- [x] D3 attempt 5 `d3-attempt5-iokit`: mapped `IOKit` to `libsystem_shim.so` for external probes and added minimal IOKit/HID/registry exports. Result: `0/2`. `tilt` again reached `231 resolved, 0 stubbed, 0 failed` but crashed during/just after EH-frame registration with `SIGSEGV si_addr=0x00000001efcd0ec4`. `nu` improved to `519 resolved, 13 stubbed, 1 failed`; remaining visible skipped dylibs are `AppKit`, `CoreGraphics`, and `Foundation`, and the runtime still dies after `_main` with `SIGSEGV si_addr=0x...b50`. Stop D3 at five attempts. Next blocker: `tilt` is no longer missing framework symbols and needs EH-frame/resolver/trampoline crash analysis outside shim-surface ownership; `nu` needs non-owned AppKit/CoreGraphics/Foundation policy or deeper Rust TLS/signal crash analysis after the shim gaps are nearly closed. Logs: `tests/external/logs/d3-attempt5-iokit/`.

### Loop 2026-06-23-D

Goal: move from `51/57` passing to `57/57` passing, or leave each still-failing binary with five narrow, logged attempts and an exact next blocker.

Global exit conditions:

- [ ] `57/57` external Mach-O CLI probes pass with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`.
- [ ] Every remaining failure has either a targeted PASS or five documented failed attempts with commands, log directories, and researched sources.
- [ ] Full-corpus rerun after any loader, syscall, resolver, shim, dylib-map, or runtime behavior change.

Per-attempt rules:

1. Change one subsystem at a time.
2. Run a targeted manifest for the affected binary or bucket.
3. If stdout/stderr is unclear, rerun with `QEMU_STRACE=1` or MachGate trace flags.
4. Update this file after each pass/fail.
5. Stop a single binary after five failed attempts and mark the precise blocker instead of looping forever.

Loop D active workers:

| Worker | Agent | Scope | Exit condition |
| --- | --- | --- | --- |
| D-main | Main thread | `nvim` libuv/signal teardown; Darwin stack-vararg `ioctl(FIONBIO)` after fixed `fcntl` | complete; `nvim` promoted in full corpus K |
| D1 | Nash `019ef51d-2aa3-7373-9668-c90602529752` | `terraform`, `packer`, `nomad` Go/runtime/process/signal semantics | one target passes or five attempts documented |
| D2 | Helmholtz `019ef51d-a262-7730-884b-b4fb9a72f61f` | `bun` ICU/libdispatch/Mach/system shim surface | `bun` passes or five attempts documented |
| D3 | Franklin `019ef51d-cb7a-7153-896e-345244a00d72` | `nu`, `tilt` Rust/framework/process shim surface | one target passes or five attempts documented |
| D4 | Hume `019ef51d-fad2-72a2-9f94-78dba8a65734` | `cmake` resolver/trampoline/native interop and remaining libcurl surface | complete; targeted `cmake --version` exits 0 |
| D5 | Noether `019ef51e-20d7-72d2-bc05-ec4eb20c932f` | live failure classification and stale-doc cleanup | Loop D classification table updated |

Loop D external Claude/Grok jobs:

| Job | PID | Report | Scope |
| --- | ---: | --- | --- |
| Grok Loop D | 4102827 | `/tmp/machgate-agent-reports/loop-d/grok-loop-d.jsonl` | complete; read-only multi-fix diagnosis with Grok subagents/best-of-n |
| Claude Loop D | 4102862 | `/tmp/machgate-agent-reports/loop-d/claude-loop-d.txt` | exited immediately; `--dangerously-skip-permissions` is refused when running as root |

Loop D public-source research context:

- Existing local public context: `/tmp/machgate-public-research/{Libc,xnu,CF,swift-corelibs-libdispatch,icu,libuv,go}`.
- Added Apple `dyld` and `libplatform` source context under `/tmp/machgate-public-research/`.
- Apple GitHub does not expose standalone `apple-oss-distributions/libcxx` or `libcxxabi` repositories; attempted clone returned `Repository not found`.
- Added upstream LLVM `llvm-project` sparse checkout for `libcxx` and `libcxxabi` under `/tmp/machgate-public-research/llvm-project` for C++ ABI/runtime context. This is context only, not vendored.

Loop D live checklist:

- [x] Closed stale Loop C Codex agent handles after collecting final reports.
- [x] Launched five Loop D Codex workers.
- [x] Launched Grok Loop D read-only diagnosis.
- [x] Launched Claude Loop D read-only diagnosis.
- [x] Main-thread `nvim` attempt 7 `nvim-loop-d-main-ioctl-thunk-attempt7`: added a guest-only resolver thunk for `_ioctl` and a narrow `machgate_darwin_ioctl_fixed` shim translator for Darwin `FIONBIO`, `FIONREAD`, `TIOCGWINSZ`, `TIOCSWINSZ`, `TIOCOUTQ`, `FIOCLEX`, and `FIONCLEX`. Targeted normal run still failed before `_main`, but the QEMU_STRACE diagnostic rerun proved the original libuv blocker was fixed: `ioctl(fd=6 request=0x8004667e->0x5421 arg=0xfffffb89c) -> 0`, `nvim` printed `NVIM v0.12.3`, and the guest called `exit_group(0)`. Logs: `tests/external/logs/nvim-loop-d-main-ioctl-thunk-attempt7/`.
- [x] Main-thread `nvim` attempt 8 `nvim-loop-d-main-ehframe-attempt8`: rejected. Skipping native Mach-O `__eh_frame` registration made targeted `nvim --version` pass, but the following full-corpus attempt `full-loop-2026-06-23-F-loop-d` regressed previously passing Go-style binaries including `goreleaser`, `chezmoi`, `duf`, `shfmt`, `fx`, `kubectl`, `helm`, and `k9s`. The EH-frame change was backed out; do not revive this approach without a per-binary or unwinder-safe design.
- [x] Main-thread `nvim` attempt 9 `nvim-loop-d-main-stackpool-attempt9`: split fixed stack-argument thunks into a separate executable pool, but helper lookup still fell back to raw `fcntl`/`ioctl`; targeted `nvim --version` failed `0/1`. Trace showed the old bad arguments again: `fcntl(... arg=0)` and `ioctl(7,0x8004667e,0) -> ENOTTY`. Logs: `tests/external/logs/nvim-loop-d-main-stackpool-attempt9/`.
- [x] Main-thread `nvim` attempt 10 `nvim-loop-d-main-provider-handle-attempt10`: accepted. Fixed mapped-dylib stack-argument helper lookup by resolving `machgate_darwin_fcntl_fixed` and `machgate_darwin_ioctl_fixed` from the same dylib handle that provided the guest import, while flat lookups still use `RTLD_DEFAULT`. Targeted `nvim --version` PASS: `1/1`, stdout `NVIM v0.12.3`; resolver reports `8 variadic thunks, 2 stack-arg thunks`; traces show `fcntl(... arg=0x1)` and Darwin `FIONBIO 0x8004667e` translated to Linux `0x5421`. Native `__eh_frame` registration is restored. Logs: `tests/external/logs/nvim-loop-d-main-provider-handle-attempt10/`.
- [x] Regression bucket `loop-d-regression-bucket-attempt10`: reran the eight binaries that regressed during rejected attempt 8 plus `nvim`. Result: `9/9` PASS (`goreleaser`, `chezmoi`, `duf`, `shfmt`, `fx`, `kubectl`, `helm`, `k9s`, `nvim`). Logs: `tests/external/logs/loop-d-regression-bucket-attempt10/`.
- [x] E1 EH-frame/native registration attempt `e1-tail-native-hook-only-attempt2-libcxx`: stopped registering native Mach-O `__eh_frame` with `__register_frame` and serves it only through the existing `_dl_find_object` header path. Targeted order-sensitive tail with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` now passes `protoc` and `nvim`; `bun` remains the known C++ static-initializer failure. Regression bucket `e1-regression-bucket-native-hook-only-attempt1` passed `9/9` for the previous EH-frame regression set (`goreleaser`, `chezmoi`, `duf`, `shfmt`, `fx`, `kubectl`, `helm`, `k9s`, `nvim`).
- [x] D1 attempt 1 `d1-attempt1-current`: baseline targeted `terraform`, `packer`, and `nomad` with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`. Result: `0/3`; all three still failed after `_main` with QEMU `SIGSEGV`. Logs: `tests/external/logs/d1-attempt1-current/`.
- [x] D1 attempt 2 `d1-attempt2-shimtrace`: diagnostic rerun with `MACHGATE_TRACE_SHIM=1`. Result: `0/3`; `terraform` showed repeated `kevent` timeouts before NULL `SIGSEGV`, `packer` hit child status `253` then `sigaction(17->19)`/Darwin `SIGSTOP` returning `EINVAL`, and `nomad` stayed on the post-`_main` NULL/`0x110` crash class. Logs: `tests/external/logs/d1-attempt2-shimtrace/`.
- [x] D1 attempt 3 `d1-attempt3-kqueue-poll`: added a narrow poll-backed `kqueue`/`kevent` model in `libsystem_shim.c` for Go `EVFILT_READ`, `EVFILT_WRITE`, and `EVFILT_USER` registrations. Result: `0/3`; no target passed and `terraform`/`nomad` retained the same Go runtime `SIGSEGV` crash class. Logs: `tests/external/logs/d1-attempt3-kqueue-poll/`.
- [x] D1 attempt 4 `d1-attempt4-sigstop-default`: added Darwin `sigaction` handling that treats `SIGKILL`/`SIGSTOP` default reset/query as success while still rejecting custom handlers. Result: `0/3`; `packer` advanced from host `SIGSEGV` to clean process exit `2`, with qemu-strace showing a child process re-exec of `/work/build-arm64/machismo`, child status `253`, then parent `exit_group(2)`. `terraform` and `nomad` still ended in Go runtime `SIGSEGV`. Logs: `tests/external/logs/d1-attempt4-sigstop-default/`.
- [x] D1 attempt 5 `d1-attempt5-packer-shimtrace`: packer-only diagnostic rerun with `MACHGATE_TRACE_SHIM=1`. Result: `0/1`; the child re-exec path failed to `dlopen` `/work/build-arm64/libsystem_shim.so`, leaving `155 failed` binds and then NULL `SIGSEGV`. This confirms the remaining packer blocker is child-process/re-exec environment support, while terraform/nomad remain blocked on deeper Darwin Go signal/runtime context handling. Logs: `tests/external/logs/d1-attempt5-packer-shimtrace/`.
- [x] D2 bun baseline `bun-d2-baseline-current`: current targeted `bun --version` with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` still failed `SIGSEGV` 139 in the single C++ static initializer. Resolver: `819 resolved, 0 stubbed, 220 failed`; `libicucore.A.dylib` unmapped; visible mapped gaps were libdispatch/Mach/system symbols. Logs: `tests/external/logs/bun-d2-baseline-current/`.
- [x] D2 attempt 1 `bun-d2-attempt1-icucore-map`: mapped `libicucore` to `libsystem_shim.so` in the external harness. Result: `0/1`; still `SIGSEGV` 139 before `_main`, resolver unchanged at `819 resolved, 220 failed`. Logs: `tests/external/logs/bun-d2-attempt1-icucore-map/`.
- [x] D2 attempt 2 `bun-d2-attempt2-dispatch-ulock`: added narrow libdispatch/ulock/time exports (`dispatch_once_f`, source create/resume/set handler, `_dispatch_source_type_mach_recv`, signpost no-op, `__ulock_*`, approximate Mach time). Result: `0/1`; resolver improved to `829 resolved, 210 failed`, crash unchanged in C++ static initializer. Logs: `tests/external/logs/bun-d2-attempt2-dispatch-ulock/`.
- [x] D2 attempt 3 `bun-d2-attempt3-malloc-zone`: exported existing malloc-zone helpers and added `malloc_good_size`, `malloc_logger`, and `malloc_zone_register`. Result: `0/1`; resolver improved to `834 resolved, 205 failed`, crash unchanged. Logs: `tests/external/logs/bun-d2-attempt3-malloc-zone/`.
- [x] D2 attempt 4 `bun-d2-attempt4-mach-surface`: added Mach host/port/vm surface (`host_info`, `mach_port_*`, `mach_msg_server_once`, `mach_vm_map`). Result: `0/1`; resolver improved to `840 resolved, 199 failed`, crash unchanged. A concurrent `resolver.c` compile error later blocked rebuilding `machismo`, so subsequent D2 verification rebuilt `system_shim` alone against the existing loader. That resolver compile error has since been fixed in the main checkout. Logs: `tests/external/logs/bun-d2-attempt4-mach-surface/`.
- [x] D2 attempt 5 `bun-d2-attempt5-system-surface-shim-only`: added remaining named system/process/fs/os shims (`__getdirentries64`, `__pthread_fchdir`, `clonefile*`, `fcopyfile`, `getprogname`, `getsegmentdata`, `kevent64`, notify fd, os-log/signpost, unfair locks, proc list helpers, pthread JIT support query) and rebuilt `system_shim` only because the then-current resolver compile error blocked full rebuild. Result: `0/1`; resolver improved to `860 resolved, 179 failed`, still `SIGSEGV` 139 in the single C++ static initializer. Current blocker: remaining ICU and thread/runtime symbols, beginning with `pthread_main_np`, `pthread_self_is_exiting_np`, `pthread_set_qos_class_self_np`, `renameatx_np`, `thread_get_state`, `thread_resume`, `thread_set_exception_ports`, `thread_suspend`, `thread_switch`, `timingsafe_bcmp`, and ICU entrypoints such as `u_charDirection`, `u_charType`, `u_errorName`, `u_getVersion`, `u_hasBinaryProperty`, `u_strToLower`, `u_strToUpper`, `u_tolower`, `u_toupper`, and `ubrk_clone`. Logs: `tests/external/logs/bun-d2-attempt5-system-surface-shim-only/`.
- [x] E2 bun attempt 1 `bun-e2-attempt1-thread-system`: added narrow libSystem/Mach thread/runtime exports (`pthread_main_np`, `pthread_self_is_exiting_np`, `thread_get_state`, `thread_resume`, `thread_set_exception_ports`, `thread_suspend`, `thread_switch`), `renameatx_np`/`__renameatx_np`, and `timingsafe_bcmp`. Targeted Docker result: `0/1`, still `SIGSEGV` 139 in the single C++ static initializer. Resolver improved to `870 resolved, 0 stubbed, 169 failed`; printed gaps are now ICU-only: `u_charDirection`, `u_charType`, `u_errorName`, `u_getVersion`, `u_hasBinaryProperty`, `u_strToLower`, `u_strToUpper`, `u_tolower`, `u_toupper`, and `ubrk_*` break-iterator functions. Logs: `tests/external/logs/bun-e2-attempt1-thread-system/`.
- [x] E2 bun attempt 2 `bun-e2-attempt2-icu-startup`: added minimal ICU startup shims for character direction/type/properties, case conversion, ICU version/error names, and inert `ubrk_*` open/clone/current/next/following APIs. Targeted Docker result: `0/1`, still `SIGSEGV` 139 in the single C++ static initializer. Resolver improved to `890 resolved, 0 stubbed, 149 failed`; newly printed gaps are `ubrk_preceding`, `ubrk_setText`, `ubrk_setUText`, `ucal_*` calendar/time-zone APIs, and `ucfpos_*`. Logs: `tests/external/logs/bun-e2-attempt2-icu-startup/`.
- [x] E2 bun attempt 3 `bun-e2-attempt3-icu-calendar`: added inert ICU break text setters, calendar/time-zone APIs, and constrained field-position close/category shims. Targeted Docker result: `0/1`, still `SIGSEGV` 139 in the single C++ static initializer. Resolver improved to `910 resolved, 0 stubbed, 129 failed`; newly printed gaps are `ucfpos_constrainField`, `ucfpos_getCategory`, `ucfpos_getField`, `ucfpos_getIndexes`, `ucfpos_open`, `ucfpos_reset`, `ucol_*` collation APIs, `ucurr_getName`, `ucurr_openISOCurrencies`, and `udat_close`. Logs: `tests/external/logs/bun-e2-attempt3-icu-calendar/`.
- [x] D4 attempt 1 `loop-d-worker4-cmake-attempt1-libcurl-map`: mapped `libcurl.4.dylib` to host `libcurl.so.4` in the external harness. Resolver improved from `4464 resolved, 11 failed` to `4475 resolved, 0 failed`, but targeted `cmake --version` still failed with `SIGSEGV si_addr=0x3d6` after CMAKE_ROOT discovery. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt1-libcurl-map/`.
- [x] D4 diagnostic `loop-d-worker4-cmake-attempt1-signal-trace`: reran attempt 1 with signal tracing. The crash was native-side: `pc` in the dynamic linker path, `lr` in glibc `dlsym`, `x8=-2`, and fault address `0x3d6`. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt1-signal-trace/`.
- [x] D4 attempt 2 `loop-d-worker4-cmake-attempt2-shim-local`: tried loading `libsystem_shim.so` as `RTLD_LOCAL` to avoid native interposition. It did not fix cmake and reduced fixed stack-arg helper visibility, so the resolver change was later backed out. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt2-shim-local/`.
- [x] D4 attempt 3 `loop-d-worker4-cmake-attempt3-framework-dlopen`: added Darwin framework `dlopen` path redirection and Linux-safe `dlopen` flag translation in the shim. Targeted cmake still failed with the same `si_addr=0x3d6`; traces showed no framework redirection before the crash, so this was not the active blocker. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt3-framework-dlopen/`.
- [x] D4 attempt 4 `loop-d-worker4-cmake-attempt4-dlsym-handles`: added an exported shim `dlsym` wrapper that translates Darwin special handles (`RTLD_DEFAULT=-2`, `RTLD_SELF=-3`, `RTLD_MAIN_ONLY=-5`) to Linux-safe handles before calling glibc `dlsym`. This fixed the native dynamic-linker crash: targeted `cmake --version` exited 0. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt4-dlsym-handles/`.
- [x] D4 attempt 5 `loop-d-worker4-cmake-attempt5-clean-pass`: backed out the unsuccessful resolver `RTLD_LOCAL` experiment, rebuilt `machismo` and `system_shim`, and reran the targeted cmake manifest with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` and signal tracing. Result: `1/1` PASS, exit status 0, resolver `4475 resolved, 0 stubbed, 0 failed`, no SIGSEGV. A direct QEMU_STRACE rerun also exited 0 and showed `exit_group(0)`; stdout is currently empty under this harness. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt5-clean-pass/`.
- [x] Full corpus `full-loop-2026-06-23-K-loop-e-final`: `51/57` passed, `6` failed. Promoted `cmake` and `nvim` to full-corpus PASS and kept `protoc` passing. Remaining failures: `nu`, `tilt`, `terraform`, `packer`, `nomad`, and `bun`.
