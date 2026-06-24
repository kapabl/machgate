# Rust Expansion Live Status

Purpose: expand the external Mach-O corpus with additional Rust CLI binaries and
check whether the `delta` helper-thread/VM-teardown failure is common.

Manifest:

- `tests/external/rust_expansion_manifest.txt`
- Temporary stability subset: `tests/external/rust_expansion_pass_only_manifest.txt`

Latest run:

- Run id: `rust-expansion-stability-2026-06-24-A`
- Command shape: `MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/rust_expansion_pass_only_manifest.txt MACHGATE_EXTERNAL_TIMEOUT=30 MACHGATE_EXTERNAL_MAP_LIBCXX=1 bash tests/test_external_macho_cli.sh`
- Snapshot retry command shape: `MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/tests/external/work/rust-expansion-stability-2026-06-24-A-repeat-4-snapshot/build-snapshot MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/rust_expansion_pass_only_manifest.txt MACHGATE_EXTERNAL_TIMEOUT=30 MACHGATE_EXTERNAL_MAP_LIBCXX=1 bash tests/test_external_macho_cli.sh`
- Logs: `tests/external/logs/rust-expansion-stability-2026-06-24-A-repeat-{1,2,3}/`, `tests/external/logs/rust-expansion-stability-2026-06-24-A-repeat-4-snapshot/`
- Work: `tests/external/work/rust-expansion-stability-2026-06-24-A-repeat-{1,2,3}/`, `tests/external/work/rust-expansion-stability-2026-06-24-A-repeat-4-snapshot/`
- Result: clean repeats `30 / 30` valid guest probes passing across repeats 1, 2, and 4-snapshot. Repeat 3 reported `7 / 10`, but the three failures were host runner exec errors while another worker modified `build-arm64/machgate`.
- Current classified result after the Loop B fixes: `13 / 13` Rust expansion rows pass. `jj`, `mise`, and `yazi` each have targeted `5 / 5` repeat coverage after their fixes.
- Final Loop B validation: `tests/external/logs/rust-expansion-spawn-shim-syscall-attempt2-repeat-{1..5}/` passed `mise` and `yazi` in every repeat; verifier snapshot `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-mise-yazi-repeat-{1..5}/` also passed both rows in every repeat with version output.
- Full-manifest validation: `tests/external/logs/rust-expansion-spawn-shim-syscall-attempt2-full/` and verifier snapshot `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-full-manifest/` both passed the full Rust expansion manifest `13 / 13`.
- Regression validation: `BUILD_DIR=/work/build-arm64-docker bash tests/run_tests.sh` passed `28 / 28`; verifier rerun `BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh` also passed `28 / 28`.

## Manifest Validation

- Duplicate check: no `name` or `url` duplicates between
  `tests/external/rust_expansion_manifest.txt`,
  `tests/external/arm64_macho_cli_manifest.txt`, and
  `tests/external/worker_manifests/*.txt`.
- Hash check: all 13 cached artifacts in `tests/external/cache/rust-expansion/`
  match the manifest SHA-256 values.
- Path check: all 13 manifest binary paths resolve in
  `tests/external/work/rust-expansion-2026-06-24-A/`; each work binary is
  executable.
- Binary check: 12 work binaries are Mach-O arm64 executables; `cargo-nextest`
  is a universal Mach-O with an arm64 slice and passed ARM64 selection/run.
- Direct binary check: `tealdeer` uses `binary_path=.`; the cached artifact and
  work binary hashes match and the work binary is executable.

## Promotion And Classification Policy

- Promote only rows with validated hash/path state, no canonical or worker
  duplicate, and a stable `5 / 5` repeat pass under the same manifest command.
- Classify passing rows as `pass-clean` when they have no failed binds or
  skipped critical dylibs in the version run.
- Classify passing rows as `pass-surface` when they pass `--version` but carry
  optional unresolved framework or dylib binds; promote these only with a note
  that they are coverage rows, not clean dependency baselines.
- Keep failing rows in the expansion manifest until the exact failure class is
  fixed and the row passes a targeted `5 / 5` repeat.
- Recommended clean promotion candidates after stability: `tealdeer`, `gping`,
  `bandwhich`, `difftastic`, `cargo-deny`, `lsd`, and `cargo-nextest`.
