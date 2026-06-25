# External Mach-O Unified Status

This table is the single status board for all visible external ARM64 macOS CLI
binaries currently tracked by MachGate.

Related failure-class note: `docs/HOST_GLIBC_BYPASS_FAILURES.md` documents the
host/glibc bypass class used by the targeted Delta fix.

Packer fix note: `docs/PACKER_SIGCHLD_FIX.md` documents the accepted SIGCHLD
policy and rejected dispatcher attempt.

Last refreshed: 2026-06-24

## Summary

| Category | Count | Meaning |
| --- | ---: | --- |
| Working: external functional full-corpus pass | 57 | Passed in the latest 120s 57-binary corpus run with libc++ mapping enabled. |
| Strict 30s subset | 51 | Also passed in the latest strict 30s 57-binary corpus run with libc++ mapping enabled. |
| Working: Rust expansion pass | 13 | Passed the Rust expansion manifest after Loop B, including targeted repeat gates for fixed rows. |
| Not working: functional blockers | 0 | No current functional blocker remains. |
| Total visible binaries | 70 | 57 original external corpus rows plus 13 Rust expansion rows. |

Functional completion: `70 / 70` working (`100.0%`).

Focused C++ / test-runner addendum:

- `docs/CATCH2_STATIC_INIT_EXTERNAL_LOOP.md` tracks the focused C++ static-init loop.
- Generated local `cpp_many_ctors`: `707 / 707` constructors completed and exited `0`.
- Optional generated local `cpp_public_test_registrars`: `707 / 707` source-derived Catch2/gtest-style registrar constructors completed and exited `0`.
- Public focused C++ rows: `18 / 22` passing after Loop K/L.
- Focused blockers: `bitcoin-29.2-test`, `bitcoin-26.2-test`, `knots-test`, and `qtum-test`.
- All focused blockers are real external C++ Boost.Test runners. `TIMEOUT_AFTER_MAIN` is fixed `8 / 8`, and the original `BOOST_AUTO_START_DBG` parser error is fixed. The remaining focused bucket is `BOOST_NOTHING_TO_TEST_ABORT` (4): `--help` prints Boost help, then aborts with `boost::unit_test::framework::nothing_to_test`.

## Unified Table

