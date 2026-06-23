# Worker 5 External Mach-O Report

Scope: C/C++/Zig/single-binary terminal utilities.

Manifest: `tests/external/worker_manifests/worker-5.txt`

## Summary

- Candidates found: 8
- Downloaded and SHA256-pinned: 8
- Verified ARM64 Mach-O executable: 8
- Intended harness command:
  - `MACHGATE_RUN_EXTERNAL=1 MACHGATE_EXTERNAL_MANIFEST=$PWD/tests/external/worker_manifests/worker-5.txt BUILD_DIR=$PWD/build-arm64 MACHGATE_EXTERNAL_LOGS=$PWD/tests/external/logs/worker-5-arm64 MACHGATE_EXTERNAL_WORK=$PWD/tests/external/work/worker-5-arm64 bash tests/test_external_macho_cli.sh`
- Result in this x86_64 container: 0 passed, 8 failed before MachGate execution
- Common blocker: `qemu-aarch64: Could not open '/lib/ld-linux-aarch64.so.1': No such file or directory`
- Official result status: environment-blocked; rerun on an ARM64 Linux container or an x86_64 container with an aarch64 Linux runtime/sysroot.

## Candidates

| name | implementation family | probe | artifact | executable path | validation |
| --- | --- | --- | --- | --- | --- |
| ninja | C++ | `--version` | `ninja-mac.zip` | `ninja` | Universal Mach-O with arm64 slice |
| cmake | C++ | `--version` | `cmake-4.3.3-macos-universal.tar.gz` | `cmake-4.3.3-macos-universal/CMake.app/Contents/bin/cmake` | Universal Mach-O with arm64 slice |
| sqlite3 | C | `--version` | `sqlite-tools-osx-arm64-3530200.zip` | `sqlite3` | Mach-O arm64 |
| duckdb | C++ | `--version` | `duckdb_cli-osx-arm64.zip` | `duckdb` | Mach-O arm64 |
| node | C/C++ runtime | `--version` | `node-v26.3.1-darwin-arm64.tar.gz` | `node-v26.3.1-darwin-arm64/bin/node` | Mach-O arm64 |
| bun | Zig/C++ runtime | `--version` | `bun-darwin-aarch64.zip` | `bun-darwin-aarch64/bun` | Mach-O arm64 |
| protoc | C++ | `--version` | `protoc-35.1-osx-aarch_64.zip` | `bin/protoc` | Mach-O arm64 |
| nvim | C | `--version` | `nvim-macos-arm64.tar.gz` | `nvim-macos-arm64/bin/nvim` | Mach-O arm64 |

## Run Evidence

All eight rows downloaded, checksum-verified, and extracted through the external harness. The harness then tried to execute `build-arm64/machismo`, which is an aarch64 Linux ELF:

`build-arm64/machismo: ELF 64-bit LSB pie executable, ARM aarch64, interpreter /lib/ld-linux-aarch64.so.1`

This session is an x86_64 CentOS Stream 9 container without `/lib/ld-linux-aarch64.so.1`. Every Worker 5 probe failed with status 255 before `machismo` could start. The repeated stderr text is:

`qemu-aarch64: Could not open '/lib/ld-linux-aarch64.so.1': No such file or directory`

Representative logs:

- `tests/external/logs/worker-5-arm64/ninja.err`
- `tests/external/logs/worker-5-arm64/sqlite3.err`
- `tests/external/logs/worker-5-arm64/ninja.strace`
- `tests/external/logs/worker-5-arm64/sqlite3.strace`

I also tried `dnf install -y glibc.aarch64 libstdc++.aarch64 zlib.aarch64`; the configured CentOS repos reported no matching packages. A native `BUILD_DIR=$PWD/build` run was exploratory only and is not a valid Worker 5 result because it executes the x86_64 MachGate binary against ARM64 guest code.

## Suspected Source-Level Fixes

No concrete MachGate source fix was identified from this run. The official Worker 5 harness command fails in the host execution layer before MachGate starts.

Next useful step is to rerun this manifest on an ARM64 Linux container, or provide an aarch64 Linux sysroot for QEMU user mode, then triage real MachGate failures from the per-binary logs.
