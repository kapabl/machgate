# External Mach-O CLI Probe Report

## Scope

This report tracks real ARM64 macOS command-line Mach-O binaries downloaded from public GitHub releases and run through MachGate inside the ARM64 Linux container.

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

Manifest with exact URLs, archive paths, and SHA256 pins:

- `tests/external/arm64_macho_cli_manifest.txt`

## Current Result

External probes currently pass for 8 of 10 binaries.

| Binary | Status | Observed output or failure |
| --- | --- | --- |
| `ripgrep` | PASS | `ripgrep 15.1.0` |
| `fd` | PASS | `fd 10.4.2` |
| `hyperfine` | PASS | `hyperfine 1.20.0` |
| `bat` | PASS | `bat 0.26.1` |
| `jq` | PASS | `jq-1.8.2` |
| `yq` | FAIL | `SIGSEGV`, `si_addr=NULL`, immediately after entering `_main` |
| `starship` | PASS | `starship 1.25.1` |
| `delta` | PASS | `delta 0.19.2` |
| `bottom` | PASS | `bottom 0.14.1` |
| `just` | FAIL | `SIGSEGV`, `si_addr=NULL`, during early Rust thread/signal setup |

## Fixes Driven By The Probes

The external binaries exposed runtime ABI gaps that the smaller unit fixtures did not hit. The current branch includes fixes for:

- Guest stack high-address and stack-size export for Darwin `pthread_get_stackaddr_np` and `pthread_get_stacksize_np`.
- Darwin `sysconf(_SC_PAGESIZE)` translation; Rust was otherwise reading Linux `_SC_VERSION` as a page size.
- `_NSGetArgc`, `_NSGetArgv`, and `_NSGetEnviron` bootstrap compatibility.
- Darwin `mmap` flag handling for imported libSystem calls.
- Darwin `pthread_setname_np`, `pthread_threadid_np`, `sigaltstack`, `sigaction`, `pthread_sigmask`, `sigemptyset`, `sigaddset`, `sigfillset`, and `sigwait` ABI wrappers.
- Minimal CoreFoundation timezone/string stubs for Rust CLI startup paths.
- QEMU user-mode syscall diagnostic logs for external failures.

## Validation

Normal test suite:

- `27/27 passed, 0 failed`

External corpus:

- `8/10 external Mach-O CLI probes passed, 2 failed`

The two remaining failures both have QEMU diagnostic logs:

- `tests/external/logs/yq.qemu-strace`
- `tests/external/logs/just.qemu-strace`

## Remaining Work

`yq` and `just` still fail before producing version output. Both failures now occur with all currently visible imports resolved, so the next debugging step is ABI-level tracing around early runtime bootstrap rather than adding more download fixtures.