| Binary | Corpus | Status | Current evidence / next action |
| --- | --- | --- | --- |
| `ripgrep` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `fd` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `hyperfine` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `bat` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `jq` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `yq` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run; Go TLS/signal group is passing. |
| `starship` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `bottom` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `just` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run; guard row also stayed passing after TLV work. |
| `zoxide` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `sd` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `git-cliff` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `sccache` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `cargo-binstall` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `hexyl` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `pastel` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `atuin` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `ninja` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `procs` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `fzf` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run; Go TLS/signal group is passing. |
| `gh` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `lazygit` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run; guarded by getaddrinfo fixes. |
| `glow` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `gum` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run; Go TLS/signal group is passing. |
| `goreleaser` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `chezmoi` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `duf` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `shfmt` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run; Go TLS/signal group is passing. |
| `fx` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `kubectl` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `k9s` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `kind` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `minikube` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `argocd` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `flux` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `kubeseal` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `stern` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `helm` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `terraform` | external-57 | WORKING-strict | Passed latest strict full corpus after Darwin-shaped `getaddrinfo`, `freeaddrinfo`, and `gai_strerror`. |
| `vault` | external-57 | WORKING-strict | Passed full corpus after sysctl/sysctlbyname/uname-style shim coverage. |
| `consul` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `nomad` | external-57 | WORKING-strict | Passed latest strict full corpus after Darwin CPU-load `host_processor_info` layout fix. |
| `boundary` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `tofu` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `terragrunt` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `pulumi` | external-57 | WORKING-strict | Passed latest strict 30s full-corpus run. |
| `cmake` | external-57 | WORKING-strict | Passed full corpus after libcurl mapping and Darwin `dlsym` special-handle translation. |
| `sqlite3` | external-57 | WORKING-strict | Passed after malloc zone, Darwin-shaped variadic `vfprintf`, and access wrapper fixes. |
| `duckdb` | external-57 | WORKING-strict | Passed full corpus with opt-in libc++ mapping. |
| `protoc` | external-57 | WORKING-strict | Passed after native Mach-O `__eh_frame` is served by `_dl_find_object`. |
| `nvim` | external-57 | WORKING-strict | Passed after `ioctl(FIONBIO)` stack-argument thunk and native Mach-O `__eh_frame` hook-only registration. |
| `node` | external-57 | WORKING-functional120 | Passed latest integrated full corpus `full-loop-2026-06-24-R-functional120`; strict 30s QEMU full-corpus run still times out. Not an active functional blocker. |
| `tilt` | external-57 | WORKING-functional120 | Passed latest integrated full corpus `full-loop-2026-06-24-R-functional120`; original targeted fix was `loop-h-tilt-pthread-kill-attempt1`. |
| `bun` | external-57 | WORKING-functional120 | Passed latest integrated full corpus `full-loop-2026-06-24-R-functional120`; original targeted fix was `bun-loop-h-attempt1-tpidr-rt`. |
| `nu` | external-57 | WORKING-functional120 | Passed latest integrated full corpus `full-loop-2026-06-24-R-functional120`; original targeted fix was `loop-i-nu-tlv-x1-attempt1`. |
| `delta` | external-57 | WORKING-functional120 | Passed latest integrated full corpus `full-loop-2026-06-24-R-functional120` and targeted `5 / 5` in `loop-m-delta-vm-interpose-attempt{1..5}` after adding `libmachgate_vm_interpose.so` and loader re-exec with `LD_PRELOAD`. The fix catches host/glibc `mmap` / `munmap` paths that bypass imported libSystem symbols and routes them through the shared Darwin VM tracker. |
| `packer` | external-57 | WORKING-functional120 | Passed latest integrated full corpus `full-loop-2026-06-24-R-functional120`, targeted `5 / 5` in `loop-q4-packer-sigchld-production-attempt{1..5}`, and reconfirmed `5 / 5` in `loop-q6-packer-post-unitfix-attempt{1..5}`. The accepted fix keeps host Linux `SIGCHLD` at default for guest Darwin `sigaction(SIGCHLD, ...)`, avoiding delivery of Linux signal `17` into Darwin Go code expecting signal `20`; blocking `wait4` still carries child status. |
| `xh` | rust-expansion | WORKING-rust | Passed clean repeats; classified pass-surface due optional unresolved bindings. |
| `uv` | rust-expansion | WORKING-rust | Passed clean repeats; classified pass-surface due optional unresolved bindings. |
| `mise` | rust-expansion | WORKING-rust | Passed targeted `5 / 5` after Darwin `F_DUPFD_CLOEXEC`, kqueue aliasing, `EV_RECEIPT`, and `EVFILT_USER` fixes. |
| `yazi` | rust-expansion | WORKING-rust | Passed targeted `5 / 5` after kqueue startup fixes and Darwin-compatible `posix_spawn*` shim wrappers. |
| `tealdeer` | rust-expansion | WORKING-rust | Passed clean repeats. |
| `gping` | rust-expansion | WORKING-rust | Passed clean repeats. |
| `bandwhich` | rust-expansion | WORKING-rust | Passed clean repeats. |
| `difftastic` | rust-expansion | WORKING-rust | Passed clean repeats; useful comparison row for `delta`. |
| `cargo-deny` | rust-expansion | WORKING-rust | Passed clean repeats. |
| `zellij` | rust-expansion | WORKING-rust | Passed clean repeats; classified pass-surface due optional unresolved bindings. |
| `jj` | rust-expansion | WORKING-rust | Passed targeted `5 / 5` after TLV register preservation fix. |
| `lsd` | rust-expansion | WORKING-rust | Passed clean repeats. |
| `cargo-nextest` | rust-expansion | WORKING-rust | Passed clean repeats; universal Mach-O artifact uses arm64 slice successfully. |

## Action Queue

1. Treat `node` strict 30s startup as timeout tuning unless strict 30s is a
   release gate.
2. Optional: rerun strict 30s full corpus after startup-cost work.

## Rust Expansion Fix Details

The Rust expansion rows are unified into the table above, but their fix history
is intentionally kept here because these failures covered distinct runtime
classes that are likely to recur in future Rust CLI binaries.

### `mise`: Tokio/mio kqueue startup

Initial result:

- `mise --version` did not avoid async startup. `mise v2026.6.13` builds a
  multi-thread Tokio runtime before CLI dispatch.
- Runtime construction reached mio's macOS kqueue backend.
- The first failure was status `134`: Darwin `fcntl(fd, 67)` was passed to
  Linux as command `67`, producing `EINVAL`.

Root cause:

