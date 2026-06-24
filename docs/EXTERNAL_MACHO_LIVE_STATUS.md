# External Mach-O Live Status

This is the visible status board for the external ARM64 macOS CLI corpus.

## Current Snapshot

Last refreshed: 2026-06-23

Command:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -v /tmp/machgate-current-external-all.txt:/tmp/machgate-current-external-all.txt:ro \
  -w /work machgate-arm64-toolchain bash -lc '
    cmake --build build-arm64 --target machismo system_shim --parallel
    MACHGATE_RUN_EXTERNAL=1 \
    BUILD_DIR=/work/build-arm64 \
    MACHGATE_EXTERNAL_MANIFEST=/tmp/machgate-current-external-all.txt \
    MACHGATE_EXTERNAL_MAP_LIBCXX=1 \
    MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/full-loop-2026-06-23-N-loop-g-current \
    MACHGATE_EXTERNAL_WORK=/work/tests/external/work/full-loop-2026-06-23-N-loop-g-current \
    MACHGATE_EXTERNAL_TIMEOUT=30 \
    bash tests/test_external_macho_cli.sh'
```

Result:

- Total visible unique external binaries: 57
- Latest strict full-corpus passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 51 / 57 (89.5%)
- Latest strict full-corpus not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 6 / 57 (10.5%)
- Latest functional passing after classifying `node` with a 120s targeted rerun plus targeted `tilt`, `bun`, and `nu`: 55 / 57 (96.5%)
- Latest functional blockers after excluding the `node` timeout artifact and targeted `tilt` / `bun` / `nu` passes: 2 / 57 (3.5%)
- Previous full-corpus passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 49 / 57 (86.0%)
- Previous full-corpus not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 8 / 57 (14.0%)
- Last full-corpus passing without libc++ opt-in: 45 / 57 (78.9%)
- Last full-corpus not passing without libc++ opt-in: 12 / 57 (21.1%)
- Previous full corpus before the `sqlite3` fix: 44 / 57 (77.2%)
- Canonical manifest: 19 / 19 passing
- Go TLS/signal group: `yq`, `fzf`, `gum`, `shfmt` all passing
- Full-corpus promoted fixes since the 45/57 baseline: `vault`, `duckdb`, `node`, `protoc`, `cmake`, `nvim`, `nomad`, and `terraform`.
- Agent status: Loop H/I Codex workers complete; Loop J Delta explorers complete; Loop L Grok Delta tournament complete; Grok reports collected for Delta and Packer.

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

Additional functional pass with longer timeout:

`node` passes targeted with `MACHGATE_EXTERNAL_TIMEOUT=120`.

Additional targeted pass pending full-corpus promotion:

`tilt` passes targeted in `loop-h-tilt-pthread-kill-attempt1`.
`bun` passes targeted in `bun-loop-h-attempt1-tpidr-rt`.
`nu` passes targeted in `loop-i-nu-tlv-x1-attempt1`.

## Current Non-Passing Binaries

| Class | Count | Binaries | Current evidence |
| --- | ---: | --- | --- |
| Helper-thread teardown crash | 1 | `delta` | Still failing. Loop L Grok tournament corrected the stale test-binary issue and scored candidates against `build-arm64/machgate`; the best retained shared VM-tracker candidate reached only `1/5`, not the `5/5` acceptance gate. Current evidence says the flaky helper `mmap(NULL,384)` / `munmap(addr,4096)` path bypasses both the Darwin syscall gate and shim wrapper and reaches a direct glibc/QEMU `munmap`, where `EINVAL` correlates with pass and `EINTR` correlates with `SIGSEGV`. |
| Process/fork-exec handoff | 1 | `packer` | Improved but still failing. Raw Darwin `execve` re-exec works, the shim `execve` wrapper is reached, and raw/shim write paths now guard fork-child `SIGPIPE`; latest targeted probe `loop-i-packer-raw-write-guard-attempt4` remains `0/1`, status `2`, with no `SIGPIPE`, `EPIPE`, or child `si_status=13`. Current blocker is later Go process/wait/runtime timing after pipe/fork/wait activity. |

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

- `delta`: `full-loop-2026-06-23-N-loop-g-current` fails with `SIGSEGV` 139. Loop L Grok retested against the correct `machgate` binary and kept the shared `guest_vm_track`/dispatch candidate because it is architecturally useful, but it is not an accepted Delta fix: best result was `1 / 5`. Current blocker is a helper-thread teardown path where `mmap(NULL,384)` / `munmap(addr,4096)` bypasses the shim and syscall gate and reaches a direct host glibc/QEMU `munmap`; passing runs see `EINVAL`, failing runs see `EINTR` followed by `SIGSEGV`.
- `nu`: PASS targeted in `loop-i-nu-tlv-x1-attempt1`, stdout from `nu --version`; reconfirmed in `loop-i-nu-tlv-x1-post-delta-backout`. The fixed gap was `_tlv_bootstrap` clobbering `x1` across the Mach-O TLV thunk; Rust kept a `LocalKey` initialization pointer in `x1` across the thunk and later faulted in `get_or_init_slow`. The shim TLV wrapper now preserves `x1` alongside `x8` through `x11` and `x30`. Guard run `loop-i-nu-tlv-x1-guards` kept `tilt`, `bun`, and `protoc` passing, and `loop-i-nu-tlv-x1-just-guard` kept `just` passing. Full corpus has not yet been rerun, so the last strict full-run record still contains the old nu failure.
- `tilt`: PASS targeted in `loop-h-tilt-pthread-kill-attempt1`, stdout `v0.37.4, built 2026-06-16`. The exact fixed gap was flat `_pthread_kill` resolving to glibc and delivering Darwin signal `16` as Linux `SIGSTKFLT`; `pthread_kill` now prefers mapped libSystem for flat runtime lookup and reaches the existing shim path. Full corpus has not yet been rerun, so the last strict full-run record still contains the old status `144` failure.
- `terraform`: PASS in `full-loop-2026-06-23-N-loop-g-current` after Darwin-shaped `getaddrinfo`, `freeaddrinfo`, and `gai_strerror`. Guard run `loop-g-getaddrinfo-guards` kept `lazygit` and `nomad` passing.
- `packer`: latest Loop I targeted probe `loop-i-packer-raw-write-guard-attempt4` remains `0/1`, status `2`, but the crash class moved. Raw Darwin `execve` re-exec works, the shim `execve` wrapper is reached, and `tests/test_darwin_execve.sh` passes. Loop I first confirmed Grok's diagnosis that shim-only `write`/`writev` protection missed raw Darwin `write_nocancel`; after routing shim and raw write/writev paths through fork-child SIGPIPE guards, the latest trace no longer shows child `SIGPIPE`/status `13`. The remaining blocker is later Go process/wait/runtime behavior: pipe/fork/wait activity succeeds far enough to reach futex/sleep timing and status `2`.
- `vault`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` after `libsystem_shim.c` sysctl/sysctlbyname/uname-style coverage; prints `Vault v2.0.3`.
- `nomad`: PASS in `full-loop-2026-06-23-N-loop-g-current`; Loop F fixed its `host_processor_info` Darwin CPU-load layout crash.
- `cmake`: PASS in `full-loop-2026-06-23-K-loop-e-final` after mapping `libcurl.4.dylib` to host `libcurl.so.4` and translating Darwin `dlsym` special handles such as `RTLD_DEFAULT=-2` before calling glibc.
- `duckdb`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` with opt-in libc++ mapping.
- `node`: strict 30s full run `full-loop-2026-06-23-N-loop-g-current` timed out during startup; Loop H strict rerun `loop-h-node-strict30-attempt1` still timed out, but targeted `loop-h-node-long120-attempt1` passes, prints version, and resolves cleanly with `4554 resolved, 0 stubbed, 0 failed`. Remaining work is startup-cost/timeout tuning, not missing shim surface.
- `bun`: PASS targeted in `bun-loop-h-attempt1-tpidr-rt`, stdout `1.3.14`. The old clean-import crash at `pc=0x1007d0a5c` was an inlined lock/runtime path reading `TPIDRRO_EL0` into `x9`; the loader now rewrites all `MRS TPIDRRO_EL0, Xt` encodings to `MRS TPIDR_EL0, Xt` instead of only the `x0` encoding. Guard run `bun-loop-h-tpidr-guards` kept `cmake`, `node`, `protoc`, and `nvim` passing at `4/4`. Full corpus has not yet been rerun, so the last strict full-run record still contains the old bun failure.
- `protoc`: PASS in `full-loop-2026-06-23-K-loop-e-final`; native Mach-O `__eh_frame` is served by `_dl_find_object` instead of direct `__register_frame`.
- `nvim`: PASS in `full-loop-2026-06-23-K-loop-e-final` after the `ioctl(FIONBIO)` stack-argument thunk and native Mach-O `__eh_frame` hook-only registration path.
- `sqlite3`: targeted PASS in `sqlite3-variadic-vfprintf-attempt7` after Darwin-shaped malloc zone, `vfprintf` variadic adapter, and access wrappers.

## Next Fix Order

1. Run the full 57-binary corpus with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` to promote the targeted `tilt`, `bun`, and `nu` passes.
2. Continue `delta` helper-thread teardown diagnosis before `sigaltstack` disable; Loop I Grok recommends deterministic Darwin `munmap` handling for the tiny scratch mapping `EINTR`/`EINVAL` divergence.
3. Continue `packer` after the raw write SIGPIPE fix; remaining evidence points past SIGPIPE into Go process/wait/runtime timing.
4. Tune `node` strict 30s startup time only if strict full-run timing remains a release gate.