- Recommended surface promotion candidates after stability: `xh`, `uv`, and
  `zellij`, because their version commands pass but logs retain unresolved
  optional Security/SystemConfiguration/libbz2 or related bindings.
- `mise` and `yazi` are ready for promotion after this loop's targeted `5 / 5` pass.

## Failure Class Summary

- `pass-clean`: `tealdeer`, `gping`, `bandwhich`, `difftastic`, `cargo-deny`,
  `lsd`, `cargo-nextest`.
- `pass-surface`: `xh`, `uv`, `zellij`.
- `fcntl/runtime startup` (fixed for `mise`): initial abort status `134` from
  Darwin `fcntl(fd, 67)` (`F_DUPFD_CLOEXEC`) reaching Linux as cmd `67` ->
  `EINVAL`, then Rust `strerror_r` ABI panic. Narrow fixes: map cmd `67` to
  Linux `F_DUPFD_CLOEXEC`, export XSI `strerror_r`, track kqueue dup aliases,
  poll `EVFILT_USER` on the kqueue/eventfd backing fd (not `ident=0`/stdin),
  and return `EV_RECEIPT` events synchronously. `mise --version` now passes
  targeted `5 / 5`.
- `kqueue/runtime startup` (fixed for `yazi`): shared abort/timeout class
  above is fixed; `yazi --version` no longer aborts `134` or times out `124`
  in tokio `ppoll`/`kevent`.
- `Darwin posix_spawn ABI` (fixed for `yazi`): after kqueue startup was fixed,
  `yazi` reached terminal/adapter startup and attempted
  `ueberzugpp layer -so chafa`. The Mach-O imported Darwin `posix_spawn*`
  symbols, but without shim exports the resolver fell through to glibc
  `posix_spawn*`, whose opaque `posix_spawn_file_actions_t` and
  `posix_spawnattr_t` layouts are not Darwin-compatible. Narrow fix: provide
  Darwin-opaque spawn action/attr wrappers in `libsystem_shim.so`, record close,
  dup2, and open actions, and implement `posix_spawn`/`posix_spawnp` with
  fork plus `execve`/`execvpe`.
- `thread/runtime SIGSEGV` (fixed): `jj` initially exited with status `139` from
  a Mach-O TLV register-clobber in `parking_lot::once::call_once_slow`; current
  `_tlv_bootstrap` wrapper passes targeted `5 / 5`.

## Exit Criteria

- Each passing binary has at least one clean targeted run.
- Each failing binary has either a targeted fix and a `5 / 5` repeat pass, or five documented failed attempts with the exact current blocker.
- The live table below is updated after each loop.
- No row is promoted into the canonical manifest until it is classified.

## Current Table