- Darwin command `67` is `F_DUPFD_CLOEXEC`.
- Linux uses `F_DUPFD_CLOEXEC=1030`.
- mio clones the kqueue fd and expects cloned descriptors to share kqueue
  registration state.

Fixes retained:

- Translate Darwin `F_DUPFD_CLOEXEC` in shim and raw syscall `fcntl` paths.
- Track kqueue fd aliases across `F_DUPFD` / `F_DUPFD_CLOEXEC`.
- Return Darwin-shaped `EV_RECEIPT` events synchronously during registration.
- Poll `EVFILT_USER` on the kqueue/eventfd backing fd instead of treating
  `ident=0` as stdin.

Validation:

- Targeted `mise` + `yazi` repeats:
  `tests/external/logs/rust-expansion-spawn-shim-syscall-attempt2-repeat-{1..5}/`
  passed `5 / 5`.
- Verifier snapshot:
  `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-mise-yazi-repeat-{1..5}/`
  also passed `5 / 5`.

### `yazi`: kqueue startup plus Darwin `posix_spawn*` ABI

Initial result:

- `yazi --version` also enters Tokio before version output because `yazi-fm`
  uses `#[tokio::main]` before `yazi_boot::init()` processes `--version`.
- The first failure matched `mise`: Darwin `F_DUPFD_CLOEXEC` reached Linux as
  `fcntl` command `67`, then runtime construction aborted.
- After the kqueue fixes, `yazi` no longer aborted or timed out. It reached
  terminal/adapter startup, attempted `ueberzugpp layer -so chafa`, and then
  panicked in Rust `std/src/sys/process/unix/common/cstring_array.rs`.

Root cause:

- The Mach-O imports Darwin `posix_spawn*` symbols.
- Without shim exports, resolver fallback let those calls reach glibc
  `posix_spawn*`.
- glibc's opaque `posix_spawn_file_actions_t` and `posix_spawnattr_t` layouts
  are not Darwin-compatible, corrupting Rust process-spawn state.

Fixes retained:

- Same kqueue startup fixes listed for `mise`.
- Export Darwin-compatible `posix_spawn`, `posix_spawnp`,
  `posix_spawn_file_actions_*`, and `posix_spawnattr_*` wrappers from
  `libsystem_shim.so`.
- Store Darwin-opaque spawn action/attr objects inside the shim.
- Record close, dup2, and open actions.
- Implement spawn with fork plus MachGate-aware Mach-O reexec first, then raw
  Linux `SYS_execve` / PATH search fallback.

Validation:

- Targeted `mise` + `yazi` repeats:
  `tests/external/logs/rust-expansion-spawn-shim-syscall-attempt2-repeat-{1..5}/`
  passed `5 / 5`.
- Full Rust expansion:
  `tests/external/logs/rust-expansion-spawn-shim-syscall-attempt2-full/`
  passed `13 / 13`.
- Verifier snapshot full manifest:
  `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-full-manifest/`
  passed `13 / 13`.

### `jj`: Mach-O TLV register preservation

Initial result:

- `jj --version` initially failed with status `139`.
- Trace pointed at `parking_lot::once::call_once_slow`, with a NULL pointer
  dereference shortly after a Mach-O TLV thunk call.

Root cause:

- The shim `_tlv_bootstrap` wrapper clobbered caller-live registers that Rust
  code kept live across the Mach-O TLV thunk.

Fix retained:

- Preserve caller-live `x1` through `x18` and `x30` around `_tlv_bootstrap_impl`
  on ARM64.

Validation:

- Targeted `jj --version` passed `5 / 5`.
- Verifier guard:
  `tests/external/logs/rust-expansion-loop-b-verifier-2026-06-24-candidate2-jj-guard-repeat-{1..5}/`
  passed `5 / 5`.

### Rust Expansion Regression Coverage

Final accepted Rust expansion state:

- `tests/external/rust_expansion_manifest.txt`: `13 / 13` passing; latest
  rerun `tests/external/logs/rust-expansion-2026-06-24-R-full13/`.
- `tests/external/rust_expansion_pass_only_manifest.txt`: verifier guard
  `10 / 10` passing; latest rerun
  `tests/external/logs/rust-expansion-2026-06-24-R-full/`.
- ARM64 project suite: `28 / 28` passing; latest rerun after Loop R.

Important classification:

- The Rust expansion failing set is now empty.
- `mise`, `yazi`, and `jj` were not the same class as `delta`.
- Additional Rust rows did not reproduce the `delta` helper-thread teardown
  class, which keeps `delta` isolated as a separate blocker.
