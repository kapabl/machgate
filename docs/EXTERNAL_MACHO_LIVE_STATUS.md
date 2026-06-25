# External Mach-O Live Status

This is the visible status board for the external ARM64 macOS CLI corpus.

## Current Snapshot

Last refreshed: 2026-06-24

Command:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -v /tmp/machgate-current-external-all.txt:/tmp/machgate-current-external-all.txt:ro \
  -w /work machgate-arm64-toolchain bash -lc '
    cmake --build build-arm64 --target machgate system_shim --parallel
    MACHGATE_RUN_EXTERNAL=1 \
    BUILD_DIR=/work/build-arm64 \
    MACHGATE_EXTERNAL_MANIFEST=/tmp/machgate-current-external-all.txt \
    MACHGATE_EXTERNAL_MAP_LIBCXX=1 \
    MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/full-loop-2026-06-24-R-functional120 \
    MACHGATE_EXTERNAL_WORK=/work/tests/external/work/full-loop-2026-06-24-R-functional120 \
    MACHGATE_EXTERNAL_TIMEOUT=120 \
    bash tests/test_external_macho_cli.sh'
```

Result:

- Total visible unique external binaries: 57
- Latest integrated functional full-corpus passing with `MACHGATE_EXTERNAL_TIMEOUT=120`: 57 / 57 (100.0%)
- Latest integrated functional full-corpus not passing with `MACHGATE_EXTERNAL_TIMEOUT=120`: 0 / 57 (0.0%)
- Latest strict full-corpus passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 51 / 57 (89.5%)
- Latest strict full-corpus not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 6 / 57 (10.5%)
- Latest functional passing after classifying `node` with a 120s targeted rerun plus targeted `tilt`, `bun`, `nu`, `delta`, and `packer`: 57 / 57 (100.0%)
- Latest functional blockers after excluding the `node` timeout artifact and targeted `tilt` / `bun` / `nu` / `delta` / `packer` passes: 0 / 57 (0.0%)
- Previous full-corpus passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 49 / 57 (86.0%)
- Previous full-corpus not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 8 / 57 (14.0%)
- Last full-corpus passing without libc++ opt-in: 45 / 57 (78.9%)
- Last full-corpus not passing without libc++ opt-in: 12 / 57 (21.1%)
- Previous full corpus before the `sqlite3` fix: 44 / 57 (77.2%)
- Canonical manifest: 19 / 19 passing
- Go TLS/signal group: `yq`, `fzf`, `gum`, `shfmt` all passing
- Full-corpus promoted fixes since the 45/57 baseline: `vault`, `duckdb`, `node`, `protoc`, `cmake`, `nvim`, `nomad`, `terraform`, `tilt`, `bun`, `nu`, `delta`, and `packer` under the integrated 120s functional gate.
- Agent status: Loop H/I Codex workers complete; Loop J Delta explorers complete; Loop L Grok Delta tournament complete; Grok reports collected for Delta and Packer; Loop Q Go-runtime comparison and SIGCHLD fix completed.
- Failure-class reference: `docs/HOST_GLIBC_BYPASS_FAILURES.md` documents the host/glibc bypass pattern accepted for the targeted Delta fix.
- Packer fix reference: `docs/PACKER_SIGCHLD_FIX.md` documents the accepted SIGCHLD policy and rejected dispatcher attempt.

## Passing Binaries

Strict 30s full-corpus passing set:

`ripgrep`, `fd`, `hyperfine`, `bat`, `jq`, `yq`, `starship`,
`bottom`, `just`, `zoxide`, `sd`, `git-cliff`, `sccache`,
`cargo-binstall`, `hexyl`, `pastel`, `atuin`, `ninja`, `procs`, `fzf`,
`gh`, `lazygit`, `glow`, `gum`, `goreleaser`, `chezmoi`, `duf`, `shfmt`,
`fx`, `kubectl`, `k9s`, `kind`, `minikube`, `argocd`, `flux`, `kubeseal`,
`stern`, `helm`, `terraform`, `vault`, `consul`, `nomad`, `boundary`,
`tofu`, `terragrunt`, `pulumi`, `cmake`, `sqlite3`, `duckdb`, `protoc`,
`nvim`.

Integrated 120s full-corpus pass:

`node`, `tilt`, `bun`, `nu`, `delta`, and `packer` all pass in
`full-loop-2026-06-24-R-functional120`. `packer` also passed targeted `5 / 5`
in `loop-q4-packer-sigchld-production-attempt{1..5}` and reconfirmed `5 / 5`
in `loop-q6-packer-post-unitfix-attempt{1..5}`.

## Current Non-Passing Binaries

| Class | Count | Binaries | Current evidence |
| --- | ---: | --- | --- |
| None | 0 | none | No current functional blocker remains. The latest integrated 120s full-corpus run `full-loop-2026-06-24-R-functional120` passes `57 / 57`; strict 30s remains a timing gate because `node` needs the longer timeout under QEMU. |

## Focused C++ / Test-Runner Research

The C++ static-initializer research loop is tracked in
`docs/CATCH2_STATIC_INIT_EXTERNAL_LOOP.md`.

Latest focused result:

- Generated local `cpp_many_ctors` fixture: `707 / 707` constructors completed, exit `0`.
- Optional generated local `cpp_public_test_registrars` fixture: `707 / 707` source-derived Catch2/gtest-style registrar constructors completed, exit `0` when enabled with `MACHGATE_TEST_CPP_PUBLIC_REGISTRARS=1`.
- Public C++ focused rows after Loop K/L: `18 / 22` passing.
- Passing focused rows: `cmake`, `ctest`, `protoc`, `duckdb`, `ninja`, `bitcoin-util`, `knots-util`, `elements-util`, `dash-util`, `syscoin-cli`, `bitcoin-test`, `bitcoin-30.2-test`, `bitcoin-28.2-test`, `bitcoin-27.2-test`, `bitcoin-25.2-test`, `elements-test`, `dash-test`, and `groestlcoin-test`.
- Current focused failing rows: `bitcoin-29.2-test`, `bitcoin-26.2-test`, `knots-test`, and `qtum-test`.
- All current focused failing rows are real external C++ Boost.Test runners. The original `BOOST_AUTO_START_DBG` parser failure is fixed, and the original `TIMEOUT_AFTER_MAIN` bucket is fixed `8 / 8`. The remaining failure bucket is `BOOST_NOTHING_TO_TEST_ABORT` (4): the binary prints Boost help for `--help`, then aborts with `boost::unit_test::framework::nothing_to_test`.
- Argv-only triage did not make the original four test-runner failures pass; logs are under `tests/external/logs/cpp-static-init-argv-matrix/`.

## Timeout-Only Functional Passes

| Class | Count | Binaries | Current evidence |
| --- | ---: | --- | --- |
| Strict 30s QEMU timeout | 1 | `node` | Strict 30s full run still times out under QEMU. Targeted 120s rerun passes with resolver summary `4554 resolved, 0 stubbed, 0 failed`. This is a startup-cost/timeout item, not a functional blocker or active non-pass. |

## Failure Details

Logs for this refresh are under:

- `tests/external/logs/full-loop-2026-06-23-A/`
- `tests/external/work/full-loop-2026-06-23-A/`
- `tests/external/logs/full-loop-2026-06-23-D-libcxx/`
- `tests/external/work/full-loop-2026-06-23-D-libcxx/`
- `tests/external/logs/full-loop-2026-06-23-E2-loop-c/`
- `tests/external/work/full-loop-2026-06-23-E2-loop-c/`
- `tests/external/logs/full-loop-2026-06-23-N-loop-g-current/`
- `tests/external/work/full-loop-2026-06-23-N-loop-g-current/`
- `tests/external/logs/full-loop-2026-06-24-Q-functional120/`
- `tests/external/work/full-loop-2026-06-24-Q-functional120/`
- `tests/external/logs/full-loop-2026-06-24-R-functional120/`
- `tests/external/work/full-loop-2026-06-24-R-functional120/`
- `tests/external/logs/loop-g-node-long120/`
- `tests/external/logs/loop-h-packer-current-final/`
- `tests/external/logs/loop-h-packer-sigpipe-guard-attempt2/`
- `tests/external/logs/loop-h-packer-forksafe-attempt1-real/`
- `tests/external/logs/loop-h-packer-forksafe-attempt2-shared/`
- `tests/external/logs/loop-h-packer-mac-execve-attempt3/`
- `tests/external/logs/loop-i-packer-write-sigpipe-attempt1/`
- `tests/external/logs/loop-i-packer-forkchild-sigpipe-ignore-attempt2/`
- `tests/external/logs/loop-i-packer-long180-attempt3/`
- `tests/external/logs/loop-i-packer-raw-write-guard-attempt4/`
- `tests/external/logs/loop-h-tilt-pthread-kill-attempt1/`
- `tests/external/logs/loop-h-tilt-pthread-kill-guards-rerun/`
- `tests/external/logs/bun-loop-h-attempt1-tpidr-rt/`
- `tests/external/logs/bun-loop-h-tpidr-guards/`
- `tests/external/logs/loop-i-nu-tlv-x1-attempt1/`
- `tests/external/logs/loop-i-nu-tlv-x1-guards/`
- `tests/external/logs/loop-i-nu-tlv-x1-just-guard/`
- `tests/external/logs/loop-i-nu-tlv-x1-post-delta-backout/`
- `tests/external/logs/loop-i-delta-munmap-noeintr-attempt1-*/`
- `tests/external/logs/loop-i-delta-small-mmap-shim-attempt2-*/`
- `tests/external/logs/loop-i-delta-proc-readlink-attempt3-*/`
- `tests/external/logs/loop-m-delta-vm-interpose-attempt{1..5}/`
- `tests/external/logs/loop-q2-sigchld-default-experiment/`
- `tests/external/logs/loop-q3-sigchld-darwin-handler/`
- `tests/external/logs/loop-q4-packer-sigchld-production-attempt{1..5}/`
- `tests/external/logs/loop-q5-sigchld-guards/`
- `tests/external/logs/loop-q6-packer-post-unitfix-attempt{1..5}/`
- `tests/external/logs/loop-j-packer-guest-path-attempt1/`
- `tests/external/logs/loop-j-delta-shim-exports-attempt*/`
- `tests/external/logs/loop-j-delta-lifecycle-attempt*/`
- `tests/external/logs/loop-j-delta-altstack-guard-attempt*/`
- `tests/external/logs/loop-j-delta-tracked-munmap-attempt*/`
- `tests/external/logs/loop-j-delta-proc-filter-attempt*/`
- `tests/external/logs/loop-j-delta-shimtrace-attempt1/`
- `tests/external/logs/loop-l-machgate-final/`
- `tests/external/logs/loop-h-delta-munmap-retry-*/`
- `tests/external/logs/loop-h-delta-exitgroup-*/`
- `tests/external/logs/loop-h-delta-small-munmap-*/`
- `tests/external/logs/loop-h-delta-shim-munmap-*/`
- `tests/external/logs/loop-g-terraform-getaddrinfo-attempt1/`
- `tests/external/logs/loop-g-getaddrinfo-guards/`
- `tests/external/logs/sqlite3-variadic-vfprintf-attempt7/`
- `tests/external/work/sqlite3-variadic-vfprintf-attempt7/`

Per-binary notes:

- `delta`: PASS targeted `5 / 5` in `loop-m-delta-vm-interpose-attempt{1..5}`, stdout `delta 0.19.2`, and promoted by integrated functional full corpus `full-loop-2026-06-24-R-functional120`. Loop M accepted a VM interposer fix from the Grok best-of-9 candidate family, but tightened it for the installed release layout. `libmachgate_vm_interpose.so` is preloaded by a one-time loader re-exec and interposes host/glibc `mmap` / `munmap` calls that bypass imported libSystem symbols, forwarding them to `machgate_darwin_mmap` / `machgate_darwin_munmap` and the shared sub-page VM tracker.
- `nu`: PASS targeted in `loop-i-nu-tlv-x1-attempt1`, stdout from `nu --version`, reconfirmed in `loop-i-nu-tlv-x1-post-delta-backout`, and promoted by integrated functional full corpus `full-loop-2026-06-24-R-functional120`. The fixed gap was `_tlv_bootstrap` clobbering `x1` across the Mach-O TLV thunk; Rust kept a `LocalKey` initialization pointer in `x1` across the thunk and later faulted in `get_or_init_slow`. The shim TLV wrapper now preserves `x1` alongside `x8` through `x11` and `x30`.
- `tilt`: PASS targeted in `loop-h-tilt-pthread-kill-attempt1`, stdout `v0.37.4, built 2026-06-16`, and promoted by integrated functional full corpus `full-loop-2026-06-24-R-functional120`. The exact fixed gap was flat `_pthread_kill` resolving to glibc and delivering Darwin signal `16` as Linux `SIGSTKFLT`; `pthread_kill` now prefers mapped libSystem for flat runtime lookup and reaches the existing shim path.
- `terraform`: PASS in `full-loop-2026-06-23-N-loop-g-current` after Darwin-shaped `getaddrinfo`, `freeaddrinfo`, and `gai_strerror`. Guard run `loop-g-getaddrinfo-guards` kept `lazygit` and `nomad` passing.
- `packer`: PASS targeted `5 / 5` in `loop-q4-packer-sigchld-production-attempt{1..5}`, reconfirmed `5 / 5` in `loop-q6-packer-post-unitfix-attempt{1..5}`, and promoted by integrated functional full corpus `full-loop-2026-06-24-R-functional120`; stdout `Packer v1.15.4`. Source context is available at `.source-context/packer-1.15.4` and `.source-context/panicwrap-1.0.0`. Loop Q proved the remaining failure was Linux `SIGCHLD=17` being delivered into Darwin Go runtime signal machinery that expects Darwin `SIGCHLD=20`; a broad C signal-dispatch experiment crashed because it did not preserve Go's signal trampoline ABI. The accepted shim policy reports success for Darwin `sigaction(SIGCHLD, ...)` but leaves the host Linux `SIGCHLD` disposition at default unless `MACHGATE_ENABLE_HOST_SIGCHLD_HANDLER=1` is explicitly set. Blocking `wait4` still carries the child result, and normal Packer parent/child flow exits `0`.
- `vault`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` after `libsystem_shim.c` sysctl/sysctlbyname/uname-style coverage; prints `Vault v2.0.3`.
- `nomad`: PASS in `full-loop-2026-06-23-N-loop-g-current`; Loop F fixed its `host_processor_info` Darwin CPU-load layout crash.
- `cmake`: PASS in `full-loop-2026-06-23-K-loop-e-final` after mapping `libcurl.4.dylib` to host `libcurl.so.4` and translating Darwin `dlsym` special handles such as `RTLD_DEFAULT=-2` before calling glibc.
- `duckdb`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` with opt-in libc++ mapping.
- `node`: strict 30s full run `full-loop-2026-06-23-N-loop-g-current` timed out during startup, but integrated functional full corpus `full-loop-2026-06-24-R-functional120` passes. Remaining work is startup-cost/timeout tuning only if strict 30s is a release gate.
- `bun`: PASS targeted in `bun-loop-h-attempt1-tpidr-rt`, stdout `1.3.14`, and promoted by integrated functional full corpus `full-loop-2026-06-24-R-functional120`. The old clean-import crash at `pc=0x1007d0a5c` was an inlined lock/runtime path reading `TPIDRRO_EL0` into `x9`; the loader now rewrites all `MRS TPIDRRO_EL0, Xt` encodings to `MRS TPIDR_EL0, Xt` instead of only the `x0` encoding.
- `protoc`: PASS in `full-loop-2026-06-23-K-loop-e-final`; native Mach-O `__eh_frame` is served by `_dl_find_object` instead of direct `__register_frame`.
- `nvim`: PASS in `full-loop-2026-06-23-K-loop-e-final` after the `ioctl(FIONBIO)` stack-argument thunk and native Mach-O `__eh_frame` hook-only registration path.
- `sqlite3`: targeted PASS in `sqlite3-variadic-vfprintf-attempt7` after Darwin-shaped malloc zone, `vfprintf` variadic adapter, and access wrappers.

## Next Fix Order

1. Optional: tune `node` strict 30s startup time only if strict full-run timing remains a release gate.
2. Optional: rerun the full 57-binary corpus with a strict 30s timeout after startup-cost work.
