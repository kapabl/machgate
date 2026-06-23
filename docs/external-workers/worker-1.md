# Worker 1: Rust Terminal Tools

## Scope

Owned files:

- `tests/external/worker_manifests/worker-1.txt`
- `docs/external-workers/worker-1.md`

Ignored artifacts used:

- `tests/external/cache/`
- `tests/external/work/worker-1/`
- `tests/external/logs/worker-1/`

## Candidates

| Name | Artifact | SHA256 | Binary path | Probe | Result |
| --- | --- | --- | --- | --- | --- |
| zoxide | `zoxide-0.9.9-aarch64-apple-darwin.tar.gz` | `57e733d0436309dae2ed97e46bba43937209395298e1d88812d4e893900cb40a` | `zoxide` | `--version` | PASS |
| sd | `sd-v1.1.0-aarch64-apple-darwin.tar.gz` | `4bd3c09226376ca0a1d69589c91e86276fae36c5fbaaee669afce583f6682030` | `sd-v1.1.0-aarch64-apple-darwin/sd` | `--version` | PASS |
| procs | `procs-v0.14.11-aarch64-mac.zip` | `6650e7f354c07d0319d0f771ef8bb898ce8d0c841865a96c23281eebb033cac3` | `procs` | `--version` | FAIL |
| git-cliff | `git-cliff-2.13.1-aarch64-apple-darwin.tar.gz` | `21547ae4a0421164070ab75c2522864ea5565858a011fabc5f583061b20f1226` | `git-cliff-2.13.1/git-cliff` | `--version` | PASS |
| sccache | `sccache-v0.16.0-aarch64-apple-darwin.tar.gz` | `ded590cae2c72042c61178632906bef62d635fa20d45f8b22110a2241f430960` | `sccache-v0.16.0-aarch64-apple-darwin/sccache` | `--version` | PASS |
| cargo-binstall | `cargo-binstall-aarch64-apple-darwin.zip` | `691c4fda20d2e58920dc056e4eb7dacec6de448bb547aca17b0ae819716ebef6` | `cargo-binstall` | `-V` | PASS |
| hexyl | `hexyl-v0.17.0-aarch64-apple-darwin.tar.gz` | `d8c50f5ec688faf566ea4c13d6aa12a5681dc370dab3ffba0a9db935a5986630` | `hexyl-v0.17.0-aarch64-apple-darwin/hexyl` | `--version` | PASS |
| pastel | `pastel-v0.12.0-aarch64-apple-darwin.tar.gz` | `7fd81518fac1dab7bd60e4759194a942c51bb911ef5de4513f4a9e3fb4b8ac1c` | `pastel-v0.12.0-aarch64-apple-darwin/pastel` | `--version` | PASS |
| atuin | `atuin-aarch64-apple-darwin.tar.gz` | `0a3fdc4cfa18758d6a872950625f2042233df0412a0c10d0dadf30bfc3bc6d6b` | `atuin-aarch64-apple-darwin/atuin` | `--version` | PASS |
| nu | `nu-0.113.1-aarch64-apple-darwin.tar.gz` | `458aa21674221b2e8adbd784f9ba8113288b78f3c352465cef37b114ff3dffd5` | `nu-0.113.1-aarch64-apple-darwin/nu` | `--version` | FAIL |

All selected executables were verified with `file` as `Mach-O 64-bit arm64 executable`.

## Harness Run

Command:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -w /work \
  machgate-arm64-toolchain \
  bash -lc 'MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/worker_manifests/worker-1.txt MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-1 MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-1 MACHGATE_EXTERNAL_TIMEOUT=20 bash tests/test_external_macho_cli.sh'
```

Result:

```text
8/10 external Mach-O CLI probes passed, 2 failed
```

Passing stdout:

- `zoxide`: `zoxide 0.9.9`
- `sd`: `sd 1.0.0`
- `git-cliff`: `git-cliff 2.13.1`
- `sccache`: `sccache 0.16.0`
- `cargo-binstall`: `1.20.1`
- `hexyl`: `hexyl 0.17.0`
- `pastel`: `pastel 0.12.0`
- `atuin`: `atuin 18.16.1 (671f96b60dac49d1d2de73cc0812986a5e22ce7b)`

## Failures

### procs

Status: `139` / SIGSEGV.

Logs:

- `tests/external/logs/worker-1/procs.err`
- `tests/external/logs/worker-1/procs.qemu-strace`

Evidence:

```text
resolver: symbol 'proc_listpids' not found in '/work/build-arm64/libsystem_shim.so'
resolver: symbol 'proc_pid_rusage' not found in '/work/build-arm64/libsystem_shim.so'
resolver: symbol 'proc_pidfdinfo' not found in '/work/build-arm64/libsystem_shim.so'
resolver: symbol 'proc_pidinfo' not found in '/work/build-arm64/libsystem_shim.so'
resolver: symbol 'sysctlnametomib' not found in '/work/build-arm64/libsystem_shim.so'
resolver: done — 180 binds resolved, 0 stubbed, 5 failed, 11735 rebases, 0 ctor/dtor ABI adapters, 0 variadic thunks
```

The QEMU trace reaches `_main`, installs signal/altstack state, then faults at `si_addr=NULL`.

Suspected source-level fix:

- Add minimal Darwin `libproc`/sysctl shims for `proc_listpids`, `proc_pid_rusage`, `proc_pidfdinfo`, `proc_pidinfo`, and `sysctlnametomib`, or make unresolved imports fail in a controlled way instead of leaving null call targets.

### nu

Status: `139` / SIGSEGV.

Logs:

- `tests/external/logs/worker-1/nu.err`
- `tests/external/logs/worker-1/nu.qemu-strace`

Evidence:

```text
isa_emul: island pool exhausted after 8191 patches
resolver: dylib[3] 'CoreGraphics' — no mapping, skipping
resolver: symbol 'kCFAbsoluteTimeIntervalSince1970' not found in '/work/build-arm64/libsystem_shim.so'
resolver: symbol 'kCFRunLoopDefaultMode' not found in '/work/build-arm64/libsystem_shim.so'
resolver: symbol 'kCFURLVolumeAvailableCapacityForImportantUsageKey' not found in '/work/build-arm64/libsystem_shim.so'
resolver: symbol 'CC_SHA256_Final' not found in '/work/build-arm64/libsystem_shim.so'
resolver: done — 393 binds resolved, 13 stubbed, 127 failed, 165264 rebases, 0 ctor/dtor ABI adapters, 3 variadic thunks
```

The QEMU trace reaches `_main`, installs signal/altstack state, then faults at `si_addr=NULL`.

Suspected source-level fixes:

- Increase or dynamically grow the ARMv8.1 LSE emulation island pool for very large Rust binaries.
- Add a `CoreGraphics = SKIP` or `STUB` mapping if command-line probes do not require it.
- Add minimal CoreFoundation constants/functions and CommonCrypto `CC_SHA256_Final` handling used by Rust dependencies, or route unresolved optional imports to safe stubs where appropriate.