| Binary | Status | Current result | Owner | Current evidence / next action |
| --- | --- | --- | --- | --- |
| `xh` | PASS | `3 / 3` clean repeats | Stability agent | No delta-like signal/VM-teardown markers in clean repeat logs. Also passed contaminated repeat 3. |
| `uv` | PASS | `3 / 3` clean repeats | Stability agent | No delta-like guest failure. Repeat 3 original run hit host `machgate` exec `EACCES`; snapshot retry passed. |
| `mise` | PASS | `5 / 5` targeted repeats | Grok mise-B + Agent eventloop | Fixed shared tokio/mio kqueue startup: `EVFILT_USER` polls kqueue/eventfd fd instead of `ident=0` (stdin); `EV_RECEIPT` synchronous returns; kqueue dup alias tracking across `F_DUPFD_CLOEXEC`. Logs: pre-fix `tests/external/logs/rust-expansion-2026-06-24-A-mise-fdupfd-attempt1-run{1..5}/`, `tests/external/logs/rust-expansion-fcntl67-attempt1-1/mise.*`; post-fix `tests/external/logs/rust-expansion-loop-b-evreceipt-docker-repeat-{1..5}/`, verifier `tests/external/logs/rust-expansion-loop-b-mise-grok-final-run{1..5}/`, Agent eventloop `tests/external/logs/rust-expansion-loop-b-eventloop-alias-valid-repeat-{1..5}/`. |
| `yazi` | PASS | `5 / 5` targeted repeats | Grok yazi-B + Agent eventloop + main | Fixed in two stages. Shared kqueue startup fix clears abort `134` and timeout `124`; Darwin-opaque `posix_spawn*` shim wrappers clear the later Rust `cstring_array.rs` panic while spawning `ueberzugpp`. Logs: pre-fix `tests/external/logs/rust-expansion-2026-06-24-A/yazi.*`, `tests/external/logs/yazi-abort-check-{1..5}/`, `tests/external/logs/rust-expansion-loop-b-eventloop-alias-valid-repeat-{1..5}/`; final pass `tests/external/logs/rust-expansion-spawn-shim-syscall-attempt2-repeat-{1..5}/`. |
| `tealdeer` | PASS | `3 / 3` clean repeats | Stability agent | No delta-like guest failure. Repeat 3 original run hit host `machgate` exec `EACCES`; diagnostic qemu rerun and snapshot retry passed. |
| `gping` | PASS | `3 / 3` clean repeats | Stability agent | No delta-like signal/VM-teardown markers in clean repeat logs. Also passed contaminated repeat 3. |
| `bandwhich` | PASS | `3 / 3` clean repeats | Stability agent | No delta-like signal/VM-teardown markers in clean repeat logs; note it may use network/system introspection in non-version modes. Also passed contaminated repeat 3. |
| `difftastic` | PASS | `3 / 3` clean repeats | Stability agent | Good `delta` comparison because both are diff/syntax tools; no delta-like markers. Also passed contaminated repeat 3. |
| `cargo-deny` | PASS | `3 / 3` clean repeats | Stability agent | No delta-like signal/VM-teardown markers in clean repeat logs. Also passed contaminated repeat 3. |
| `zellij` | PASS | `3 / 3` clean repeats | Stability agent | Terminal-heavy tool passes `--version`; no delta-like markers. Also passed contaminated repeat 3. |
| `jj` | PASS | `5 / 5` targeted repeats | Agent jj + Grok jj | Fixed: TLV thunk clobbered caller-live `x2`/`x3` in `parking_lot::once::call_once_slow` (`pc=0x100db0260`, `ldr [x3,#0x20]`). Not delta teardown. Logs: `tests/external/logs/rust-expansion-2026-06-24-A/jj-agent/attempt0.trace.err`, `attempt1.trace.{out,err}`, `repeat-{1,2,3,4,5}.{out,err}`; Grok comparison logs: `tests/external/logs/rust-expansion-jj-grok-attempt1/`. |
| `lsd` | PASS | `3 / 3` clean repeats | Stability agent | No delta-like signal/VM-teardown markers in clean repeat logs. Also passed contaminated repeat 3. |
| `cargo-nextest` | PASS | `3 / 3` clean repeats | Stability agent | Universal Mach-O artifact passed initial ARM64 selection/run and clean repeats. Repeat 3 original run hit host `machgate` exec `ETXTBSY` during concurrent build update; snapshot retry passed. |

## Agents

| Worker | Scope | Status | Output |
| --- | --- | --- | --- |
| Agent mise `019ef7c9-9a5b-79a1-b52a-9439d16ee616` | `mise` abort status `134` | done | Added Darwin `F_DUPFD_CLOEXEC` translation in shim/raw fcntl paths. Rebuilt `machgate`/`system_shim`; targeted `mise --version` repeated `0 / 5`, all status `124`. Logs: `tests/external/logs/rust-expansion-2026-06-24-A-mise-fdupfd-attempt1-run{1..5}/`. |
| Agent yazi `019ef7c9-9b9d-70f1-865e-fb53cb216d1b` | `yazi` abort status `134` | done | Diagnosed `fcntl(67)` abort; verified Darwin `F_DUPFD_CLOEXEC` mapping in shim/raw fcntl. Rebuilt `machgate`/`system_shim`; `yazi --version` `0/5` pass, `5/5` abort-cleared. Logs: `tests/external/logs/rust-expansion-2026-06-24-A/yazi.*`, `tests/external/logs/yazi-abort-check-{1..5}/`. |
| Agent jj `019ef7c9-9e45-7761-ba06-bcf92493b9b1` | `jj` SIGSEGV status `139` | done | TLV register preservation fix in `src/shim/libsystem_shim.c`; targeted `jj --version` repeat passed `5 / 5` with logs in `tests/external/logs/rust-expansion-2026-06-24-A/jj-agent/`. |
| Agent stability `019ef7c9-a1fd-7fc3-ae6f-5f08c508639a` | repeat/pass bucket and compare failure class with `delta` | done | `tests/external/logs/rust-expansion-stability-2026-06-24-A-repeat-{1,2,3}/`, `tests/external/logs/rust-expansion-stability-2026-06-24-A-repeat-4-snapshot/` |
| Agent manifest `019ef7ca-1422-7972-83ff-a2a21a06114c` | validate candidate list, hashes, and promotion policy | done | no duplicate names/URLs; all 13 hashes and paths validated; promotion policy recorded above |
| Grok mise PID `206616` | independent `mise` diagnosis | done | `/tmp/machgate-agent-reports/rust-expansion/grok-mise.txt` |
| Grok yazi | independent `yazi` diagnosis | done | Abort `134` and timeout `124` cleared by shared kqueue shim fix; intermediate `cstring_array.rs` panic was superseded by the Darwin `posix_spawn*` shim fix. Logs above. |
| Grok jj PID `206556` | independent `jj` diagnosis | done | `tests/external/logs/rust-expansion-jj-grok-attempt1/` |

