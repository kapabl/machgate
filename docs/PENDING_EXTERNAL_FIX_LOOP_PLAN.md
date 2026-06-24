# Pending External Mach-O Fix Loop Plan

This plan is for finishing the currently non-passing external ARM64 macOS CLI binaries.

## Current Baseline

Last refreshed: 2026-06-23.

- Corpus: 57 unique external ARM64 macOS CLI binaries
- Latest strict full run with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 51 / 57
- Latest functional count after classifying `node` timeout plus targeted `tilt`, `bun`, and `nu`: 55 / 57
- Strict full-run non-passes: 6 / 57
- Functional blockers after `node` timeout plus targeted `tilt`, `bun`, and `nu` classification: 2 / 57
- Previous passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 49 / 57
- Previous not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 8 / 57
- Passing without libc++ opt-in: 45 / 57
- Not passing without libc++ opt-in: 12 / 57
- Previous full corpus before `sqlite3` fix: 44 / 57
- Canonical manifest: 19 / 19 passing
- Latest strict full-run logs: `tests/external/logs/full-loop-2026-06-23-N-loop-g-current/`
- Latest strict full-run work dir: `tests/external/work/full-loop-2026-06-23-N-loop-g-current/`
- `node` timeout classification logs: `tests/external/logs/loop-g-node-long120/`
- `node` Loop H strict cleanup logs: `tests/external/logs/loop-h-node-strict30-attempt1/`
- `node` Loop H 120s confirmation logs: `tests/external/logs/loop-h-node-long120-after-surface/`
- `bun` Loop H TPIDR fix logs: `tests/external/logs/bun-loop-h-attempt1-tpidr-rt/`
- `bun` Loop H guard logs: `tests/external/logs/bun-loop-h-tpidr-guards/`
- `packer` Loop I raw write guard logs: `tests/external/logs/loop-i-packer-raw-write-guard-attempt4/`
- `nu` Loop I TLV confirmation after Delta backout: `tests/external/logs/loop-i-nu-tlv-x1-post-delta-backout/`
- `delta` Loop I failed attempts:
  - `tests/external/logs/loop-i-delta-munmap-noeintr-attempt1-*/`
  - `tests/external/logs/loop-i-delta-small-mmap-shim-attempt2-*/`
  - `tests/external/logs/loop-i-delta-proc-readlink-attempt3-*/`
- Loop I Grok reports:
  - `/tmp/machgate-agent-reports/loop-i/grok-packer-bestof9-rerun.txt`
  - `/tmp/machgate-agent-reports/loop-i/grok-nu-bestof6-rerun.txt`
  - `/tmp/machgate-agent-reports/loop-i/grok-delta-bestof6-rerun.txt`
  - `/tmp/machgate-agent-reports/loop-i/grok-packer-post-sigpipe-bestof9.txt`
- Loop J Grok reports:
  - `/tmp/machgate-agent-reports/loop-j/grok-delta-bestof6.txt`
  - `/tmp/machgate-agent-reports/loop-j/grok-packer-post-guestpath-bestof4.txt`
- Agent status: Loop G/H/I Codex workers complete; Loop J Delta explorers complete; Loop J Grok reports collected.

Current strict full-run non-passes:

`delta`, `nu`, `tilt`, `packer`, `node`, `bun`.

Current functional blockers, excluding the classified `node` timeout artifact and targeted `tilt` / `bun` / `nu` passes:

`delta`, `packer`.

Loop G promoted `terraform` to the full-corpus passing set. Loop H has targeted `tilt` and `bun` passes pending full-corpus promotion. Loop I has a targeted `nu` pass pending full-corpus promotion. `nomad`, `cmake`, `nvim`, and `protoc` remain passing.

## Loop 2026-06-24-J Active Status

Started: 2026-06-24.

Owner scope: final functional blockers `delta` and `packer`.

Loop J agent/research findings:

- Delta Codex explorers converged on the helper-thread `find_calling_pr` process-scan teardown path. Evidence still points to a helper-side `mmap(NULL, 384)` / `munmap(addr, 4096)` sequence, followed by `EINTR`, target `SIGSEGV`, and launcher re-raise.
- Delta binding analysis found `_mmap`, `_munmap`, `_read`, `_readlink`, and `_sigaltstack` import/lazy-stub evidence, but `MACHGATE_TRACE_SHIM=1` showed the failing `mmap(384)` / `munmap(4096)` pair is not entering the shim wrapper.
- Grok Delta best-of-6 recommended a shared sub-page VM tracker. Implemented attempt did not satisfy the 5/5 gate.
- Grok Packer best-of-4 confirmed the guest-path reexec patch moved Packer past the earlier non-Mach-O rejection: qemu-strace now shows host `execve("/work/build-arm64/machismo", {machismo, packer, --version})` and a second config load. Packer still hangs/exits status 2 after runtime thread/netpoll activity.

Loop J Packer attempts:

- [x] `loop-j-packer-guest-path-attempt1`: added guest-path normalization for `/proc/self/exe` / loader path in `src/syscall/execve_reexec.c`, fork-safe env snapshot injection, and raw `DARWIN_PROC_PIDPATHINFO` self-path alignment. Result: targeted Packer still `0/1`, status 2, but the old fork child path rejection is gone. Evidence: child host `execve` of `machismo` with the Packer guest path and second `machismo: loaded config`. Logs: `tests/external/logs/loop-j-packer-guest-path-attempt1/`.

Loop J Delta attempts:

- [x] `loop-j-delta-shim-exports-attempt1..5`: exported/import-wired `read`, `readlink`, `readlinkat`, `munmap`, and `mprotect`; added basic shim `munmap` retry. Result: `1/5`, rejected. Failing traces still reached helper `munmap(...,4096) = EINTR`.
- [x] `loop-j-delta-lifecycle-attempt1..5`: tested exported pthread lifecycle registry/barrier for `find_calling_pr` before main `sigaltstack(SS_DISABLE)`. Result: `0/5`, rejected and backed out.
- [x] `loop-j-delta-altstack-guard-attempt1..5`: tested skipping the Rust altstack guard-page `mprotect(PROT_NONE)`. Result: `0/5`, rejected and backed out.
- [x] `loop-j-delta-tracked-munmap-attempt1..5`: added shared `guest_vm_track` helper for raw/shim `mmap` and `munmap` sub-page tracking. Result: `1/5`, rejected as currently wired. `MACHGATE_TRACE_SHIM=1` showed the failing `mmap(384)` / `munmap(4096)` pair is not entering the shim path, and qemu-strace still showed host `munmap(...)=EINTR`.
- [x] `loop-j-delta-proc-filter-attempt1..5`: filtered imported Darwin `readlink` for `/proc/<other-pid>/exe`. Result: `0/5`, rejected.

Loop J stop rule status:

- Delta has not met the 5/5 targeted gate. Do not increase repeat N for any rejected candidate; first find why the helper scratch allocation bypasses both the shim and raw Darwin syscall tracker.
- Packer next step is a fork-safe trace inside `machgate_execve_macho_guest_forksafe()` or a minimal fixture for `execve("/proc/self/exe")` after fork, because guest-path reexec now starts but does not complete the Packer command.

Loop K tracing note:

- [x] `loop-k-delta-strace-direct`: tried direct `strace -ff -tt -T -s 256 -yy` around `delta --version` inside the ARM64 Docker/QEMU toolchain. Host-style `strace` is not usable in this binfmt/QEMU-user environment: even `strace /bin/true` fails with `PTRACE_TRACEME: Function not implemented`, including with `--cap-add=SYS_PTRACE --security-opt seccomp=unconfined`. QEMU guest tracing still reproduces the failure. Logs: `tests/external/logs/loop-k-delta-strace-direct/`.

## Loop 2026-06-23-H Node Status

Started: 2026-06-23.

Owner scope: node missing shim/dylib surface and strict timeout in `src/shim/libsystem_shim.c`, `src/shim/libsystem_shim.ver`, and this plan file only.

Missing node binds from `full-loop-2026-06-23-N-loop-g-current` were classified as:

- Safe constants / passive objects: `kSecClass`, `kSecClassCertificate`, `kSecMatchLimit`, `kSecMatchLimitAll`, `kSecPolicyAppleSSL`, `kSecPolicyOid`, `kSecReturnRef`.
- Safe no-op / pure failure wrappers: `SecItemCopyMatching`, `SecPolicyCopyProperties`, `SecTrustSettingsCopyTrustSettings`, `getsectdata`, `getsectiondata`, `os_log_type_enabled`.
- Safe host-backed wrapper: `getpeereid` via Linux `SO_PEERCRED`.
- Direct low-risk Mach VM wrappers: `vm_allocate` as anonymous `mmap`, `vm_protect` as `mprotect`.
- Unsafe real Mach semantics, exported only as explicit failure: `mach_make_memory_entry_64`, `mach_vm_remap`, `vm_remap`.

