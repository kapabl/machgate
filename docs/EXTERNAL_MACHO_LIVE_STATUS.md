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
    MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/full-loop-2026-06-23-D-libcxx \
    MACHGATE_EXTERNAL_WORK=/work/tests/external/work/full-loop-2026-06-23-D-libcxx \
    MACHGATE_EXTERNAL_TIMEOUT=120 \
    bash tests/test_external_macho_cli.sh'
```

Result:

- Total visible unique external binaries: 57
- Last full-corpus passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 48 / 57 (84.2%)
- Last full-corpus not passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`: 9 / 57 (15.8%)
- Last full-corpus passing without libc++ opt-in: 45 / 57 (78.9%)
- Last full-corpus not passing without libc++ opt-in: 12 / 57 (21.1%)
- Previous full corpus before the `sqlite3` fix: 44 / 57 (77.2%)
- Canonical manifest: 19 / 19 passing
- Go TLS/signal group: `yq`, `fzf`, `gum`, `shfmt` all passing
- Full-corpus promoted fixes since the 45/57 baseline: `vault`, `duckdb`, and `node`.
- Agent status: no active local Codex, Grok, or Claude helper agents in this loop.

## Passing Binaries

`ripgrep`, `fd`, `hyperfine`, `bat`, `jq`, `yq`, `starship`, `delta`,
`bottom`, `just`, `zoxide`, `sd`, `git-cliff`, `sccache`,
`cargo-binstall`, `hexyl`, `pastel`, `atuin`, `ninja`, `procs`, `fzf`,
`gh`, `lazygit`, `glow`, `gum`, `goreleaser`, `chezmoi`, `duf`, `shfmt`,
`fx`, `kubectl`, `k9s`, `kind`, `minikube`, `argocd`, `flux`, `kubeseal`,
`stern`, `helm`, `consul`, `boundary`, `tofu`, `terragrunt`, `pulumi`,
`sqlite3`, `vault`, `duckdb`, `node`.

## Current Non-Passing Binaries

| Class | Count | Binaries | Current evidence |
| --- | ---: | --- | --- |
| SIGSEGV with missing shim/dylib surface | 5 | `nu`, `tilt`, `cmake`, `bun`, `protoc` | These crash with SIGSEGV and have unresolved or skipped dependency surface. `nu` improved from 92 to 43 failed binds after CF/CommonCrypto/libiconv/CoreServices work; `cmake`/`bun`/`protoc` now use opt-in libc++ mapping, but still need remaining shim/framework/runtime coverage. `tilt` still skips large framework/system surface. |
| SIGSEGV after startup | 4 | `terraform`, `packer`, `nomad`, `nvim` | These reach `_main` and crash later. `terraform`/`packer` show NULL SIGSEGV after Go runtime startup; `nomad` no longer has missing `CFDictionaryAddValue`/`CFStringGetSystemEncoding`, but still crashes with skipped `IOKit`, `Security`, and `libresolv.9.dylib` surface plus LSE island range warnings; `nvim` now has 0 failed binds and crashes in libuv signal teardown. |

## Failure Details

Logs for this refresh are under:

- `tests/external/logs/full-loop-2026-06-23-A/`
- `tests/external/work/full-loop-2026-06-23-A/`
- `tests/external/logs/full-loop-2026-06-23-D-libcxx/`
- `tests/external/work/full-loop-2026-06-23-D-libcxx/`
- `tests/external/logs/sqlite3-variadic-vfprintf-attempt7/`
- `tests/external/work/sqlite3-variadic-vfprintf-attempt7/`

Per-binary notes:

- `nu`: `full-loop-2026-06-23-C` still `SIGSEGV` 139 after `_main` with 46 failed binds. The remaining bind blockers are 36 skipped/no-map framework/dylib instances (`AppKit` 1 pasteboard constant, `CoreServices` 6 FSEvents functions, `IOKit` 16 HID/registry/default-port symbols, `Security` 10 certificate/trust functions, `libiconv` 3 conversion functions) plus 10 mapped `libSystem` gaps (`copyfile_state_*`, `fclonefileat`, `fcopyfile`, `fsetattrlist`, `host_statistics64`, `proc_listallpids`, `proc_pidpath`, `pthread_set_qos_class_self_np`). `CoreGraphics` and `Foundation` are skipped in the load list but have no visible `nu` bind/lazy-bind symbols. Targeted `nu-classification-attempt4` with current libiconv/CoreServices mapping still fails with `SIGSEGV` at Rust thread-local initialization (`pc=0x1018fe474`, faulting `str xzr, [x8]`).
- `tilt`: `full-loop-2026-06-23-D-libcxx` still `SIGSEGV` 139 after opt-in libc++ mapping. Resolver reports 204 binds resolved and 27 failed, with skipped `libresolv.9.dylib` and `Security` still visible.
- `terraform`: `full-loop-2026-06-23-D-libcxx` still `SIGSEGV` 139 after `_main`. Resolver reports 133 binds resolved and 10 failed from skipped `libresolv.9.dylib` and `Security`.
- `packer`: `full-loop-2026-06-23-D-libcxx` still `SIGSEGV` 139 after `_main`. Resolver reports 145 binds resolved and 10 failed from skipped `libresolv.9.dylib` and `Security`.
- `vault`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` after `libsystem_shim.c` sysctl/sysctlbyname/uname-style coverage; prints `Vault v2.0.3`.
- `nomad`: `SIGSEGV` 139 in `shim-sysctl-cf-vault-nomad-attempt1`. `CFDictionaryAddValue` and `CFStringGetSystemEncoding` now resolve; remaining visible blockers are 34 failed binds from skipped `libresolv.9.dylib`, `IOKit`, and `Security`, plus LSE island out-of-B-range warnings. qemu trace ends with `SIGSEGV si_addr=NULL` then `si_addr=0x110`.
- `cmake`: `full-loop-2026-06-23-D-libcxx` reaches `_main`, prints `CMake Error: Could not find CMAKE_ROOT !!!`, then `SIGSEGV` 139. Opt-in libc++ mapping is active; resolver reports 4437 binds resolved and 38 failed, including skipped `libcurl.4.dylib`, CommonCrypto hash symbols, CF bundle/URL/UUID helpers, `__mb_cur_max`, and `LSOpenCFURLRef`.
- `duckdb`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` with opt-in libc++ mapping.
- `node`: PASS in full-loop `full-loop-2026-06-23-D-libcxx` with opt-in libc++ mapping; prints `v26.3.1`.
- `bun`: `full-loop-2026-06-23-D-libcxx` still `SIGSEGV` 139 in the single C++ static initializer. Opt-in libc++ mapping is active, but `libicucore.A.dylib` is unmapped, the LSE island pool exhausts after 32774 patches, and resolver reports 817 binds resolved and 222 failed.
- `protoc`: `full-loop-2026-06-23-D-libcxx` still `SIGSEGV` 139 during static constructors. Opt-in libc++ mapping is active; resolver reports 1655 binds resolved and only 1 failed bind, `sigsetjmp`.
- `nvim`: `full-loop-2026-06-23-D-libcxx` still `SIGSEGV` 139 with skipped dylib surface cleared: `CoreServices` maps to `libm.so.6`, `libiconv.2.dylib` maps to `libc.so.6`, and resolver reports `414 resolved, 0 failed`. `nvim-trace-attempt4` shows `SIGSEGV si_addr=0x210` in `_uv_close` from `_signal_teardown`; `libutil.dylib` has no visible imported symbols.
- `sqlite3`: targeted PASS in `sqlite3-variadic-vfprintf-attempt7` after Darwin-shaped malloc zone, `vfprintf` variadic adapter, and access wrappers.

## Next Fix Order

1. Fix the narrowest remaining blocker first: add/verify `sigsetjmp` for `protoc`, rerun, and update the attempt count.
2. Attack Go/runtime crashes: `terraform`, `packer`, `nomad`.
3. Add focused shim/dylib surface for `nu`, `tilt`, `cmake`, `bun`, and `nvim`.
4. Keep full-corpus reruns with `MACHGATE_EXTERNAL_MAP_LIBCXX=1` after any loader, shim, syscall, or mapping change.