## Notes

- Initial signal: the `delta` issue is not universal among Rust binaries. Ten
  new Rust rows passed on the first run.
- The Rust expansion failing set is now empty. `mise` timeout, `yazi`
  kqueue/spawn ABI, and `jj` TLS clobber were separate classes, not delta
  helper-thread teardown.
- `jj` initial `rust-expansion-2026-06-24-A` failure was Rust TLS, not delta
  helper-thread teardown: qemu trace shows clone3 helper spawn, then main-thread
  SIGSEGV at `si_addr=0x20` in `parking_lot::once::Once::call_once_slow`
  (`ldr x8, [x3, #0x20]` with `x3=(nil)`) immediately after a Mach-O TLV thunk
  call. Signal context matches the `nu` class (`x4` near TLV image, `x6/x7`
  `0x1ff/0x200`). Root cause: `_tlv_bootstrap` clobbered caller-live `x2`/`x3`
  across the C impl; the current shim wrapper already preserves `x1` through
  `x18`. Agent targeted rerun logs under
  `tests/external/logs/rust-expansion-2026-06-24-A/jj-agent/`: attempt 1 exited
  `0`, and repeats `repeat-1` through `repeat-5` all exited `0` with stdout
  `jj 0.42.0-b8f7c455170e3273897aaf94431f8ccfb1afa7ad`.

## Loop 2026-06-24-B: Remaining Timeout Fixes

Scope: fix or classify the remaining Rust expansion failures: `mise` and
`yazi`.

Current baseline:

- Rust expansion pass: `13 / 13`
- Failing rows: none
- Shared blocker (fixed): `EVFILT_USER` kqueue shim polled stdin (`ident=0`)
  instead of the eventfd backing fd; missing `EV_RECEIPT` synchronous returns;
  kqueue dup fds not aliased after `F_DUPFD_CLOEXEC`. Post-fix `mise` and
  `yazi` both pass targeted `5 / 5`; the later `yazi` argv/env panic was fixed
  by Darwin-opaque `posix_spawn*` shim wrappers.

Exit criteria:

- A row is fixed only after targeted `5 / 5` repeats pass.
- A candidate is rejected after it fails a targeted run or causes a regression.
- Stop after five documented attempts per row if no production-safe fix is
  found.
- After either row is fixed, rerun `tests/external/rust_expansion_manifest.txt`
  and update the pass/fail count. Final Loop B rerun passed `13 / 13`.

Workers:

| Worker | Scope | Status | Output |
| --- | --- | --- | --- |
| Agent eventloop `019ef7ec-7fde-7761-8a81-656c582023d8` | shared `eventfd`/`socketpair`/`ppoll`/`kqueue` diagnosis | done | Root cause: `collect_kqueue_pollfds` polled stdin for `EVFILT_USER`; fixed to poll kqueue/eventfd fd plus `emit_kqueue_receipts` and dup alias tracking. Local validation: `tests/external/logs/rust-expansion-loop-b-eventloop-alias-valid-repeat-{1..5}/`, full manifest `tests/external/logs/rust-expansion-loop-b-eventloop-alias-full-manifest/`, ARM64 suite `28/28`. |
| Agent mise-B `019ef7ec-80d4-7bc3-8489-9c65c7d2820e` | `mise` source/log diagnosis and narrow candidates | done | Targeted `5 / 5` in docker; logs `tests/external/logs/rust-expansion-loop-b-mise-grok-final-run{1..5}/`. |
| Agent yazi-B `019ef7ec-82f3-7db2-b317-8d146ece87f9` | `yazi` source/log diagnosis and narrow candidates | done | Shared kqueue fix cleared timeout; smoke `tests/external/logs/rust-expansion-loop-b-yazi-grok-smoke-1/` exposed a separate `cstring_array.rs` panic later fixed by the spawn ABI shim. |
| Agent source-B `019ef7ec-85d7-7773-af54-94801399c87c` | Tokio/mio/yazi/mise source research for macOS event-loop startup | done | Source-backed diagnosis below. |
| Agent verifier-B `019ef7ec-8953-7f13-895b-dbc7db04dce1` | build/test loop and regression guard after candidates | done | Candidate2 verifier snapshot: build `machgate`/`system_shim`; `mise` + `yazi` targeted `5 / 5`; `jj` guard `5 / 5`; pass-only manifest `10 / 10`; full Rust expansion manifest `13 / 13`; ARM64 suite `28 / 28`. Logs under `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-*`. |
| Grok eventloop PID `225373` | independent shared event-loop diagnosis | done | `EVFILT_USER` stdin poll + missing receipts diagnosis confirmed in `src/shim/libsystem_shim.c`. |
| Grok mise PID `225430` | independent `mise` timeout diagnosis | done | `5 / 5` post-fix; logs above. |
| Grok yazi PID `225476` | independent `yazi` timeout diagnosis | done | Timeout class cleared; intermediate `cstring_array.rs` panic in smoke log was superseded by the spawn ABI shim fix. |
| Grok eventloop verifier | docker `5/5` mise + `5/5` yazi timeout-cleared | done | `mise` `5/5` in `tests/external/logs/rust-expansion-loop-b-evreceipt-docker-repeat-{1..5}/`; `yazi` `0/5` pass, `5/5` timeout-cleared in `tests/external/logs/rust-expansion-loop-b-evreceipt-final-{1..5}/` (status `1` argv/env panic). |
| Main spawn ABI fix | Darwin `posix_spawn*` wrappers for yazi's post-kqueue panic | done | Added Darwin-opaque spawn file-action/attr wrappers and fork/exec-backed `posix_spawn`/`posix_spawnp` exports in `src/shim/libsystem_shim.c` and `src/shim/libsystem_shim.ver`. Targeted `mise` + `yazi` repeats passed `5 / 5`; full Rust expansion passed `13 / 13`. Logs: `tests/external/logs/rust-expansion-spawn-shim-syscall-attempt2-repeat-{1..5}/`, `tests/external/logs/rust-expansion-spawn-shim-syscall-attempt2-full/`. |
| Verifier candidate2 snapshot | Independent verifier rerun after spawn ABI fix | done | Logs: `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-mise-yazi-repeat-{1..5}/`, `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-jj-guard-repeat-{1..5}/`, `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-pass-only-guard/`, `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-full-manifest/`. |
- Agent eventloop candidate `rust-expansion-loop-b-eventloop-alias`: implemented
  kqueue fd alias tracking for `F_DUPFD`/`F_DUPFD_CLOEXEC` in
  `src/shim/libsystem_shim.c` on top of the `EV_RECEIPT`/`EVFILT_USER` shim
  behavior. Build command:
  `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build-arm64 --target machgate system_shim --parallel'`.
  Valid targeted repeats:
  `tests/external/logs/rust-expansion-loop-b-eventloop-alias-valid-repeat-{1..5}/`;
  result `mise` `5 / 5`, `yazi` `0 / 5` pass but `5 / 5` timeout-cleared
  with status `1`. An earlier repeat set without `valid` in the path was
  invalid because the host filesystem filled during extraction and produced
  extraction/bus-error artifacts. Full manifest rerun
  `tests/external/logs/rust-expansion-loop-b-eventloop-alias-full-manifest/`
  passed `12 / 13`; `BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh`
  passed `28 / 28`.
- Early main-thread log read: `mise` and `yazi` initially shared an `eventfd2`
  + `fcntl(fd, 67, NULL) -> EINVAL` setup failure, followed by Rust
  `strerror_r failure` while formatting the panic. Darwin command `67` is
  `F_DUPFD_CLOEXEC`; after adding that mapping, both rows moved to timeout in
  `ppoll` on event-loop fds.