Loop H node attempts:

- [x] `loop-h-node-strict30-attempt1`: rebuilt `machismo` / `system_shim` and ran targeted `node --version` with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` and strict `MACHGATE_EXTERNAL_TIMEOUT=30`. Result: `0/1`, status 124. The log stops during chained fixup processing after `resolver: segment 2...`; it does not reach the resolver summary or `_main`.
- [x] `loop-h-node-long120-after-surface`: same manifest with `MACHGATE_EXTERNAL_TIMEOUT=120`. Result: `1/1`, output `v26.3.1`. Resolver is now clean: `4554 binds resolved, 0 stubbed, 0 failed`. This confirms the Loop H shim-surface pass removed the 19 node failed binds without fixing the strict 30s timeout.

Current node blocker: startup cost under the strict 30s external timeout, not missing shim surface or a SIGSEGV. The visible expensive phases remain large chained fixup/rebase processing, EH-frame synthesis/registration, and gdb JIT symbol generation for the large node image.

## Loop 2026-06-23-H Bun Status

Started: 2026-06-23.

Owner scope: bun clean-import static initializer crash.

Loop H bun attempts:

- [x] `bun-loop-h-attempt1-tpidr-rt`: reviewed Loop G bun logs plus `full-loop-2026-06-23-N-loop-g-current/bun.*`. `__init_offsets` contains one entry, `0x007dfa84`, so the constructor entry is `0x1007dfa84`. The fault at fileoff `0x7d0a5c` is inside that constructor path, not at entry. Disassembly around `0x1007d0a5c` shows an inlined lock/runtime path reading `TPIDRRO_EL0` into `x9`, aligning it, then executing `ldr x20, [x9]`. Existing loader rewriting only matched `MRS TPIDRRO_EL0, x0` (`0xd53bd060`), while bun uses other destination registers such as `x8`, `x9`, and `x10`. Patched the loader to rewrite the whole `MRS TPIDRRO_EL0, Xt` encoding family to `MRS TPIDR_EL0, Xt` while preserving `Rt`. Targeted `bun --version` with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`, `MACHGATE_TRACE_SIGNALS=1`, and `MACHGATE_TRACE_LCMAIN=1` passed `1/1`, output `1.3.14`; resolver stayed clean at `1039 resolved, 0 stubbed, 0 failed`, the loader logged `rewrote 234 Darwin TPIDRRO reads to Linux TPIDR reads`, static initializers completed, and `_main` was reached. Logs: `tests/external/logs/bun-loop-h-attempt1-tpidr-rt/`.
- [x] `bun-loop-h-tpidr-guards`: requested guard bucket after the shared loader fix kept `cmake`, `node`, `protoc`, and `nvim` passing at `4/4` with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` and `MACHGATE_EXTERNAL_TIMEOUT=120`. Outputs included `node` `v26.3.1`, `protoc` `libprotoc 35.1`, and `nvim` `NVIM v0.12.3`. Logs: `tests/external/logs/bun-loop-h-tpidr-guards/`.

Current bun blocker: none in targeted testing. Full corpus has not yet been rerun, so the last strict full-run record still contains the old `SIGSEGV` in the single static initializer.

## Loop 2026-06-23-G Active Status

Started: 2026-06-23 12:29 PDT.

Goal: recover from the Loop F `delta` regression, then continue reducing the six hard external failures.

Latest full-corpus baseline is `full-loop-2026-06-23-N-loop-g-current`: 51 / 57 passing at the strict 30s timeout. `node` passes in the targeted 120s rerun, and Loop H targeted runs moved `tilt` and `bun` to pending-promotion passes, so the functional blocker list is three binaries.

Active local Codex workers:

- [x] `delta` stability / pthread-HOME-procfs: agent `019ef5f4-9636-7932-9310-0b18032e51aa` (`Ohm the 2nd`); not fixed, current blocker is helper-thread teardown around `munmap` / `sigaltstack`.
- [x] `tilt` + `nu` Darwin TLS/TSD compatibility: agent `019ef5f4-b547-7471-ac85-e782cbca56e1` (`Cicero the 2nd`); original Go cgo TSD abort moved forward, remaining tilt blocker is signal/thread startup.
- [x] `terraform` `getaddrinfo` Darwin layout crash: agent `019ef5f4-d853-7561-a737-abc0da729d36` (`Boyle the 2nd`); fixed and passing in the full corpus.
- [x] `packer` child `execve` / re-exec behavior: agent `019ef5f4-f66e-7ef2-b0af-c13c071148ed` (`Avicenna the 2nd`); raw re-exec primitive works. Loop I moved the failure past child status 13/SIGPIPE; packer still exits 2 later in process/wait/runtime timing.
- [x] `bun` ICU/static initializer crash: agent `019ef5f5-1690-7823-9cea-d8fd3fa1e3a5` (`Lagrange the 2nd`); ICU import surface is clean, remaining blocker is constructor/runtime behavior.

Active report-only Grok helpers:

- [x] `delta`: PID `9470`, output `/tmp/machgate-agent-reports/loop-g/grok-delta.txt`.
- [x] `tilt` + `nu` TLS/TSD: PID `9472`, output `/tmp/machgate-agent-reports/loop-g/grok-tls.txt`.
- [x] `terraform`: PID `9475`, output `/tmp/machgate-agent-reports/loop-g/grok-terraform.txt`.
- [x] `packer` + `bun`: PID `9478`, output `/tmp/machgate-agent-reports/loop-g/grok-packer-bun.txt`.

Active report-only Claude helpers:

- [x] `delta`: PID `14975`, output `/tmp/machgate-agent-reports/loop-g/claude-delta.txt`; blocked by Claude local session cap until 1pm PDT.
- [x] `tilt` + `nu` TLS/TSD: PID `14977`, output `/tmp/machgate-agent-reports/loop-g/claude-tls.txt`; blocked by Claude local session cap until 1pm PDT.
- [x] `terraform`: PID `14980`, output `/tmp/machgate-agent-reports/loop-g/claude-terraform.txt`; blocked by Claude local session cap until 1pm PDT.
- [x] `packer` + `bun`: PID `14983`, output `/tmp/machgate-agent-reports/loop-g/claude-packer-bun.txt`; blocked by Claude local session cap until 1pm PDT.

Loop G exit conditions:

- [ ] `delta` passes at least 5/5 targeted repeats after any accepted fix.
- [ ] Any binary-specific fix is validated by its targeted Docker/QEMU probe.
- [ ] If a fix changes shared loader/syscall/shim behavior, run a regression bucket before accepting it.
- [ ] Before commit/push, run the full 57-binary external corpus plus standard unit tests.
- [ ] If a binary reaches 5 failed fix attempts in this loop, mark it `HARDER-5` with exact logs and blocker.

Loop G bun attempts:

- [x] `bun-loop-g-attempt1-icu-final-surface`: reviewed Loop F F1 logs and Grok F1 report, classified the remaining 25 failures as bounded ICU handle/string/close surface rather than real ICU data tables, and added minimal `unumsys_*`, `uplrules_*`, `ureldatefmt_*`, `ures_*`, and `utext_*` exports in `src/shim/libsystem_shim.c` / `.ver`. Targeted Docker/QEMU command rebuilt `machismo system_shim` and ran the pinned `bun --version` manifest with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`. Result: `0/1`; resolver improved from `1014 resolved, 0 stubbed, 25 failed` to `1039 resolved, 0 stubbed, 0 failed`, but bun still `SIGSEGV` 139 in the single C++ static initializer. Logs: `tests/external/logs/bun-loop-g-attempt1-icu-final-surface/`.
- [x] `bun-loop-g-attempt2-icu-clean-signal`: reran the same targeted manifest with `MACHGATE_TRACE_SIGNALS=1` and `MACHGATE_TRACE_LCMAIN=1`. Result: `0/1`; resolver stayed clean at `1039 resolved, 0 stubbed, 0 failed`. Exact first fault: `pc=0x1007d0a5c`, `addr=NULL`, `segment=__TEXT`, `section=__text`, `fileoff=0x7d0a5c`, `insn=0xf9400134` (`ldr x20, [x9]`) with `x9=NULL`, during the single `__init_offsets` initializer. Current bun blocker is no longer missing ICU symbols; next owner should diagnose the stripped constructor/runtime object state at `0x1007d0a5c` before adding more stubs. Logs: `tests/external/logs/bun-loop-g-attempt2-icu-clean-signal/`.

