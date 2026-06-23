# Worker 3 Kubernetes/Cloud CLI Report

## Scope

Worker-owned files:

- `tests/external/worker_manifests/worker-3.txt`
- `docs/external-workers/worker-3.md`

Ignored artifact locations:

- `tests/external/cache/`
- `tests/external/work/`
- `tests/external/logs/`

No source files or canonical manifest files were edited.

## Candidate Sources

All candidates are famous Kubernetes/cloud command-line tools with public versioned ARM64 macOS release artifacts. Each downloadable artifact was saved under `tests/external/cache/`, SHA256-pinned in the worker manifest, extracted or copied by the external harness, and verified with `file` as a `Mach-O 64-bit arm64 executable`.

## Candidate Results

| Name | Version | Probe | Result | Evidence |
| --- | --- | --- | --- | --- |
| `kubectl` | v1.36.2 | `version --client` | FAIL 139 | `tests/external/logs/kubectl.err`, `.strace`, `.qemu-strace` |
| `helm` | v4.2.2 | `version --client` | FAIL 139 | `tests/external/logs/helm.err`, `.strace`, `.qemu-strace` |
| `k9s` | v0.51.0 | `version` | FAIL 134 | `tests/external/logs/k9s.err`, `.strace`, `.qemu-strace` |
| `kind` | v0.32.0 | `version` | FAIL 139 | `tests/external/logs/kind.err`, `.strace`, `.qemu-strace` |
| `minikube` | v1.38.1 | `version` | FAIL 139 | `tests/external/logs/minikube.err`, `.strace`, `.qemu-strace` |
| `tilt` | v0.37.4 | `version` | FAIL 134 | `tests/external/logs/tilt.err`, `.strace`, `.qemu-strace` |
| `argocd` | v3.4.4 | `version --client` | FAIL 139 | `tests/external/logs/argocd.err`, `.strace`, `.qemu-strace` |
| `flux` | v2.8.8 | `--version` | FAIL 139 | `tests/external/logs/flux.err`, `.strace`, `.qemu-strace` |
| `kubeseal` | v0.38.1 | `--version` | FAIL 139 | `tests/external/logs/kubeseal.err`, `.strace`, `.qemu-strace` |
| `stern` | v1.34.0 | `--version` | FAIL 139 | `tests/external/logs/stern.err`, `.strace`, `.qemu-strace` |

Aggregate result:

- PASS: 0
- FAIL: 10

## Run Details

Environment:

- Container: `machgate-arm64-toolchain`, `linux/arm64`, repo mounted at `/work`
- Build dir: `/work/build-arm64`
- Manifest: `/work/tests/external/worker_manifests/worker-3.txt`
- Harness: `tests/test_external_macho_cli.sh`

Commands:

```sh
MACHGATE_RUN_EXTERNAL=1 \
BUILD_DIR=/work/build-arm64 \
MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/worker_manifests/worker-3.txt \
bash tests/test_external_macho_cli.sh
```

The full worker manifest run completed with:

```text
0/10 external Mach-O CLI probes passed, 10 failed
```

Each failing row also went through the harness diagnostic reruns, producing the per-tool `strace` and `QEMU_STRACE=1` logs listed above.

## Failure Notes

Common Go CLI failure after `_main`:

- `kubectl`, `helm`, `kind`, `argocd`, `flux`, and `stern` reached `machismo: calling _main ...`, then crashed with `SIGSEGV` at `si_addr=NULL`.
- These runs have unresolved CoreFoundation-style imports. The recurring missing symbols are `CFArrayGetCount`, `CFArrayGetValueAtIndex`, `CFDataGetBytePtr`, `CFDataGetLength`, `CFStringCreateWithBytes`, `CFArrayCreateMutable`, `CFArrayAppendValue`, `CFDateCreate`, `CFDataCreate`, `CFErrorCopyDescription`, `CFErrorGetCode`, and `CFStringCreateExternalRepresentation`.
- `kubectl`, `helm`, `argocd`, and `flux` also miss `mach_vm_region` and `proc_regionfilename`.
- The harness maps `CoreFoundation` to `libsystem_shim.so` and skips `Security`; these binaries appear to make enough startup calls through those imports that unresolved function pointers become fatal.

Branch island range failures:

- `k9s` aborts before `_main`: `syscall_gate: island at 0x109b12000 too far from 0x1000d7910`.
- `tilt` aborts before `_main`: `syscall_gate: island at 0x1088c4000 too far from 0x1000ec1bc`.
- Both also emit many `isa_emul: island out of B range` lines during LSE/ISA patching (`k9s`: 2821 lines, `tilt`: 1477 lines), so larger text layouts are stressing nearby executable island allocation.

Early LSE island crashes:

- `minikube` and `kubeseal` crash before resolver output.
- Both logs stop immediately after `machismo: LSE island pool at ... (adjacent to segments)`.
- QEMU traces show `SIGSEGV` at the LSE island pool address (`minikube`: `0x106a4f000`; `kubeseal`: `0x104660000`) after MachGate makes the main text segment writable for LSE patching.

Additional notes:

- `tilt` links `libc++.1.dylib`, `CoreFoundation`, `Security`, `CoreServices`, and `libSystem.B.dylib`; the current harness has no `libc++.1.dylib` mapping and skips `CoreServices` and `Security`. It reports `59 failed` binds before the syscall-gate island failure.
- No probe produced useful stdout before failure.

## Suspected Source-Level Fixes

No source fixes were made by this worker. Suspected next fixes for the main/source owner:

1. Extend or split nearby executable branch-island allocation for syscall-gate and LSE/ISA emulation so large Go binaries can patch low `__TEXT` addresses without out-of-range direct branches.
2. Investigate LSE patcher island-pool mapping/protection for large direct binaries; `minikube` and `kubeseal` crash at the island pool before resolver setup.
3. Add minimal CoreFoundation/process-region shim coverage for Go's macOS startup paths, especially the recurring `CF*` data/array/error/string functions plus `mach_vm_region` and `proc_regionfilename`.
4. After branch-island issues are fixed, revisit skipped `Security`, `CoreServices`, and `libc++.1.dylib` mappings for `tilt` and the Go TLS/certificate lookup paths.