- Agent mise result: post-fix targeted runs
  `rust-expansion-2026-06-24-A-mise-fdupfd-attempt1-run{1..5}` all timed out
  with status `124`. The qemu traces consistently show `eventfd2`,
  successful `fcntl(...,F_DUPFD_CLOEXEC)`, `socketpair`, and then a blocking
  `ppoll` interrupted only by the harness `SIGALRM`/`SIGTERM`.
- Agent mise-B source diagnosis: the manifest `mise --version` path is not a
  manifest/env shortcut failure. `mise v2026.6.13` calls
  `tokio::runtime::Builder::new_multi_thread().enable_all().build()?` in
  `src/main.rs` before async CLI dispatch, so even `--version` initializes
  Tokio/mio. Its lockfile uses Tokio `1.52.3` and mio `1.2.1`; mio's macOS
  selector creates a kqueue, clones the selector fd with `F_DUPFD_CLOEXEC`,
  registers `EVFILT_USER` with `EV_RECEIPT`, and may wait through another clone.
  The old shim keyed kqueue registrations by numeric fd, which loses Darwin
  kqueue state across duplicated descriptors. The current shared shim contains
  the narrow candidate surface: kqueue fd alias tracking plus `EV_RECEIPT`
  success events. Local compile validation passed in `/tmp/machgate-loop-b-build`,
  but requested `mise 5/5` and yazi smoke could not run locally because the
  x86_64 workspace lacks `qemu-aarch64` and `/lib/ld-linux-aarch64.so.1`; the
  existing `build-arm64` cache is also pinned to `/work/build-arm64`.
- Grok mise result: independent diagnosis confirmed cmd `67` =
  `F_DUPFD_CLOEXEC` (not `F_GETSIGSINFO`); added XSI `strerror_r` shim alias.
  Grok repeats `rust-expansion-mise-grok-attempt{1..5}`: `0 / 5`, all status
  `124`; report at `/tmp/machgate-agent-reports/rust-expansion/grok-mise.txt`.
- Grok yazi result: initial run
  `tests/external/logs/rust-expansion-2026-06-24-A/yazi.{err,qemu-strace}`
  shows `eventfd2(0,526336)`, then `fcntl(3,67,NULL)->EINVAL`, then Rust panic
  `Failed building the Runtime: Os { code: 22 }` and SIGABRT status `134`.
  Darwin `67` is `F_DUPFD_CLOEXEC`. After rebuilding with the narrow shim/syscall
  mapping (`cmd=67->1030`), five abort-check runs
  (`tests/external/logs/yazi-abort-check-{1..5}/`) no longer panic or abort
  (`5/5` abort-cleared) but still exit `124` with empty stdout. Post-fix traces
  (`yazi-shimtrace/err`, `yazi-180/err`) show successful
  `fcntl(fd=3 cmd=67->1030)`, `kevent(...) -> 1`, `socketpair`, then blocking
  `ppoll`/`kevent` with no `pthread_create` before timeout. Hang occurs inside
  `#[tokio::main]` runtime construction, before async `main` body/terminal init.
- Agent yazi-B addendum: source check confirms `yazi-fm` reaches
  `#[tokio::main]` before `yazi_boot::init()` can process `--version`, so the
  original block was a real Tokio/mio startup dependency and not a manifest
  shortcut issue. After the Loop B kqueue candidate, `/work` evreceipt logs
  show yazi gets past the socketpair/ppoll timeout and into terminal/adapter
  setup, where it falls back to the `Chafa` adapter, attempts
  `ueberzugpp layer -so chafa`, and exits through the new Rust
  `std/src/sys/process/unix/common/cstring_array.rs` panic:
  `range end index 18446744073709551615 out of range for slice of length 0`.
  This was a separate terminal/process-spawn class and was later fixed by the
  Darwin `posix_spawn*` shim wrappers; verifier logs now show
  `Yazi 26.5.6 ...` in all five targeted repeats. Local yazi `5/5` could
  not be run from `/home`: `uname -m` is `x86_64`; `build/` is x86_64 and
  invalid for ARM64 guest execution; `build-arm64` is pinned to missing `/work`;
  and a throwaway aarch64 configure failed because the sysroot lacks `crt1.o`,
  `crti.o`, `-lc`, and `crtn.o`.
