# Worker 2: Go Terminal Tools

## Scope

Worker-owned manifest:

- `tests/external/worker_manifests/worker-2.txt`

Ignored artifacts and logs used:

- `tests/external/cache/`
- `tests/external/work/`
- `tests/external/logs/`

No source files or canonical manifest rows were edited.

## Harness Run

Command run from the repository root:

```sh
docker run --rm --platform linux/arm64 -v "$PWD:/work" -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail
cmake --build build-arm64 --parallel
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/worker_manifests/worker-2.txt bash tests/test_external_macho_cli.sh'
```

Result:

- PASS: 0
- FAIL: 10
- Total: 10

All artifacts were downloaded from public GitHub release URLs, SHA256-pinned, and verified as `Mach-O 64-bit arm64 executable` after extraction or direct download.

## Candidate Results

| Name | Project | Artifact | Probe | Status | Evidence |
| --- | --- | --- | --- | --- | --- |
| `fzf` | junegunn/fzf v0.73.1 | `fzf-0.73.1-darwin_arm64.tar.gz` | `--version` | FAIL 139 | 0 failed binds; segfault immediately after `machismo: calling _main at 0x100081810`; logs: `tests/external/logs/fzf.err`, `tests/external/logs/fzf.qemu-strace` |
| `gh` | cli/cli v2.95.0 | `gh_2.95.0_macOS_arm64.zip` | `--version` | FAIL 139 | 22 failed CoreFoundation/Security-adjacent binds, then NULL segfault after `_main`; logs: `tests/external/logs/gh.err`, `tests/external/logs/gh.qemu-strace` |
| `lazygit` | jesseduffield/lazygit v0.62.2 | `lazygit_0.62.2_darwin_arm64.tar.gz` | `--version` | FAIL 139 | 21 failed binds including `mach_vm_region`, `proc_regionfilename`, and CoreFoundation symbols; NULL segfault after `_main`; logs: `tests/external/logs/lazygit.err`, `tests/external/logs/lazygit.qemu-strace` |
| `glow` | charmbracelet/glow v2.1.2 | `glow_2.1.2_Darwin_arm64.tar.gz` | `--version` | FAIL 139 | 19 failed CoreFoundation binds, then NULL segfault after `_main`; logs: `tests/external/logs/glow.err`, `tests/external/logs/glow.qemu-strace` |
| `gum` | charmbracelet/gum v0.17.0 | `gum_0.17.0_Darwin_arm64.tar.gz` | `--version` | FAIL 139 | 0 failed binds; `syscall_gate: patched 3 Darwin syscalls`; NULL segfault after `_main`; logs: `tests/external/logs/gum.err`, `tests/external/logs/gum.qemu-strace` |
| `goreleaser` | goreleaser/goreleaser v2.16.0 | `goreleaser_Darwin_arm64.tar.gz` | `--version` | FAIL 139 | 22 failed CoreFoundation/Security-adjacent binds, then NULL segfault after `_main`; logs: `tests/external/logs/goreleaser.err`, `tests/external/logs/goreleaser.qemu-strace` |
| `chezmoi` | twpayne/chezmoi v2.70.5 | `chezmoi_2.70.5_darwin_arm64.tar.gz` | `--version` | FAIL 139 | 22 failed CoreFoundation/Security-adjacent binds, then NULL segfault after `_main`; logs: `tests/external/logs/chezmoi.err`, `tests/external/logs/chezmoi.qemu-strace` |
| `duf` | muesli/duf v0.9.1 | `duf_0.9.1_darwin_arm64.tar.gz` | `--version` | FAIL 139 | missing `getfsstat`, then NULL segfault after `_main`; logs: `tests/external/logs/duf.err`, `tests/external/logs/duf.qemu-strace` |
| `shfmt` | mvdan/sh v3.13.1 | direct `shfmt_v3.13.1_darwin_arm64` | `--version` | FAIL 139 | 0 failed binds; segfault immediately after `machismo: calling _main at 0x10007bf90`; logs: `tests/external/logs/shfmt.err`, `tests/external/logs/shfmt.qemu-strace` |
| `fx` | antonmedv/fx 39.2.0 | direct `fx_darwin_arm64` | `--version` | FAIL 139 | segfaults during LSE patching before resolver completion; qemu-strace shows `si_addr=0x0000000100d83000`, the reported LSE island pool address; logs: `tests/external/logs/fx.err`, `tests/external/logs/fx.qemu-strace` |

Additional trace-mode representative logs:

- `tests/external/logs/worker-2-fzf.trace.err`
- `tests/external/logs/worker-2-shfmt.trace.err`
- `tests/external/logs/worker-2-fx.trace.err`

`MACHGATE_TRACE_SHIM=1 MACHGATE_TRACE_SIGNALS=1` did not show additional shim calls after guest entry for `fzf` or `shfmt`; both still crash immediately after entering the LC_MAIN entrypoint.

## Suspected Source-Level Fixes

1. Go LC_MAIN/runtime startup path: `fzf`, `gum`, and `shfmt` have zero failed binds but still crash with `SIGSEGV si_addr=NULL` immediately after MachGate enters the LC_MAIN entrypoint. This suggests a loader/startup, stack, TLS/TLV, commpage, signal, pthread, or Go-runtime ABI issue before the guest reaches an imported shim call.
2. Missing macOS shim surface for Go tools that pull in Security/CoreFoundation: `gh`, `lazygit`, `glow`, `goreleaser`, and `chezmoi` expose missing `CFArray*`, `CFData*`, `CFString*`, `CFError*`, and related symbols. Those are probably secondary until the base Go startup crash is fixed, but they will block these tools later.
3. Missing filesystem shim: `duf` imports `getfsstat`, which is currently unresolved and should map to Linux mount/statfs data if `duf --version` reaches that path.
4. LSE island allocation/protection: `fx` crashes before bind resolution while patching LSE atomics. The reported pool is `0x100d83000`, and qemu-strace reports `SIGSEGV si_addr=0x0000000100d83000`; the LSE patcher likely selected an island address inside the reserved Mach-O span that was not mapped or made writable before writing island code.
