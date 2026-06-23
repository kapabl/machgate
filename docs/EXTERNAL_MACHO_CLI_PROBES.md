# External Mach-O CLI Probe Report

## Scope

This report tracks real ARM64 macOS command-line Mach-O binaries downloaded from public releases and run through MachGate inside the ARM64 Linux container.

The binaries are not vendored in this repository. The test harness downloads release assets into `tests/external/cache/`, verifies pinned SHA256 checksums, extracts the Mach-O executables into `tests/external/work/`, and writes diagnostics to `tests/external/logs/`.

## Corpus

| Binary | Version | Source | Probe |
| --- | --- | --- | --- |
| `ripgrep` | 15.1.0 | `BurntSushi/ripgrep` GitHub release | `--version` |
| `fd` | 10.4.2 | `sharkdp/fd` GitHub release | `--version` |
| `hyperfine` | 1.20.0 | `sharkdp/hyperfine` GitHub release | `--version` |
| `bat` | 0.26.1 | `sharkdp/bat` GitHub release | `--version` |
| `jq` | 1.8.2 | `jqlang/jq` GitHub release | `--version` |
| `yq` | 4.53.3 | `mikefarah/yq` GitHub release | `--version` |
| `starship` | 1.25.1 | `starship/starship` GitHub release | `--version` |
| `delta` | 0.19.2 | `dandavison/delta` GitHub release | `--version` |
| `bottom` | 0.14.1 | `ClementTsang/bottom` GitHub release | `--version` |
| `just` | 1.53.0 | `casey/just` GitHub release | `--version` |
| `zoxide` | 0.9.9 | `ajeetdsouza/zoxide` GitHub release | `--version` |
| `sd` | 1.1.0 | `chmln/sd` GitHub release | `--version` |
| `git-cliff` | 2.13.1 | `orhun/git-cliff` GitHub release | `--version` |
| `sccache` | 0.16.0 | `mozilla/sccache` GitHub release | `--version` |
| `cargo-binstall` | 1.20.1 | `cargo-bins/cargo-binstall` GitHub release | `-V` |
| `hexyl` | 0.17.0 | `sharkdp/hexyl` GitHub release | `--version` |
| `pastel` | 0.12.0 | `sharkdp/pastel` GitHub release | `--version` |
| `atuin` | 18.16.1 | `atuinsh/atuin` GitHub release | `--version` |
| `ninja` | 1.13.2 | `ninja-build/ninja` GitHub release | `--version` |

Manifest with exact URLs, archive paths, and SHA256 pins:

- `tests/external/arm64_macho_cli_manifest.txt`

Canonical row count:

- 19 total rows
- 31 additional reproducible passing rows needed to reach the 50-row target

## Current Result

External probes currently pass for 18 of 19 binaries.

| Binary | Status | Observed output or failure |
| --- | --- | --- |
| `ripgrep` | PASS | `ripgrep 15.1.0 (rev af60c2de9d)` |
| `fd` | PASS | `fd 10.4.2` |
| `hyperfine` | PASS | `hyperfine 1.20.0` |
| `bat` | PASS | `bat 0.26.1 (979ba22)` |
| `jq` | PASS | `jq-1.8.2` |
| `yq` | FAIL | `SIGSEGV`, status 139, after entering `_main` |
| `starship` | PASS | `starship 1.25.1` |
| `delta` | PASS | `delta 0.19.2` |
| `bottom` | PASS | `bottom 0.14.1` |
| `just` | PASS | `just 1.53.0` |
| `zoxide` | PASS | `zoxide 0.9.9` |
| `sd` | PASS | `sd 1.0.0` |
| `git-cliff` | PASS | `git-cliff 2.13.1` |
| `sccache` | PASS | `sccache 0.16.0` |
| `cargo-binstall` | PASS | `1.20.1` |
| `hexyl` | PASS | `hexyl 0.17.0` |
| `pastel` | PASS | `pastel 0.12.0` |
| `atuin` | PASS | `atuin 18.16.1 (671f96b60dac49d1d2de73cc0812986a5e22ce7b)` |
| `ninja` | PASS | `1.13.2` |

## Fixes Driven By The Probes

The external binaries exposed runtime ABI gaps that the smaller unit fixtures did not hit. The current branch includes fixes for:

- Guest stack high-address and stack-size export for Darwin `pthread_get_stackaddr_np` and `pthread_get_stacksize_np`.
- Darwin `sysconf(_SC_PAGESIZE)` translation; Rust was otherwise reading Linux `_SC_VERSION` as a page size.
- `_NSGetArgc`, `_NSGetArgv`, and `_NSGetEnviron` bootstrap compatibility.
- Darwin `mmap` flag handling for imported libSystem calls.
- Darwin `pthread_setname_np`, `pthread_threadid_np`, `sigaltstack`, `sigaction`, `pthread_sigmask`, `sigemptyset`, `sigaddset`, `sigfillset`, and `sigwait` ABI wrappers.
- Minimal CoreFoundation timezone/string/data/array/error stubs for CLI startup paths.
- Nearby branch-island fixes for LSE emulation and Darwin syscall gateway patching.
- Loader segment-span fixes for large external Mach-O images with zero-sized debug segments.
- QEMU user-mode syscall diagnostic logs for external failures.

## Validation

Normal test suite:

- Command: `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail; cmake --build build-arm64 --parallel; bash tests/fixtures/build_fixtures.sh; BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh'`
- Result: `27/27 passed, 0 failed`

External corpus:

- Command: `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail; MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/arm64_macho_cli_manifest.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/canonical-merge-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/canonical-merge-attempt1 MACHGATE_EXTERNAL_TIMEOUT=30 bash tests/test_external_macho_cli.sh'`
- Result: `18/19 external Mach-O CLI probes passed, 1 failed`

The remaining failure has diagnostics at:

- `tests/external/logs/canonical-merge-attempt1/yq.err`
- `tests/external/logs/canonical-merge-attempt1/yq.qemu-strace`

## Remaining Work

`yq` still fails before producing version output. Its current canonical run resolves all binds, enters `_main`, and then segfaults, matching the Go LC_MAIN startup class tracked by Worker F.

The canonical manifest is intentionally below the 50-row target because only 9 additional passing rows were known at merge time. Add 31 more reproducible passing external rows before raising the canonical corpus to 50.