- Agent source-B result: official/source repo check pins the shared startup
  surface to Tokio/mio, not yazi/mise application version logic.
  - Upstream pins: yazi `v26.5.6` commit
    `aa526434f00bb44e2e902d9a4ac5f810da1018b9`
    ([tag](https://github.com/sxyazi/yazi/tree/v26.5.6)) locks Tokio
    `1.52.2` and mio `1.2.0`
    ([Cargo.toml](https://github.com/sxyazi/yazi/blob/v26.5.6/Cargo.toml#L74),
    [Cargo.lock](https://github.com/sxyazi/yazi/blob/v26.5.6/Cargo.lock#L2438-L2450),
    [Cargo.lock](https://github.com/sxyazi/yazi/blob/v26.5.6/Cargo.lock#L4698-L4714)).
    mise `v2026.6.13` tag object
    `7ce511e4ebb7f5d9819f44350dde97e75c8b56f0` peels to commit
    `05cfac61af165e3b214e1b32920063ecb9bc0db5`
    ([tag](https://github.com/jdx/mise/tree/v2026.6.13)) and locks Tokio
    `1.52.3` plus mio `1.2.1`
    ([Cargo.toml](https://github.com/jdx/mise/blob/v2026.6.13/Cargo.toml#L192),
    [Cargo.lock](https://github.com/jdx/mise/blob/v2026.6.13/Cargo.lock#L5278-L5290),
    [Cargo.lock](https://github.com/jdx/mise/blob/v2026.6.13/Cargo.lock#L9016-L9032)).
    `tokio-rs/mio` has source tag `v1.2.0`
    (`ce39a6be2cc739165daaeb10cce609b9b77242ac`); no public `v1.2.1`
    tag was found by `git ls-remote`, so the exact mise mio source was inferred
    from the lockfile plus the adjacent `v1.2.0` kqueue backend.
  - Tokio startup path: `Builder::enable_all()` enables I/O on Unix when net,
    process, or signal support is present
    ([Tokio 1.52.3](https://github.com/tokio-rs/tokio/blob/tokio-1.52.3/tokio/src/runtime/builder.rs#L349-L377));
    runtime driver construction calls `mio::Poll::new()` and
    `mio::Waker::new(...)`
    ([Tokio 1.52.3](https://github.com/tokio-rs/tokio/blob/tokio-1.52.3/tokio/src/runtime/io/driver.rs#L117-L132));
    the Unix signal driver clones Tokio's global signal receiver and registers
    it with the mio registry
    ([Tokio 1.52.3](https://github.com/tokio-rs/tokio/blob/tokio-1.52.3/tokio/src/runtime/signal/mod.rs#L41-L79),
    [register](https://github.com/tokio-rs/tokio/blob/tokio-1.52.3/tokio/src/runtime/io/driver/signal.rs#L1-L12)).
    Tokio `1.52.2` has the same relevant source path
    ([Tokio 1.52.2](https://github.com/tokio-rs/tokio/blob/tokio-1.52.2/tokio/src/runtime/io/driver.rs#L117-L132)).
  - mio macOS backend: `mio` selects `selector/kqueue.rs` and
    `waker/kqueue.rs` on macOS
    ([mio v1.2.0](https://github.com/tokio-rs/mio/blob/v1.2.0/src/sys/unix/mod.rs#L16-L90)).
    `Selector::new()` calls `kqueue()` and `fcntl(F_SETFD, FD_CLOEXEC)`,
    `select()` waits with `kevent`, and registration uses `EVFILT_READ` /
    `EVFILT_WRITE` with `EV_CLEAR | EV_RECEIPT | EV_ADD`
    ([kqueue.rs](https://github.com/tokio-rs/mio/blob/v1.2.0/src/sys/unix/selector/kqueue.rs#L74-L162)).
    `Waker::new()` duplicates the selector and registers an `EVFILT_USER`
    waker; `wake()` triggers it through `NOTE_TRIGGER`
    ([waker](https://github.com/tokio-rs/mio/blob/v1.2.0/src/sys/unix/waker/kqueue.rs#L1-L25),
    [setup/wake](https://github.com/tokio-rs/mio/blob/v1.2.0/src/sys/unix/selector/kqueue.rs#L215-L265)).
    Tokio's Unix signal globals create a `UnixStream::pair()`
    ([Tokio unix.rs](https://github.com/tokio-rs/tokio/blob/tokio-1.52.3/tokio/src/signal/unix.rs#L65-L73));
    on macOS mio implements that as `socketpair(AF_UNIX, SOCK_STREAM)` followed
    by `fcntl(F_SETFL, O_NONBLOCK)` and `fcntl(F_SETFD, FD_CLOEXEC)` on both
    fds
    ([mio UDS pair](https://github.com/tokio-rs/mio/blob/v1.2.0/src/sys/unix/uds/mod.rs#L84-L133)).
  - Version paths: yazi's `yazi` binary is `#[tokio::main]`, so runtime
    construction happens before its async body and before `yazi_boot::init()`
    can parse `--version` and call `process::exit(0)`
    ([main](https://github.com/sxyazi/yazi/blob/v26.5.6/yazi-fm/src/main.rs#L9-L30),
    [boot init](https://github.com/sxyazi/yazi/blob/v26.5.6/yazi-boot/src/lib.rs#L11-L15),
    [version action](https://github.com/sxyazi/yazi/blob/v26.5.6/yazi-boot/src/actions/actions.rs#L8-L15)).
    mise also builds a Tokio runtime before version handling:
    `main()` calls `Builder::new_multi_thread().enable_all().build()?.block_on(main_())`,
    and `Cli::run()` calls `ctrlc::init()` before its fast
    `print_version_if_requested()` exit path
    ([main](https://github.com/jdx/mise/blob/v2026.6.13/src/main.rs#L105-L113),
    [Cli::run](https://github.com/jdx/mise/blob/v2026.6.13/src/cli/mod.rs#L630-L677),
    [version fast path](https://github.com/jdx/mise/blob/v2026.6.13/src/cli/version.rs#L87-L99),
    [ctrlc](https://github.com/jdx/mise/blob/v2026.6.13/src/ui/ctrlc.rs#L12-L24)).
  - Expected pre-`--version` Darwin/macOS surface: `kqueue`,
    `fcntl(F_SETFD)`, fd duplication with close-on-exec for cloned kqueue /
    signal receiver fds, `kevent` registration receipts for `EVFILT_USER` and
    `EVFILT_READ`, `socketpair(AF_UNIX, SOCK_STREAM)`, and macOS fallback
    `fcntl(F_SETFL, O_NONBLOCK)` / `fcntl(F_SETFD)` on socketpair fds. For
    mise, `pthread_create` can also be expected from the multi-thread runtime,
    and `sigaction`/signal-hook setup can appear once `ctrlc::init()` is polled.
  - Diagnosis: `socketpair` is expected upstream Tokio signal-driver behavior.
    `ppoll` is not a macOS event-loop primitive; when it appears after
    `socketpair` in these traces, it is MachGate's Linux host wait underneath
    the shim/gateway kqueue emulation. Current MachGate source confirms that
    the shim `kqueue()` returns an `eventfd`, records `EVFILT_READ`,
    `EVFILT_WRITE`, and `EVFILT_USER`, and waits via host `poll()`, while the
    raw syscall gateway `kevent` path still only accepts a zero-change,
    zero-event no-op wait
    ([shim](../src/shim/libsystem_shim.c),
    [raw gateway](../src/syscall/syscall_range_200_399.c)).
    The narrow missing behavior is therefore not another version-path import;
    it is Darwin kqueue readiness/wakeup semantics in MachGate: make
    `EVFILT_USER` wakeups and `EVFILT_READ` socketpair registrations reliably
    produce Darwin-shaped `kevent` events, and keep the raw syscall and shim
    kqueue models consistent for nonzero `nchanges`/`nevents`.
- Stability repeat result: repeats 1 and 2 passed `10 / 10`; repeat 3 reported
  `7 / 10` but its three failures were host-runner exec errors against
  `/work/build-arm64/machgate` (`EACCES` for `uv`/`tealdeer`, `ETXTBSY` for
  `cargo-nextest`) while the shared build artifact was being modified. The
  repeat-4 snapshot rerun passed `10 / 10`, giving each initially passing row
  three clean guest executions.
- No initially passing Rust row currently looks `delta`-like or guest-flaky
  from the clean repeat logs.