Loop G tilt/nu TLS attempts:

- [x] `loop-g-tilt-nu-attempt1-tsd-flat`: reviewed Loop F `tilt` and `nu` diagnostics plus the Go Darwin arm64 runtime TLS source. The old Go `runtime/cgo` path creates a pthread key, stores magic `0xc476c475c47957`, then scans the TPIDRRO-visible TSD vector. The active gap was not a failed bind; flat-namespace imports for Darwin `pthread_key_create` / `pthread_setspecific` / `pthread_getspecific` were resolving through `RTLD_DEFAULT` to glibc before the mapped libSystem shim, so the magic never reached the shim mirror. Added flat-lookup preference for mapped libSystem runtime symbols and `pthread_getspecific` trace. Targeted Docker/QEMU result: `0/2`; `tilt` moved past the original TSD abort and logged `pthread_key_create -> 128`, `pthread_setspecific(128, 0xc476c475c47957)`, then failed later with Go `runtime: cannot allocate memory`; `nu` remained the Rust TLS `SIGSEGV`. Logs: `tests/external/logs/loop-g-tilt-nu-attempt1-tsd-flat/`.
- [x] `loop-g-tilt-nu-attempt2-flat-runtime`: extended the same flat-namespace libSystem preference to Darwin `mmap` / `munmap` / `mprotect`, after QEMU showed Go's allocator calling glibc `mmap` with Darwin `MAP_ANON` bits and receiving `EBADF`. Targeted Docker/QEMU result: `0/2`; `tilt` advanced through multiple successful shim `mmap` calls, then hit a later Go abort around signal setup after `rt_sigprocmask(..., sigsetsize=8) = EINVAL`; `nu` remained unchanged at the Rust TLS store fault with clean imports. Logs: `tests/external/logs/loop-g-tilt-nu-attempt2-flat-runtime/`.
- [x] `loop-g-tilt-attempt3-flat-signals`: extended the same flat-namespace libSystem preference to Darwin `pthread_sigmask`, `sigaction`, and `sigaltstack`. Tilt-only Docker/QEMU result: `0/1`; the binary reached Go signal/altstack setup and created a helper thread, then exited with `SIGSTKFLT` status 144. This is progress past the original Darwin TSD/TLS mirror failure, but not an accepted pass. No regression bucket was run because neither `tilt` nor `nu` passed. Logs: `tests/external/logs/loop-g-tilt-attempt3-flat-signals/`.

Loop H tilt signal/thread attempt:

- [x] `loop-h-tilt-pthread-kill-attempt1`: classified the remaining `tilt` stack-fault as flat `_pthread_kill` binding to glibc, which delivered Darwin signal `16` as Linux `SIGSTKFLT` to the main thread after Go helper-thread startup. Added `pthread_kill` to the resolver's mapped-libSystem flat-runtime preference so the existing shim translates/ignores Darwin signal 16. Targeted `tilt version` result: `1/1` PASS, stdout `v0.37.4, built 2026-06-16`, resolver `231 resolved, 0 stubbed, 0 failed`. Guard run `loop-h-tilt-pthread-kill-guards-rerun` kept `yq`, `fzf`, `gum`, `shfmt`, `lazygit`, and `nomad` passing at `6/6`. A first guard command `loop-h-tilt-pthread-kill-guards` did not reach probes because the shared dirty build tree failed during a concurrent `machismo` relink; the rerun used the already rebuilt binary from the passing tilt attempt. Full corpus has not yet been rerun.

Loop I nu TLV attempt:

- [x] `loop-i-nu-tlv-x1-attempt1`: implemented Grok's winning diagnosis that `_tlv_bootstrap` clobbered `x1` across the Mach-O TLV thunk. Rust `nu` kept the `LocalKey` initialization pointer in `x1`; the C implementation call clobbered it, and `get_or_init_slow` later faulted at the old Rust TLS crash site. The assembly wrapper now preserves `x1` with `x30` while still preserving `x8` through `x11`. Targeted `nu --version` passed `1/1`.
- [x] `loop-i-nu-tlv-x1-guards`: targeted guard bucket kept `tilt`, `bun`, and `protoc` passing at `3/3`.
- [x] `loop-i-nu-tlv-x1-just-guard`: canonical-manifest `just` guard passed `1/1`.
- [x] `loop-i-nu-tlv-x1-post-delta-backout`: targeted `nu --version` still passed after backing out failed Delta-specific code.
- [ ] Full corpus has not yet been rerun, so `nu` is a targeted pass pending promotion.

Loop G live attempt log:

- [x] `delta` attempt G1 `loop-g-delta-current-attempt1-{1..5}`: 0 / 5 passing with the dirty Loop F tree. All failures show the helper thread scanning `/proc/<pid>/exe`, `munmap(..., 4096) = EINTR`, child `SIGSEGV`, then parent `tgkill(SIGSEGV)`. Eager `fake_home` initialization alone is not an accepted fix.
- [x] `delta` diagnostic G2 `loop-g-delta-signals-attempt2`: still failed. `MACHGATE_TRACE_SIGNALS=1` now correctly gates the dispatcher logging, but no useful guest PC was captured; the visible crash is the parent intentionally re-raising SIGSEGV after the helper dies.
- [x] `delta` attempt G3 `loop-g-delta-stack-revert-attempt3-{1..5}`: 1 / 5 passing. This improves over 0 / 5 but is still not stable enough to accept. Keep the result as evidence that the stack-identity change worsened timing, but do not claim `delta` is fixed.
- [x] `delta` Loop H classification: reviewed Loop G delta logs plus `full-loop-2026-06-23-N-loop-g-current/delta.*`. Passing qemu traces diverge when helper thread `munmap(tiny 384-byte scratch mapping, 4096)` returns `EINVAL`, then the helper disables its altstack, unmaps the altstack, exits thread 0, the main thread prints `delta 0.19.2`, disables its altstack, unmaps, and `exit_group(0)`. Failing traces reach the same `/proc/<pid>/exe` scan but the helper's one-page `munmap` is interrupted with `EINTR`; QEMU reports child target `SIGSEGV` and the parent re-raises `SIGSEGV`. This is a helper-thread teardown race around small scratch mapping cleanup before `sigaltstack` disable, not the parent re-raise itself.
- [x] `delta` Loop H rejected attempts: `loop-h-delta-munmap-retry-{1..5}` retried raw Darwin `munmap` on `EINTR` (0 / 5), `loop-h-delta-exitgroup-{1..5}` tested LC_MAIN `_exit` -> `exit_group` (0 / 5), `loop-h-delta-small-munmap-{1..5}` tracked raw sub-page mmap/oversized munmap (1 / 5), and `loop-h-delta-shim-munmap-{1..5}` tested the analogous shim `mmap`/`munmap` interposer (0 / 5). None reached the required 5 / 5, so no production fix was accepted and the code attempts were backed out.
- [x] `delta` Loop I rejected attempt `loop-i-delta-munmap-noeintr-attempt1-{1..5}`: signal-masked host `munmap` plus `EINTR` retry in raw and shim paths. Result: 0 / 5. QEMU still returned `munmap(...,4096) = EINTR` and delivered target `SIGSEGV` before retry could make progress. Code was backed out.
- [x] `delta` Loop I rejected attempt `loop-i-delta-small-mmap-shim-attempt2-{1..5}`: tracked sub-page anonymous `mmap` and tried to return Darwin-shaped `EINVAL` for oversized page `munmap`. Result: 1 / 5. Not accepted; code was backed out.
- [x] `delta` Loop I rejected attempt `loop-i-delta-proc-readlink-attempt3-{1..5}`: tried to hide non-self `/proc/<pid>/exe` through shim `readlink`/`readlinkat` plus resolver preference to shorten `find_calling_process`. Result: 0 / 5. Trace still showed host `readlinkat` and the same `munmap(...,4096) = EINTR` failure. Code was backed out.
- [x] Grok `delta` report: recommends keeping `host_processor_info` and signal diagnostics, rejecting the CFString no-copy alias and procfs raw-syscall hiding, and testing the stack-identity revert because the main-thread stack change correlated with 0/5.
- [x] Grok `tilt`/`nu` TLS report: identifies the core semantic gap as missing Darwin-style per-thread TSD storage at the TPIDR-visible base. Current side-table/mirror writes are not enough for inlined Darwin/Go TLS reads; suggested owner is `src/shim/libsystem_shim.c` TSD init/get/set/create and `pthread_create`.
- [x] Grok `terraform` report: warns against blind kqueue churn until the Go PC is symbolized. Likely gaps include `EVFILT_USER` poll wiring, `EV_CLEAR`, event `data`, and possibly Darwin-shaped `getifaddrs`.
- [x] Grok `packer`/`bun` report: `packer` needs scoped Mach-O re-exec for Darwin `execve` / `__mac_execve`; `bun` still has a bounded tail of ICU imports (`unumsys_*`, `uplrules_*`, `ureldatefmt_*`, `ures_close`) worth one more narrow stub pass before declaring semantic ICU debt.

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
- [x] F1 bun attempt 1 `bun-f1-attempt1-icu-collation`: added the visible ICU constrained-field-position, collation, currency, and `udat_close` exports. Targeted Docker result: `0/1`, still `SIGSEGV` 139 in the single C++ static initializer. Resolver improved to `930 resolved, 0 stubbed, 109 failed`; newly printed gaps are `udat_*`, `udatpg_*`, `udtitvfmt_*`, and `uenum_close`/`uenum_count`/`uenum_next`. Logs: `tests/external/logs/bun-f1-attempt1-icu-collation/`.
- [x] F1 bun attempt 2 `bun-f1-attempt2-icu-datefmt`: added minimal ICU date/date-interval formatting, date-pattern-generator, and basic `uenum_*` helpers. Targeted Docker result: `0/1`, still `SIGSEGV` 139 in the single C++ static initializer. Resolver improved to `951 resolved, 0 stubbed, 88 failed`; newly printed gaps are `ufieldpositer_*`, `ufmtval_*`, `uidna_*`, `uldn_*`, and `ulistfmt_*`. Logs: `tests/external/logs/bun-f1-attempt2-icu-datefmt/`.
- [x] F1 bun attempt 3 `bun-f1-attempt3-icu-format-display`: added minimal ICU field-position iterator, formatted-value, IDNA, locale-display-name, and list-format helpers. Targeted Docker result: `0/1`, still `SIGSEGV` 139 in the single C++ static initializer. Resolver improved to `972 resolved, 0 stubbed, 67 failed`; newly printed gaps are `uloc_*` locale APIs and `unorm2_getNFCInstance`/`unorm2_getNFDInstance`/`unorm2_getNFKCInstance`. Logs: `tests/external/logs/bun-f1-attempt3-icu-format-display/`.
- [x] F1 bun attempt 4 `bun-f1-attempt4-icu-locale-normalize`: added minimal ICU locale string helpers and no-op normalization handles. Targeted Docker result: `0/1`, still `SIGSEGV` 139 in the single C++ static initializer. Resolver improved to `994 resolved, 0 stubbed, 45 failed`; newly printed gaps are `unorm2_isNormalized`, `unum_*`, `unumf_*`, `unumrf_*`, and `unumsys_close`. Logs: `tests/external/logs/bun-f1-attempt4-icu-locale-normalize/`.
- [x] F1 bun attempt 5 `bun-f1-attempt5-icu-numberfmt`: added minimal ICU number-format, formatted-number, number-range, and `unumsys_close` shims. Targeted Docker result: `0/1`, still `SIGSEGV` 139 in the single C++ static initializer. Resolver improved to `1014 resolved, 0 stubbed, 25 failed`; newly printed gaps are `unumsys_*`, `uplrules_*`, `ureldatefmt_*`, and `ures_close`. Stopped F1 after five failed Loop F attempts. Logs: `tests/external/logs/bun-f1-attempt5-icu-numberfmt/`.
- [x] D4 attempt 1 `loop-d-worker4-cmake-attempt1-libcurl-map`: mapped `libcurl.4.dylib` to host `libcurl.so.4` in the external harness. Resolver improved from `4464 resolved, 11 failed` to `4475 resolved, 0 failed`, but targeted `cmake --version` still failed with `SIGSEGV si_addr=0x3d6` after CMAKE_ROOT discovery. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt1-libcurl-map/`.
- [x] D4 diagnostic `loop-d-worker4-cmake-attempt1-signal-trace`: reran attempt 1 with signal tracing. The crash was native-side: `pc` in the dynamic linker path, `lr` in glibc `dlsym`, `x8=-2`, and fault address `0x3d6`. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt1-signal-trace/`.
- [x] D4 attempt 2 `loop-d-worker4-cmake-attempt2-shim-local`: tried loading `libsystem_shim.so` as `RTLD_LOCAL` to avoid native interposition. It did not fix cmake and reduced fixed stack-arg helper visibility, so the resolver change was later backed out. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt2-shim-local/`.
- [x] D4 attempt 3 `loop-d-worker4-cmake-attempt3-framework-dlopen`: added Darwin framework `dlopen` path redirection and Linux-safe `dlopen` flag translation in the shim. Targeted cmake still failed with the same `si_addr=0x3d6`; traces showed no framework redirection before the crash, so this was not the active blocker. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt3-framework-dlopen/`.
- [x] D4 attempt 4 `loop-d-worker4-cmake-attempt4-dlsym-handles`: added an exported shim `dlsym` wrapper that translates Darwin special handles (`RTLD_DEFAULT=-2`, `RTLD_SELF=-3`, `RTLD_MAIN_ONLY=-5`) to Linux-safe handles before calling glibc `dlsym`. This fixed the native dynamic-linker crash: targeted `cmake --version` exited 0. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt4-dlsym-handles/`.
- [x] D4 attempt 5 `loop-d-worker4-cmake-attempt5-clean-pass`: backed out the unsuccessful resolver `RTLD_LOCAL` experiment, rebuilt `machismo` and `system_shim`, and reran the targeted cmake manifest with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` and signal tracing. Result: `1/1` PASS, exit status 0, resolver `4475 resolved, 0 stubbed, 0 failed`, no SIGSEGV. A direct QEMU_STRACE rerun also exited 0 and showed `exit_group(0)`; stdout is currently empty under this harness. Logs: `tests/external/logs/loop-d-worker4-cmake-attempt5-clean-pass/`.
- [x] Full corpus `full-loop-2026-06-23-K-loop-e-final`: `51/57` passed, `6` failed. Promoted `cmake` and `nvim` to full-corpus PASS and kept `protoc` passing. Remaining failures: `nu`, `tilt`, `terraform`, `packer`, `nomad`, and `bun`.

### Loop 2026-06-23-F

Start time: 2026-06-23 11:40 PDT.

Goal: move from `51/57` passing to `57/57` passing, or leave each still-failing binary with five new narrow, logged Loop F attempts and an exact next blocker.

Global exit conditions:

- [ ] `57/57` external Mach-O CLI probes pass with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`.
- [ ] Every remaining failure has either a targeted PASS or five Loop F failed attempts with commands, log directories, and researched sources.
- [ ] Full-corpus rerun after any loader, syscall, resolver, shim, dylib-map, runtime, or harness behavior change.

Per-attempt rules:

1. Change one subsystem at a time.
2. Run a targeted manifest for the affected binary or bucket.
3. If stdout/stderr is unclear, rerun with `QEMU_STRACE=1`, `MACHGATE_TRACE_SHIM=1`, or targeted signal/context logging.
4. Update this file after each pass/fail.
5. Stop a single binary after five failed Loop F attempts and mark the precise blocker instead of looping forever.

Loop F Codex workers:

| Worker | Agent | Scope | Write ownership | Exit condition | Status |
| --- | --- | --- | --- | --- | --- |
| F1 | Hubble `019ef5c9-5b58-7353-9be6-e31499b1999e` | `bun` ICU surface after `910/0/129` | `src/shim/libsystem_shim.c`, `src/shim/libsystem_shim.ver`, Loop F docs | `bun` passes or five ICU attempts documented | stopped; 5/5 failed |
| F2 | Epicurus `019ef5c9-5c4f-77c1-a74b-f89a83e00f5c` | `tilt` post-`_main` crash with `231/0/0` binds | crash/context instrumentation in loader/runtime files, Loop F docs | `tilt` passes or first guest PC/root blocker is proven | complete; Go Darwin TLS/TSD mirror blocker proven |
| F3 | Turing `019ef5c9-5e50-7733-95ee-c4dcd641eb9e` | `nu` remaining `519/13/1` plus skipped AppKit/CoreGraphics/Foundation policy | resolver/framework classification or minimal stubs, Loop F docs | `nu` passes or remaining failed symbol and framework policy are proven | complete; imports clean, Rust TLS crash remains |
| F4 | Confucius `019ef5c9-60c5-7833-9a62-2456bda8c9c8` | `terraform`/`nomad` Go runtime post-`_main` crashes | Go/runtime tracing and narrow shim semantics, Loop F docs | one Go target passes or five attempts documented | complete; `nomad` passes, `terraform` nil deref remains |
| F5 | Bacon `019ef5c9-635d-7bf3-b933-fd9cebd0b18f` | `packer` child-process/re-exec status 2 | process/env diagnostics or harness support, Loop F docs | `packer` passes or child-process boundary is precisely documented | complete; HARDER-5 execve support required |

F2 tilt attempts:

- [x] F2 attempt 1 `loop-f2-tilt-attempt1-signal-lcmain`: targeted `tilt version` with `MACHGATE_TRACE_SIGNALS=1`, `MACHGATE_TRACE_LCMAIN=1`, `MACHGATE_EXTERNAL_MAP_LIBCXX=1`, and harness QEMU_STRACE. Result: `0/1`, still `SIGSEGV` 139 after `_main`. Exact fault: `pc=0x100095554`, `lr=0x100075b78`, `addr=NULL`, `__TEXT,__text`, fileoff `0x95554`, instruction `ldr x0, [x0]` after `mov x0, xzr`; resolver remained `231 resolved, 0 stubbed, 0 failed`. Disassembly identified the PC as Go's deliberate abort stub reached from the Darwin cgo/TLS startup path. Logs: `tests/external/logs/loop-f2-tilt-attempt1-signal-lcmain/`.
- [x] F2 attempt 2 `loop-f2-tilt-attempt2-highregs-shim`: added high-register signal diagnostics in `src/trampoline.c`, rebuilt `build-arm64`, and reran targeted tilt with `MACHGATE_TRACE_SIGNALS=1`, `MACHGATE_TRACE_LCMAIN=1`, `MACHGATE_TRACE_SHIM=1`, and `MACHGATE_EXTERNAL_MAP_LIBCXX=1`. Result: `0/1`, still `SIGSEGV` 139 at the same guest PC/LR. Full register context includes `x3=0xc476c475c47957`, `x6=argv`, `x7=envp`, `x8=applep`, `x28=NULL`. Root subsystem: old Go Darwin `runtime/cgo` `tlsinit` creates a pthread key, stores magic `0xc476c475c47957`, then scans the `TPIDRRO_EL0` TLS base for that value; MachGate's current TPIDRRO-to-TPIDR rewrite plus shim TSD mirror does not expose the magic where this binary scans. Blocker is Darwin TSD/TLS mirror semantics in `src/shim/libsystem_shim.c`, outside F2 write ownership. Logs: `tests/external/logs/loop-f2-tilt-attempt2-highregs-shim/`.

Loop F external read-only jobs:

| Job | PID | Report | Scope | Status |
| --- | ---: | --- | --- | --- |
| Grok F1 bun ICU | 4171812 | `/tmp/machgate-agent-reports/loop-f/grok-f1-bun-icu.jsonl` | ICU shim strategy from local logs and public ICU source | complete; recommends bounded ICU groups, not broad tables |
| Grok F2 runtime | 4171848 | `/tmp/machgate-agent-reports/loop-f/grok-f2-runtime.jsonl` | `tilt`, `terraform`, `nomad` post-`_main` crash diagnosis | complete; points to signal context, nomad IOKit, terraform kevent |
| Grok F3 nu frameworks | 4171882 | `/tmp/machgate-agent-reports/loop-f/grok-f3-nu-frameworks.jsonl` | `nu` one failed bind and non-GUI framework policy | complete; one failed bind is `_NSPasteboardTypeString` |
| Grok F4 packer child | 4171923 | `/tmp/machgate-agent-reports/loop-f/grok-f4-packer-child.jsonl` | `packer` child/re-exec status 2 diagnosis | complete; Darwin `execve` returns ENOTSUP |
| Claude F1 bun ICU | 4172924 | `/tmp/machgate-agent-reports/loop-f/claude-f1-bun-icu.txt` | ICU shim strategy | blocked; local Claude session limit until 1pm |
| Claude F2 runtime | 4172929 | `/tmp/machgate-agent-reports/loop-f/claude-f2-runtime.txt` | post-`_main` runtime crash diagnosis | blocked; local Claude session limit until 1pm |
| Claude F3 nu frameworks | 4172944 | `/tmp/machgate-agent-reports/loop-f/claude-f3-nu-frameworks.txt` | `nu` framework/import policy | blocked; local Claude session limit until 1pm |
| Claude F4 packer child | 4172949 | `/tmp/machgate-agent-reports/loop-f/claude-f4-packer-child.txt` | child/re-exec diagnosis | blocked; local Claude session limit until 1pm |
| Grok delta pthread | 2074 | `/tmp/machgate-agent-reports/loop-f-delta/grok-delta-pthread.txt` | `delta` pthread/helper-thread race diagnosis | launched; corrected relaunch |
| Grok delta signal | 2075 | `/tmp/machgate-agent-reports/loop-f-delta/grok-delta-signal.txt` | `delta` SIGSEGV/SIGBUS and altstack semantics | launched; corrected relaunch |
| Grok delta procfs | 2076 | `/tmp/machgate-agent-reports/loop-f-delta/grok-delta-procfs.txt` | `delta` Darwin process scan over Linux procfs | launched; corrected relaunch |
| Grok Loop F next | 2077 | `/tmp/machgate-agent-reports/loop-f-delta/grok-loopf-next.txt` | remaining failure priority review | launched; corrected relaunch |
| Claude delta pthread | 2559 | `/tmp/machgate-agent-reports/loop-f-delta/claude-delta-pthread.txt` | `delta` pthread/helper-thread race diagnosis | launched; corrected relaunch |
| Claude delta signal | 2560 | `/tmp/machgate-agent-reports/loop-f-delta/claude-delta-signal.txt` | `delta` SIGSEGV/SIGBUS and altstack semantics | launched; corrected relaunch |
| Claude delta procfs | 2561 | `/tmp/machgate-agent-reports/loop-f-delta/claude-delta-procfs.txt` | `delta` Darwin process scan over Linux procfs | launched; corrected relaunch |
| Claude Loop F next | 2562 | `/tmp/machgate-agent-reports/loop-f-delta/claude-loopf-next.txt` | remaining failure priority review | launched; corrected relaunch |

Loop F live checklist:

- [x] Start from clean pushed commit `c3cd52a`.
- [x] Launch five Codex workers.
- [x] Launch four Grok jobs.
- [x] Launch four Claude jobs.
- [x] Record Claude launch result: all four Claude jobs exited immediately with `You've hit your session limit - resets 1pm (America/Los_Angeles)`.
- [x] Record Grok F1 result: bounded ICU group stubs are the recommended next `bun` step; avoid broad real ICU tables for now.
- [x] Record Grok F2 result: first runtime move is better guest PC/LR signal context; `nomad` likely needs the remaining IOKit symbols, while `terraform` points at kevent/signal semantics.
- [x] Record Grok F3 result: `nu`'s one failed bind is `_NSPasteboardTypeString`; minimal policy is a single shim data export, not mapping AppKit.
- [x] Record Grok F4 result: `packer` status 2 is real child `execve` unsupported (`ENOTSUP`) from the Darwin syscall gate; harness-only changes cannot close it safely.
- [x] Record first worker report.
- [ ] Integrate reviewed patches.
- [x] Run targeted verification after integrated Loop F attempts.
- [ ] Run full 57-binary corpus after integrated behavior changes.

Loop F worker reports:

