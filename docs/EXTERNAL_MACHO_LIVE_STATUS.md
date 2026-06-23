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
    MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/full-loop-2026-06-23-K-loop-e-final \
    MACHGATE_EXTERNAL_WORK=/work/tests/external/work/full-loop-2026-06-23-K-loop-e-final \
    MACHGATE_EXTERNAL_TIMEOUT=120 \
    bash tests/test_external_macho_cli.sh'
```

Result:

- Total visible unique external binaries: 57
- Last full-corpus passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 51 / 57 (89.5%)
- Last full-corpus not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 6 / 57 (10.5%)
- Previous full-corpus passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 49 / 57 (86.0%)
- Previous full-corpus not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 8 / 57 (14.0%)
- Last full-corpus passing without libc++ opt-in: 45 / 57 (78.9%)
- Last full-corpus not passing without libc++ opt-in: 12 / 57 (21.1%)
- Previous full corpus before the `sqlite3` fix: 44 / 57 (77.2%)
- Canonical manifest: 19 / 19 passing
- Go TLS/signal group: `yq`, `fzf`, `gum`, `shfmt` all passing
- Full-corpus promoted fixes since the 45/57 baseline: `vault`, `duckdb`, `node`, `protoc`, `cmake`, and `nvim`.
- Agent status: Loop E Codex workers complete; Grok read-only report collected; Claude was blocked by the local session limit.

## Passing Binaries

`ripgrep`, `fd`, `hyperfine`, `bat`, `jq`, `yq`, `starship`, `delta`,
`bottom`, `just`, `zoxide`, `sd`, `git-cliff`, `sccache`,
`cargo-binstall`, `hexyl`, `pastel`, `atuin`, `ninja`, `procs`, `fzf`,
`gh`, `lazygit`, `glow`, `gum`, `goreleaser`, `chezmoi`, `duf`, `shfmt`,
`fx`, `kubectl`, `k9s`, `kind`, `minikube`, `argocd`, `flux`, `kubeseal`,
`stern`, `helm`, `consul`, `boundary`, `tofu`, `terragrunt`, `pulumi`,
`cmake`, `sqlite3`, `vault`, `duckdb`, `node`, `protoc`, `nvim`.

## Current Non-Passing Binaries

| Class | Count | Binaries | Current evidence |
| --- | ---: | --- | --- |
| SIGSEGV with missing shim/dylib surface | 2 | `nu`, `bun` | `nu` is down to `519 resolved, 13 stubbed, 1 failed` and still has skipped/non-owned AppKit/CoreGraphics/Foundation plus a post-`_main` crash. `bun` is down to `910 resolved, 0 stubbed, 129 failed`; the remaining printed gaps are ICU field-position, collation, currency, and date APIs. |
| SIGSEGV after startup/runtime | 3 | `tilt`, `terraform`, `nomad` | `tilt` has `231 resolved, 0 stubbed, 0 failed`, reaches `_main`, then crashes. `terraform` is a zero-failed-bind Go runtime crash. `nomad` remains a Go/runtime crash after the IOKit and LSE work. |
| Process/env unsupported | 1 | `packer` | `packer` exits status `2`; prior traces show child re-exec/env support is the blocker, not parent SIGSEGV. |

## Failure Details

Logs for this refresh are under:

- `tests/external/logs/full-loop-2026-06-23-A/`
- `tests/external/work/full-loop-2026-06-23-A/`
- `tests/external/logs/full-loop-2026-06-23-D-libcxx/`
- `tests/external/work/full-loop-2026-06-23-D-libcxx/`
- `tests/external/logs/full-loop-2026-06-23-E2-loop-c/`
- `tests/external/work/full-loop-2026-06-23-E2-loop-c/`
- `tests/external/logs/full-loop-2026-06-23-K-loop-e-final/`
- `tests/external/work/full-loop-2026-06-23-K-loop-e-final/`
- `tests/external/logs/sqlite3-variadic-vfprintf-attempt7/`
- `tests/external/work/sqlite3-variadic-vfprintf-attempt7/`

Per-binary notes:

- `nu`: `full-loop-2026-06-23-K-loop-e-final` still `SIGSEGV` 139 after `_main`. Resolver reports `519 resolved, 13 stubbed, 1 failed`; skipped/no-map dylibs are still `AppKit`, `CoreGraphics`, and `Foundation`, with `libobjc.A.dylib` stubbed. Native `__eh_frame` is served by the `_dl_find_object` hook.
- `tilt`: `full-loop-2026-06-23-K-loop-e-final` still `SIGSEGV` 139 after `_main`, but resolver reports `231 resolved, 0 stubbed, 0 failed`. It has no visible missing imports now; the blocker is runtime/EH/trampoline behavior after `_main`.
- `terraform`: `full-loop-2026-06-23-K-loop-e-final` still `SIGSEGV` 139 after `_main`. Resolver has no visible failed binds; remaining blocker is Go/runtime behavior, not import surface.
- `packer`: `full-loop-2026-06-23-K-loop-e-final` exits status `2`. This is the known child-process/re-exec environment class from Loop D, not a promoted pass.
- `vault`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` after `libsystem_shim.c` sysctl/sysctlbyname/uname-style coverage; prints `Vault v2.0.3`.
- `nomad`: `full-loop-2026-06-23-K-loop-e-final` still `SIGSEGV` 139 after `_main`. LSE placement is clean; remaining blocker is Go/runtime behavior plus remaining platform-probe semantics.
- `cmake`: PASS in `full-loop-2026-06-23-K-loop-e-final` after mapping `libcurl.4.dylib` to host `libcurl.so.4` and translating Darwin `dlsym` special handles such as `RTLD_DEFAULT=-2` before calling glibc.
- `duckdb`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` with opt-in libc++ mapping.
- `node`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` with opt-in libc++ mapping; prints `v26.3.1`.
- `bun`: `full-loop-2026-06-23-K-loop-e-final` still `SIGSEGV` 139 in the single C++ static initializer. LSE exhaustion is fixed. Resolver improved to `910 resolved, 0 stubbed, 129 failed`; remaining printed gaps are ICU field-position, collation, currency, and date APIs.
- `protoc`: PASS in `full-loop-2026-06-23-K-loop-e-final`; native Mach-O `__eh_frame` is served by `_dl_find_object` instead of direct `__register_frame`.
- `nvim`: PASS in `full-loop-2026-06-23-K-loop-e-final` after the `ioctl(FIONBIO)` stack-argument thunk and native Mach-O `__eh_frame` hook-only registration path.
- `sqlite3`: targeted PASS in `sqlite3-variadic-vfprintf-attempt7` after Darwin-shaped malloc zone, `vfprintf` variadic adapter, and access wrappers.

## Next Fix Order

1. Continue ICU surface for `bun`; latest blocker is `ucfpos_*`, `ucol_*`, `ucurr_*`, and `udat_close`.
2. Instrument zero-bind runtime crashes for `tilt`, `terraform`, and `nomad`.
3. Decide the production policy for `packer` child-process/re-exec support.
4. Resolve the remaining `nu` skipped framework/runtime crash class.
5. Keep full-corpus reruns with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` after any loader, shim, syscall, or mapping change.
