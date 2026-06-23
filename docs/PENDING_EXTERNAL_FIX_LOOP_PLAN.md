# Pending External Mach-O Fix Loop Plan

This plan is for finishing the currently non-passing external ARM64 macOS CLI binaries.

## Current Baseline

Last refreshed: 2026-06-23.

- Corpus: 57 unique external ARM64 macOS CLI binaries
- Passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 48 / 57
- Not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 9 / 57
- Passing without libc++ opt-in: 45 / 57
- Not passing without libc++ opt-in: 12 / 57
- Previous full corpus before `sqlite3` fix: 44 / 57
- Canonical manifest: 19 / 19 passing
- Latest full-run logs: `tests/external/logs/full-loop-2026-06-23-D-libcxx/`
- Latest full-run work dir: `tests/external/work/full-loop-2026-06-23-D-libcxx/`
- Agent status: no active local Codex, Grok, or Claude helper agents in this loop.

Current non-passing binaries:

`nu`, `tilt`, `terraform`, `packer`, `nomad`, `cmake`, `bun`, `protoc`,
`nvim`.

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
- [x] `nomad`: HARDER-5. Attempt 5 resolved the narrow CoreFoundation gaps `CFDictionaryAddValue` and `CFStringGetSystemEncoding`, but the binary still exits 139 after `_main`. Current blockers are skipped `IOKit`, `Security`, and `libresolv.9.dylib` imports plus many LSE island out-of-B-range warnings; no remaining mapped CoreFoundation missing-symbol lines are visible.

Plan:

- [x] Reproduce with targeted manifests.
- [x] Locate syscalls/errno immediately before panic/crash.
- [~] Fix Darwin-to-Linux errno/syscall behavior if wrong; no attempt-4 syscall patch was clear enough to apply without crossing into shim or child-process ownership.
- [x] Rerun the group after each patch.

### Queue D: Missing Shim / Dylib Surface

Owner scope: `src/shim/libsystem_shim.c`, `src/shim/libsystem_shim.ver`, dylib map/test config only.

- [~] `nu`: low-risk CommonCrypto/CoreFoundation shim surface added through attempt 3. `full-loop-2026-06-23-C` still reports `SIGSEGV` after `_main` and 46 failed binds. Classification from the bind/lazy-bind tables: 36 skipped/no-map framework/dylib bind instances (`AppKit` 1 `_NSPasteboardTypeString`, `CoreServices` 6 `FSEventStream*`, `IOKit` 16 HID/registry/default-port symbols, `Security` 10 certificate/trust symbols, `libiconv` 3 conversion symbols) plus 10 mapped `libSystem` gaps (`copyfile_state_alloc`, `copyfile_state_free`, `copyfile_state_get`, `fclonefileat`, `fcopyfile`, `fsetattrlist`, `host_statistics64`, `proc_listallpids`, `proc_pidpath`, `pthread_set_qos_class_self_np`). `CoreGraphics` and `Foundation` are skipped/no-map dylibs in the load list but have no visible bind/lazy-bind symbols in `nu`. No remaining `CoreFoundation` imports are missing from the shim.
- [~] `tilt`: attempt 2 with opt-in Apple-ABI `libc++.1.dylib` mapping still `SIGSEGV si_addr=NULL` after `_main`. Failed binds dropped to 27, with `Security` and `libresolv.9.dylib` still skipped and only `CoreServices` math mapped. Remaining blocker is non-libc++ framework/system surface, not the C++ runtime.
- [~] `cmake`: attempt 2 with opt-in Apple-ABI `libc++.1.dylib` mapping reaches `_main`, then prints `CMake Error: Could not find CMAKE_ROOT !!!` and crashes with `SIGSEGV si_addr=0x3d6`. Failed binds dropped to 38; remaining visible blockers are skipped `libcurl.4.dylib`, CommonCrypto MD5/SHA1/SHA384/SHA512, CF bundle/URL/UUID helpers, `__mb_cur_max`, and `LSOpenCFURLRef`.
- [~] `bun`: attempt 2 with opt-in Apple-ABI `libc++.1.dylib` mapping still `SIGSEGV si_addr=NULL` in the single C++ static initializer. `libicucore.A.dylib` remains unmapped, LSE island pool still exhausts after 32774 patches, and resolver reports 222 failed binds from ICU/system/libdispatch/Mach/copyfile-style surface.
- [~] `protoc`: attempt 2 with opt-in Apple-ABI `libc++.1.dylib` mapping reduced failed binds from 1510 to 1 (`sigsetjmp` from `libSystem.B.dylib`) but still `SIGSEGV si_addr=0x10000000` while running 5 static constructors. Stop at 2/5 for now because the next visible blocker is libSystem/shim or constructor-state behavior, not missing libc++ mapping.
- [~] `nvim`: attempt 4 still `SIGSEGV si_addr=0x210`, but skipped dylib surface is no longer the blocker. `nvim-libiconv-attempt2` mapped `libiconv.2.dylib` to `libc.so.6` and reduced failed binds from 7 to 4. `nvim-coreservices-attempt3` mapped `CoreServices` to `libm.so.6` and added `LocaleRefGetPartString`, reducing failed binds to 0. `nvim-trace-attempt4` shows the crash in `_uv_close` from `_signal_teardown`: PC `0x1000bf248`, LR `0x10039ce80`, faulting on `[NULL + 0x210]`. `libutil.dylib` imports no visible symbols, so no libutil shim was added.

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

### Current Nine-Binary Queue

These are the only binaries still failing after `full-loop-2026-06-23-D-libcxx`:

| Binary | Latest class | Next owner |
| --- | --- | --- |
| `nu` | SIGSEGV after `_main`; 43 failed binds remain, mostly FSEvents/Security/IOKit/libSystem process-copyfile surface | shim/framework surface plus Rust TLS crash analysis |
| `tilt` | SIGSEGV; 27 failed binds after libc++ mapping, with skipped `libresolv.9.dylib` and `Security` | resolver/shim surface |
| `terraform` | SIGSEGV after `_main`; 10 failed binds from skipped `libresolv.9.dylib` and `Security` | Go runtime/process/syscall semantics |
| `packer` | SIGSEGV after `_main`; 10 failed binds from skipped `libresolv.9.dylib` and `Security` | Go runtime/process/syscall semantics |
| `nomad` | SIGSEGV after `_main`; 34 failed binds plus LSE island out-of-range warnings | skipped dylib surface plus LSE placement |
| `cmake` | reaches `_main`, prints missing `CMAKE_ROOT`, then SIGSEGV; 38 failed binds | environment/argv packaging plus CommonCrypto/CF/libcurl surface |
| `bun` | SIGSEGV in C++ static initializer; ICU unmapped, LSE pool exhausted, 222 failed binds | ICU mapping, LSE capacity, libdispatch/Mach surface |
| `protoc` | SIGSEGV during static constructors; only 1 failed bind: `sigsetjmp` | narrow libSystem shim, then constructor-state rerun |
| `nvim` | SIGSEGV with 0 failed binds; crash in libuv signal teardown | runtime state/signal teardown |