- [x] F5 `packer` attempt 1 `f5-attempt1-repro`: packer-only Docker/QEMU external harness rerun with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`, existing `build-arm64`, and the standard `--version` manifest reproduced status `2`. Logs: `tests/external/logs/f5-attempt1-repro/`; work: `tests/external/work/f5-attempt1-repro/`. QEMU trace showed the first `/work/build-arm64/machismo` exec was the harness timeout/loader path and successfully loaded `MACHISMO_CONFIG` plus `/work/build-arm64/libsystem_shim.so` with `155 resolved, 0 failed`. The later child exited `253` before any host `execve`, then the parent exited `2`.
- [x] F5 `packer` attempt 2 `f5-attempt2-checkpoint-disable`: reran with `CHECKPOINT_DISABLE=1 PACKER_CHECKPOINT_DISABLE=1`; still status `2`. Logs: `tests/external/logs/f5-attempt2-checkpoint-disable/`; work: `tests/external/work/f5-attempt2-checkpoint-disable/`. Checkpoint/update-check suppression is not sufficient.
- [x] F5 `packer` attempt 3 `f5-attempt3-packer-log`: reran with `PACKER_LOG=1`, `PACKER_LOG_PATH=/work/tests/external/logs/f5-attempt3-packer-log/packer-runtime.log`, and mounted `TMPDIR`. Still status `2`; Packer log and temp `packer-log*` files were empty, confirming the child dies before useful Packer diagnostics. Logs: `tests/external/logs/f5-attempt3-packer-log/`; work: `tests/external/work/f5-attempt3-packer-log/`.
- [x] F5 `packer` attempt 4 `f5-attempt4-isolated-config`: reran with isolated `HOME`, `XDG_CONFIG_HOME`, `TMPDIR`, `PACKER_CONFIG=/dev/null`, and checkpoint disabled. Still status `2`. Logs: `tests/external/logs/f5-attempt4-isolated-config/`; work: `tests/external/work/f5-attempt4-isolated-config/`. Default user config discovery is not the trigger.
- [x] F5 `packer` attempt 5 `f5-attempt5-isolated-plugins`: reran with isolated `HOME`, `XDG_CONFIG_HOME`, `XDG_CACHE_HOME`, `TMPDIR`, `PACKER_CONFIG=/dev/null`, `PACKER_PLUGIN_PATH=/work/tests/external/work/f5-attempt5-isolated-plugins/plugins`, and checkpoint disabled. Still status `2`. Logs: `tests/external/logs/f5-attempt5-isolated-plugins/`; work: `tests/external/work/f5-attempt5-isolated-plugins/`. Plugin path isolation is not sufficient.
- [x] F5 final classification: no safe harness-only fix found. Current packer blocker is Darwin child-process replacement semantics: Packer's Go child path reaches Darwin `execve`/process replacement inside the guest, and MachGate currently returns Darwin `ENOTSUP` for `execve` and `__mac_execve`, so the child exits `253` and the parent reports status `2`. A real fix should be proposed as a code change outside F5 ownership: implement narrowly scoped Darwin `execve` translation for Mach-O guests by re-execing the Linux loader as `machismo <guest> ...`, preserving `MACHISMO_CONFIG`, `LD_LIBRARY_PATH`, cwd, stdio, and argv/env semantics, then run targeted `packer` plus full 57-binary regression.
- [x] F3 `nu` attempt 1 `loop-f-nu-attempt1-failed-bind-log`: added resolver failed-bind logging and reran `nu --version`. Result: `0/1`, still `SIGSEGV` 139 after `_main`. The exact remaining failed bind is `_NSPasteboardTypeString` from skipped `AppKit`; resolver stayed `519 resolved, 13 stubbed, 1 failed`. Logs: `tests/external/logs/loop-f-nu-attempt1-failed-bind-log/`.
- [x] F3 `nu` attempt 2 `loop-f-nu-attempt2-appkit-data-stub`: implemented the narrow non-GUI policy in `src/resolver.c`: keep AppKit skipped, but resolve `_NSPasteboardTypeString` to an inert NULL data cell instead of an executable return-zero stub. Result: `0/1`, still `SIGSEGV` 139 after `_main`, but import surface is clean at `520 resolved, 13 stubbed, 0 failed`. Logs: `tests/external/logs/loop-f-nu-attempt2-appkit-data-stub/`.
- [x] F3 `nu` attempt 3 `loop-f-nu-attempt3-signal-trace`: diagnostic rerun with `MACHGATE_TRACE_SIGNALS=1`. Result: `0/1`, still `SIGSEGV` 139 with `520 resolved, 13 stubbed, 0 failed`. Signal context is the known Rust TLS store fault: `pc=0x1018fe474`, `lr=0x1011fbcec`, fault address `0x...fb50`, instruction `str xzr, [x8]`, `x8` equal to the fault address. Current F3 blocker is no longer AppKit/CoreGraphics/Foundation import policy; next owner should investigate Rust TLS/runtime signal context after `_main`. Logs: `tests/external/logs/loop-f-nu-attempt3-signal-trace/`.
- [x] F4 `terraform`/`nomad` attempt 1 `loop-f4-attempt1-runtime-trace`: created `/tmp/machgate-loop-f4-terraform-nomad.txt` with the pinned `terraform --version` and `nomad --version` rows, rebuilt `machismo system_shim`, then ran Docker arm64 external harness with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`, `MACHGATE_TRACE_SHIM=1`, `MACHGATE_TRACE_SIGNALS=1`, and `MACHGATE_TRACE_LCMAIN=1`. Result: `0/2`; both still `SIGSEGV` 139. Existing loader signal diagnostics were not reached because Go replaces SIGSEGV handlers through the shim. Logs: `tests/external/logs/loop-f4-attempt1-runtime-trace/`; work: `tests/external/work/loop-f4-attempt1-runtime-trace/`.
- [x] F4 `terraform`/`nomad` attempt 2 `loop-f4-attempt2-signal-context`: added trace-only SIGSEGV/SIGBUS/SIGILL/SIGABRT chaining in `libsystem_shim.c` when `MACHGATE_TRACE_SIGNALS` is set, then reran the same two-row Docker command. Result: `0/2`, but first guest context is now exact. `terraform`: `pc=0x1001c2564`, `lr=0x1001c25c0`, `addr=NULL`, `x3=NULL`; raw instruction at the fault is `ldrsb x27, [x3]`. Last shim events are kqueue setup and `kevent(kq=5, nchanges=1, nevents=0)`. `nomad`: `pc=0x10111ca18` in `github.com/shirou/gopsutil/v3/cpu.perCPUTimes`, `lr=0x10111ca04`, `addr=NULL`, `x4=NULL`; raw instruction is `ldrsb x27, [x4]` after the Darwin `host_processor_info` CPU probe path. Logs: `tests/external/logs/loop-f4-attempt2-signal-context/`; work: `tests/external/work/loop-f4-attempt2-signal-context/`.
- [x] F4 `terraform`/`nomad` attempt 3 `loop-f4-attempt3-host-processor-info`: fixed the narrow `host_processor_info` shim bug where it returned success with `out_processor_info=NULL` and count `0`; it now returns a small Darwin-shaped `PROCESSOR_CPU_LOAD_INFO` array allocated with `mmap` for `vm_deallocate`. Rebuilt arm64 `system_shim` and reran the same two-row Docker command. Result: `1/2`; `nomad --version` PASS, stdout `Nomad v2.0.3`, `BuildDate 2026-06-09T17:33:24Z`, `Revision a2a5fee9c42d6481adcb9be865bcbccf8fd4d725`. `terraform` remains `SIGSEGV` 139 with the same first context: `pc=0x1001c2564`, `lr=0x1001c25c0`, `addr=NULL`, `x3=NULL`, fault instruction `ldrsb x27, [x3]`; resolver remains `143 resolved, 0 stubbed, 0 failed`. Logs: `tests/external/logs/loop-f4-attempt3-host-processor-info/`; work: `tests/external/work/loop-f4-attempt3-host-processor-info/`.
- [x] F4 final classification: stop after one Go target pass. `nomad` is promoted by the `host_processor_info` fix despite seven unrelated visible IOKit binds still printing in this local mixed-worker state. `terraform` is not an import-surface failure; current blocker is a stripped Go runtime/application nil dereference after kqueue setup at `pc=0x1001c2564` with no failed binds. Next owner should continue from the exact signal context and inspect the Go pclntab/source correlation for the stripped Terraform text address before changing more shim semantics.
- [x] Combined targeted verification `loop-f-combined-six-attempt1`: reran the six previously failing binaries (`nu`, `tilt`, `terraform`, `packer`, `nomad`, `bun`) after all Loop F worker changes. Result: `1/6`; `nomad` PASS, while `nu`, `tilt`, `terraform`, `packer`, and `bun` still fail. Logs: `tests/external/logs/loop-f-combined-six-attempt1/`; work: `tests/external/work/loop-f-combined-six-attempt1/`.
- [x] Full-corpus attempt `full-loop-2026-06-23-M-loop-f-clean`: valid clean rerun after Loop F integration. Result: `51/57`; `nomad` PASS, but `delta` regressed, so net corpus progress was not acceptable. Failures were `delta`, `nu`, `tilt`, `terraform`, `packer`, and `bun`. Logs: `tests/external/logs/full-loop-2026-06-23-M-loop-f-clean/`; work: `tests/external/work/full-loop-2026-06-23-M-loop-f-clean/`.
- [x] Delta regression attempt `loop-f-delta-trace-attempt2`: reran `delta --version` with `MACHGATE_TRACE_SIGNALS=1` and `MACHGATE_TRACE_LCMAIN=1`. Result: `0/1`; imports were clean at `279 resolved, 0 stubbed, 0 failed`, but the binary still crashed after `_main` without guest signal context. QEMU trace showed Rust startup threads, `/proc` scanning, `HOME` rewrite, and then `SIGSEGV`. Logs: `tests/external/logs/loop-f-delta-trace-attempt2/`.
- [x] Delta regression attempt `loop-f-delta-shimtrace-attempt3`: reran `delta --version` with `MACHGATE_TRACE_SHIM=1` as well. Result: `0/1`; the active path reached `pthread_create`, `sigaltstack`, `sigaction`, `readlinkat(/proc/...)`, `HOME rewritten to /work/build-arm64/userdata`, and `openat(.../.cache/bat/themes.bin) = ENOENT` before `SIGSEGV`. The only new resolved import consumed by `delta` was the Loop F `CFStringCreateWithCStringNoCopy` alias. Logs: `tests/external/logs/loop-f-delta-shimtrace-attempt3/`.
- [x] Delta regression attempt `loop-f-delta-remove-cfstring-attempt4`: rejected and removed the unsafe `CFStringCreateWithCStringNoCopy` shim/export. One targeted run passed (`1/1`, stdout `delta 0.19.2`), but later repeats proved this was not sufficient to stabilize `delta`. Logs: `tests/external/logs/loop-f-delta-remove-cfstring-attempt4/`; work: `tests/external/work/loop-f-delta-remove-cfstring-attempt4/`.
- [x] Delta repeat loop `loop-f-delta-plain-repeat-1..5`: plain targeted repeats after removing the CFString alias. Result: `1/5` PASS and `4/5` `SIGSEGV` failures. QEMU diagnostic reruns show the `find_calling_process` helper thread scanning `/proc/*/exe` while main rewrites `HOME`; success and failure diverge around helper-thread teardown (`munmap(..., 4096)` returns either `EINVAL` then clean exit, or is interrupted before process status 11). Current blocker is a real thread/signal/procfs runtime compatibility issue, not import resolution.
- [x] Delta external-agent split: launched four read-only Grok jobs and four read-only Claude jobs under `/tmp/machgate-agent-reports/loop-f-delta/` for focused pthread, signal, procfs, and remaining-failure diagnosis. Local Claude may still be session-limited until 1pm PDT.

