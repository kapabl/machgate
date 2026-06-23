# External Mach-O Expansion Plan

## Goal

Grow the real-world ARM64 macOS CLI Mach-O corpus from 10 binaries to 50 binaries, then keep every checked-in unit test passing while fixing or documenting every external failure.

The repository must keep only reproducible metadata:

- public download URL
- SHA256
- executable path inside archive
- probe arguments
- observed status and fix notes

Third-party binaries, extracted work directories, and diagnostic logs stay ignored under `tests/external/cache/`, `tests/external/work/`, and `tests/external/logs/`.

## Exit Criteria

- [ ] `tests/external/arm64_macho_cli_manifest.txt` contains 50 pinned ARM64 macOS CLI Mach-O rows.
- [ ] Every row has a SHA256 pin and a deterministic probe, usually `--version`.
- [ ] Each failing binary has gone through the 5-try failure loop below.
- [ ] `docs/EXTERNAL_MACHO_CLI_PROBES.md` records pass/fail status, failure evidence, and fixes.
- [ ] `bash tests/fixtures/build_fixtures.sh` passes.
- [ ] `bash tests/run_tests.sh` passes.
- [ ] `MACHGATE_RUN_EXTERNAL=1 bash tests/test_external_macho_cli.sh` has been run against the 50-row corpus.

## Current Status

- Corpus size: 10 / 50
- External passing: 8 / 10
- External failing: 2 / 10
- Known failing binaries: `yq`, `just`
- Normal unit tests: 27 / 27 passing

## Outer Corpus Loop

Repeat until the manifest reaches 50 rows:

1. Pick a famous terminal-oriented macOS CLI tool with an ARM64 macOS release artifact.
2. Download the artifact into the ignored cache.
3. Verify it contains an ARM64 Mach-O executable.
4. Compute SHA256 for the original downloadable artifact.
5. Add one manifest row with URL, SHA256, archive member path, and probe args.
6. Run the binary through MachGate.
7. If it passes, update the report and continue.
8. If it fails, enter the failure loop.

Preferred candidate families:

- search/navigation: `fzf`, `zoxide`, `eza`, `sd`, `dust`, `duf`, `procs`
- data formats: `dasel`, `jless`, `fx`, `xq`, `rq`
- build/dev tools: `git-cliff`, `watchexec`, `tokei`, `sccache`, `cargo-binstall`
- infra tools: `terraform`, `packer`, `nomad`, `vault`, `consul`
- Kubernetes/cloud: `kubectl`, `helm`, `k9s`, `kind`, `tilt`
- network/security: `age`, `cosign`, `trivy`, `rclone`, `minio`
- language runtimes/tools only when the probe is simple and noninteractive.

Reject candidates that are GUI apps, installers only, x86-only, require login credentials for `--version`, or are not redistributable from a stable public release URL.

## Failure Loop

For each failing binary, stop after fixed or after 5 fix attempts.

### Attempt 1

- Read `stdout` and `stderr`.
- Check resolver output for unresolved imports, skipped dylibs, bad maps, wrong entrypoint, or obvious missing shim symbols.
- Fix the smallest visible issue.
- Rebuild and rerun the binary.

### Attempt 2

- Enable existing MachGate diagnostics:
  - `MACHGATE_TRACE_SHIM=1`
  - `MACHGATE_TRACE_SIGNALS=1`
- Rerun the binary.
- Fix the smallest ABI issue visible in the logs.
- Rebuild and rerun.

### Attempt 3

- Inspect Mach-O imports, linked dylibs, sections, and entrypoint.
- Compare with a passing binary from the same runtime family when possible.
- Fix missing symbol shims, dylib mapping, structure translation, or argument/errno translation.
- Rebuild and rerun.

### Attempt 4

- Run `strace -f` when available.
- Run QEMU syscall tracing with `QEMU_STRACE=1`.
- Preserve diagnostic paths in the report.
- Fix based on the trace.
- Rebuild and rerun.

### Attempt 5

- Add deeper targeted instrumentation around the suspected MachGate subsystem:
  - syscall gateway
  - imported libSystem shim
  - signal/altstack handling
  - pthread/TLV handling
  - dyld resolver or stubbed dylibs
- Rebuild and rerun.
- If still failing, mark `HARDER-EXTERNAL` in the report with exact evidence and next suspected subsystem.

## Documentation Rules

For every binary added:

- update the corpus table in `docs/EXTERNAL_MACHO_CLI_PROBES.md`
- record PASS or FAIL
- if FAIL, record attempt count, last visible error, diagnostic log paths, and suspected subsystem
- if fixed, record the source files changed and the behavior that changed

## Parallel Worker Protocol

Workers must not edit the canonical manifest directly.

Each worker owns one scratch manifest/report pair:

- `tests/external/worker_manifests/worker-N.txt`
- `docs/external-workers/worker-N.md`

Worker responsibilities:

1. Find 6-10 candidate public ARM64 macOS CLI release artifacts.
2. Validate download and SHA256.
3. Verify extracted executable is an ARM64 Mach-O.
4. Run each candidate through MachGate with the external harness using the worker manifest.
5. Write results and any failure evidence to the worker report.

The main thread merges validated rows into `tests/external/arm64_macho_cli_manifest.txt`, reruns the combined corpus, and owns source fixes.

## Active Work Queue

- [x] Worker 1 launched: Rust terminal tools (`Ohm`, `019ef2f7-a856-74f1-80ef-a93830120a39`).
- [x] Worker 2 launched: Go terminal tools (`Carver`, `019ef2f7-c70b-7131-b4c0-4543d1da69f1`).
- [x] Worker 3 launched: Kubernetes/cloud CLIs (`Archimedes`, `019ef2f7-edeb-79b2-8846-cca60912d595`).
- [x] Worker 4 launched: HashiCorp and infra CLIs (`Wegener`, `019ef2f8-16b7-7450-8558-c19c475a2674`).
- [x] Worker 5 launched: C/C++/Zig/single-binary utilities (`Schrodinger`, `019ef2f8-4553-7fc0-8b2d-79fbb4b075c0`).
- [~] Main: investigate existing `yq` failure.
- [~] Main: investigate existing `just` failure.
- [ ] Main: merge validated worker rows until corpus reaches 50.
- [ ] Main: run full normal unit tests.
- [ ] Main: run full external suite.

## Main Thread Notes

- Live dashboard added: `docs/EXTERNAL_MACHO_LIVE_STATUS.md`.
- `yq`: retry with `MACHGATE_TRACE_SHIM=1 MACHGATE_TRACE_SIGNALS=1` still crashes immediately after `machismo: calling _main at 0x100086050`; no shim call fires after entry. The binary is Go-style, has `LC_MAIN entryoff 548944`, no exported `_main` symbol, and imports a broad libSystem surface plus skipped `Security` and `libresolv`.
- `just`: retry with shim/signal tracing reaches Rust runtime setup. Last visible shim calls are `pthread_get_stackaddr_np`, `pthread_get_stacksize_np`, `mmap`, `mprotect`, `sigaction`, `sigaltstack`, and `pthread_threadid_np`; then it crashes with `SIGSEGV si_addr=NULL`. Suspected subsystem is Darwin signal/altstack or Rust TLV/runtime bootstrap.
- Worker manifests currently visible: worker 1, worker 2, worker 3, and worker 4. Together they list 39 new candidates, giving 49 total rows including the existing canonical 10 before merge. Worker 5 has no visible files yet.