### Loop 2026-06-23-G

Scope: delta stability and pthread/HOME/procfs-adjacent behavior only.

- [x] Reviewed `/tmp/machgate-agent-reports/loop-f-delta/`. The Claude delta reports were empty. Grok pthread/procfs/signal reports agreed that `delta` is not an import-resolution failure: failures happen after clean binds while the `find_calling_process` helper scans `/proc/*/exe` and tears down its guard/altstack mappings. Grok recommended eager HOME initialization and tightening pthread stack queries before broader procfs or signal changes.
- [x] Delta attempt `loop-g-delta-stability-1..10`: changed `src/shim/libsystem_shim.c` so fake HOME is initialized once during pthread-shim construction, before guest helper threads start, and so only the real main pthread querying itself receives MachGate's synthetic Mach-O main stack from `pthread_get_stackaddr_np` / `pthread_get_stacksize_np`; helper threads use native host stack metadata. Docker/QEMU command rebuilt `machismo system_shim` once, then ran ten one-row `delta --version` repeats with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`. Result: `1/10` harness PASS, `9/10` `SIGSEGV` 139. HOME moved out of the late teardown window and helper stack queries became host-stack queries, but the helper still dies around `munmap(..., 4096)` before `sigaltstack` disable in most runs.
- [x] Delta diagnostic `loop-g-delta-signal-trace`: reran with `MACHGATE_TRACE_SIGNALS=1 MACHGATE_TRACE_SHIM=1`. The trace confirmed eager HOME initialization and native helper stack metadata. No guest signal dispatcher context was emitted before QEMU reported the child `SIGSEGV`; the qemu trace still ended with helper thread proc scan followed by `munmap(..., 4096) = -1 errno=4` and parent re-raising SIGSEGV.
- [x] Delta rejected attempt `loop-g-delta-proc-self-rebuilt-1..10`: briefly tested making `proc_pidpath` fail closed for non-self PIDs to match the raw `/proc/<pid>/exe` hiding policy. Forced a `system_shim` rebuild by removing the shim object/library first. Result: `1/10` PASS and `9/10` `SIGSEGV` 139, so this was not a stability fix and the code change was backed out.
- [ ] Delta remains a blocker. Current best next target is the helper-thread teardown path around Darwin/Rust altstack guard unmapping: success reaches `munmap(guard, 4096) -> EINVAL`, disables `sigaltstack`, unmaps the altstack, and exits 0; failure is interrupted by child `SIGSEGV` before that sequence completes. Avoid sleeps; next work should instrument or translate the raw Darwin `munmap`/`sigaltstack` interaction around PROT_NONE altstack guard pages.

### Loop 2026-06-23-G

- [x] Terraform classification: reviewed `full-loop-2026-06-23-K-loop-e-final`, `full-loop-2026-06-23-M-loop-f-clean`, Loop F F4 logs, and `/tmp/machgate-agent-reports/loop-f-delta/grok-loopf-next.txt`. The repeated sequence was `kqueue() -> fd 5`, two registration-only `kevent(kq=5, nchanges=1, nevents=0)` calls returning success, then `SIGSEGV` at `pc=0x1001c2564`, `lr=0x1001c25c0`, `x3=NULL`, instruction `ldrsb x27, [x3]`. `go tool addr2line` mapped the PC to `net.cgoLookupHostIP` at `net/cgo_unix.go:216`, meaning Go had accepted a successful `getaddrinfo` result and then dereferenced a NULL `ai_addr` under Darwin `struct addrinfo` expectations.
- [x] Terraform fix `loop-g-terraform-getaddrinfo-attempt1`: added Darwin-shaped `getaddrinfo`, `freeaddrinfo`, and `gai_strerror` exports in `src/shim/libsystem_shim.c`/`.ver`. The wrapper calls host `getaddrinfo`, translates supported IPv4/IPv6 results into Darwin `addrinfo` field order and Darwin sockaddr bytes, and returns Darwin `EAI_*` values. Targeted Docker/QEMU result: `1/1` PASS; stdout `Terraform v1.15.6` / `on darwin_arm64`. Logs: `tests/external/logs/loop-g-terraform-getaddrinfo-attempt1/`.
- [x] Loop G guards `loop-g-getaddrinfo-guards`: reran `lazygit --version` and `nomad --version` after the same rebuild. Result: `2/2` PASS. Logs: `tests/external/logs/loop-g-getaddrinfo-guards/`.
- [x] Loop G current six-target verification `loop-g-current-six-targeted`: reran `delta`, `nu`, `tilt`, `terraform`, `nomad`, and `bun` after Loop G shared edits with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`. Result: `2/6`; `terraform` and `nomad` PASS, `delta` remains `SIGSEGV`, `nu` remains `SIGSEGV`, `tilt` now exits with stack fault status `144`, and `bun` remains `SIGSEGV` with clean ICU imports. Logs: `tests/external/logs/loop-g-current-six-targeted/`.
- [x] Packer current probe `loop-g-packer-current`: `packer --version` still fails with status `2`, but it has advanced. The child now successfully calls host `execve("/work/build-arm64/machismo", [machismo, packer, --version], ...)` and loads `/work/build-arm64/libsystem_shim.so`; the child exits with status `13`, then the parent exits `2`. Current packer blocker is no longer missing child loader/shim environment; it is the re-executed child process failing during its own guest startup/runtime. Logs: `tests/external/logs/loop-g-packer-current/`.
- [x] Full corpus `full-loop-2026-06-23-N-loop-g-current`: 51 / 57 with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`. `terraform` is promoted to PASS in the full corpus and `nomad` remains PASS. Failures: `delta` (`SIGSEGV`), `nu` (`SIGSEGV`), `tilt` (stack fault status `144`), `packer` (status `2` after child status `13`), `node` (timeout at 30s while resolving/logging imports), and `bun` (`SIGSEGV`, clean ICU imports). Logs: `tests/external/logs/full-loop-2026-06-23-N-loop-g-current/`.
- [x] `node` timeout classified by targeted rerun `loop-g-node-long120`: `node --version` passes with 120s timeout. Treat the full-corpus `node` timeout as a harness timeout artifact, not a functional regression. Logs: `tests/external/logs/loop-g-node-long120/`.
- [x] ARM64 unit suite: initial Docker run without `BUILD_DIR` was invalid because it used stale `/work/build`; valid rerun with `BUILD_DIR=/work/build-arm64` passed `28/28`. Command: `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'cmake --build build-arm64 --target machismo system_shim wrapgen --parallel && BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh'`.

### Loop 2026-06-23-G Packer Child Re-exec

- [x] Packer attempt 1 `loop-g-packer-reexec-attempt1`: added a narrow Mach-O re-exec helper for raw Darwin `execve` and `__mac_execve`. It accepts only Mach-O/fat Mach-O targets, re-execs `/proc/self/exe` as `machismo <guest> ...`, and preserves inherited `MACHISMO_CONFIG` / `LD_LIBRARY_PATH`. Added fixture `darwin_execve_exit42`; targeted ARM64 Docker/QEMU regression passed. Packer still failed `0/1`, status `2`; logs: `tests/external/logs/loop-g-packer-reexec-attempt1/`.
- [x] Packer attempt 2 `loop-g-packer-reexec-attempt2-trace`: reran with `MACHGATE_TRACE_SYSCALL=1`. Result: `0/1`, status `2`; no raw Darwin `execve`, `__mac_execve`, or `posix_spawn` trace fired before the child exited `253`, which showed Packer's Go path is not using the raw syscall gateway for this imported `_execve` edge.
- [x] Packer attempt 3 `loop-g-packer-reexec-attempt3-shim-execve`: exported an `execve` wrapper from `libsystem_shim.so` using the same Mach-O re-exec helper and reran Packer. Result: `0/1`, status `2`; the previous child status `253` moved to child `SIGPIPE` (`si_status=13`) while the parent still exited `2`, and qemu-strace still showed no successful host `execve` of the child Mach-O. Logs: `tests/external/logs/loop-g-packer-reexec-attempt3-shim-execve/`.
- [x] Packer Loop H rejected attempt `loop-h-packer-sigpipe-guard-attempt2`: tested a loader-only `SIGPIPE` guard to keep MachGate startup diagnostics from killing the re-execed child on closed output pipes. Result: `0/1`, status `2`; qemu-strace still showed the Go fork child status `13` and parent `exit_group(2)`. The guard was backed out. Logs: `tests/external/logs/loop-h-packer-sigpipe-guard-attempt2/`.
- [x] Packer Loop H final probe `loop-h-packer-current-final`: rebuilt after backing out the failed guard and reran the one-row Packer manifest. Result: `0/1`, status `2`. QEMU trace shows Go `forkExec` fd setup (`pipe2`, `dup3`, `fcntl`, signal resets), then child `SIGPIPE` (`si_status=13`) and parent `exit_group(2)`. Logs: `tests/external/logs/loop-h-packer-current-final/`.
- [x] Packer Loop H attempt `loop-h-packer-forksafe-attempt1-real`: added a fork-child-safe re-exec entry point so the shim wrapper avoids tracing, `getenv`, `fprintf`, and `snprintf` after `fork`. Also relaxed `argv[0] == guest_path` rejection so Go's argv shape can be preserved. `tests/test_darwin_execve.sh` passed, but targeted Packer stayed `0/1`, status `2`; child still died with `SIGPIPE`.
- [x] Packer Loop H attempt `loop-h-packer-forksafe-attempt2-shared`: moved fork-child detection into the shared re-exec helper so both raw Darwin `execve` and shim `execve` use the same safer path. Targeted Packer stayed `0/1`, status `2`; qemu-strace still showed child `SIGPIPE` after Go fd setup and before visible helper syscalls.
- [x] Packer Loop H attempt `loop-h-packer-mac-execve-attempt3`: made Darwin `__mac_execve` ignore its MAC label argument and route through the Mach-O re-exec helper instead of returning unsupported. `tests/test_darwin_range_200_399.sh` and `tests/test_darwin_execve.sh` passed, but targeted Packer stayed `0/1`, status `2`; no new progress beyond the same child `SIGPIPE`.
- [x] Packer Loop I diagnostic `loop-i-packer-write-sigpipe-attempt1`: shim `write`/`writev` SIGPIPE guard moved Packer through config/plugin discovery but still showed fork child `SIGPIPE`. Grok's Packer report identified the remaining gap as raw Darwin `write_nocancel` / `write` paths bypassing the shim guard.
- [x] Packer Loop I attempt `loop-i-packer-forkchild-sigpipe-ignore-attempt2`: kept host `SIGPIPE` ignored when the fork child resets Darwin `SIGPIPE` to default. `tests/test_darwin_execve.sh` passed. Targeted Packer no longer died with child `SIGPIPE`; the run timed out/teardown-failed later.
- [x] Packer Loop I diagnostic `loop-i-packer-long180-attempt3`: longer timeout still failed status `2`; trace showed the old child `SIGPIPE` class was gone and the process reached repeated `ppoll`/futex timing before the diagnostic wrapper wedged.
- [x] Packer Loop I attempt `loop-i-packer-raw-write-guard-attempt4`: added shared fork-child SIGPIPE protection for shim `write`/`writev` and raw Darwin `write`, `writev`, `write_nocancel`, `writev_nocancel`, guarded write, and connectx writev. Build passed; `tests/test_darwin_execve.sh`, `tests/test_darwin_write_stdout.sh`, and touched syscall range tests `000_099`, `100_199`, `200_399`, and `400_plus` passed. Targeted Packer remains `0/1`, status `2`, but qemu trace has no `SIGPIPE`, `EPIPE`, or child `si_status=13`; current blocker is later Go process/wait/runtime timing after pipe/fork/wait activity.
- [ ] Packer remains blocked after moving past SIGPIPE. The safe minimal re-exec primitive works for direct raw Darwin `execve`, the imported shim `execve` boundary is reached, and raw write paths no longer kill the fork child. The next Packer loop should inspect the later process/wait/futex state rather than repeating SIGPIPE fixes.

Loop I report-only helper findings:

- [x] Grok Packer report `/tmp/machgate-agent-reports/loop-i/grok-packer-bestof9-rerun.txt`: winning diagnosis was raw `write_nocancel` bypassing the shim guard; implemented in `loop-i-packer-raw-write-guard-attempt4`.
- [ ] Grok Packer post-SIGPIPE report `/tmp/machgate-agent-reports/loop-i/grok-packer-post-sigpipe-bestof9.txt`: running as PID `85772`, read-only, focused on the current status-2/futex/process-wait blocker after SIGPIPE was eliminated.
- [x] Grok Nu report `/tmp/machgate-agent-reports/loop-i/grok-nu-bestof6-rerun.txt`: high-confidence root cause is `_tlv_bootstrap` clobbering `x1` across the Mach-O TLV thunk. Rust keeps the `LocalKey` init pointer in `x1`; the current wrapper only preserves `x8` through `x11`.
- [x] Grok Delta report `/tmp/machgate-agent-reports/loop-i/grok-delta-bestof6-rerun.txt`: high-confidence discriminator remains helper tiny scratch `munmap(..., 4096)` returning `EINVAL` in passes versus `EINTR` in failures. Recommended next narrow attempt is deterministic Darwin `munmap` handling for tracked tiny mappings and no guest-visible `EINTR`, with shim `munmap` export if the imported path is used.

### Loop 2026-06-24-L Launcher Rename and Delta Retest

- [x] Renamed the CMake loader executable target from `machismo` to `machgate`, updated install output, README usage, test harness invocations, and the external Mach-O manifest comment. Valid ARM64 Docker/QEMU test run passed `28/28` with `BUILD_DIR=/work/build-arm64` and `build-arm64/machgate`.
- [x] Grok Delta tournament `/tmp/machgate-agent-reports/loop-l/grok-delta-task.txt` completed after correcting stale `machismo` binary usage. Best retained candidate is shared `guest_vm_track`/dispatch wiring for mmap/munmap, but Delta remains below the acceptance gate: `loop-l-machgate-final` was `1/5`, not `5/5`.
- [ ] Delta remains open. Current evidence says the failing helper `mmap(NULL,384)` / `munmap(addr,4096)` path bypasses both the Darwin syscall gate and shim wrapper and reaches direct host glibc/QEMU `munmap`; pass correlates with `EINVAL`, failure correlates with `EINTR` followed by `SIGSEGV`. Next loop should hook or explain that lower path before trying more shim-only fixes.
- [ ] Packer remains open. Latest accepted progress is still Loop I: raw/shim write SIGPIPE protection removed the child `SIGPIPE` class, but targeted Packer stays status `2` after pipe/fork/wait activity. Next loop should inspect Go process/wait/futex timing instead of repeating SIGPIPE or basic re-exec fixes.
