# Worker 4 - HashiCorp and Infrastructure CLIs

## Scope

Worker manifest: `tests/external/worker_manifests/worker-4.txt`

Artifacts were downloaded into ignored `tests/external/cache/` paths and verified with `sha256sum` plus `file`. Every selected executable is reported by `file` as `Mach-O 64-bit arm64 executable`.

## Run

Command:

```sh
docker run --rm -v "$PWD:/work" -w /work machgate-arm64-toolchain:latest bash -lc 'MACHGATE_RUN_EXTERNAL=1 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/worker_manifests/worker-4.txt BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_TIMEOUT=20 bash tests/test_external_macho_cli.sh'
```

Result: 0/9 passed, 9 failed.

`strace` log files were created but are empty in this container run. `QEMU_STRACE=1` logs were populated under `tests/external/logs/*.qemu-strace`.

## Candidates

| Name | Version | Source | Probe | Status | Evidence |
| --- | --- | --- | --- | --- | --- |
| terraform | 1.15.6 | HashiCorp release zip | `--version` | FAIL | Reaches `_main`, then SIGSEGV. Resolver reports 22 missing CoreFoundation symbols including `CFArrayGetCount`, `CFDataGetBytePtr`, `CFStringCreateWithBytes`, and `CFStringCreateExternalRepresentation`. Logs: `tests/external/logs/terraform.err`, `tests/external/logs/terraform.qemu-strace`. |
| packer | 1.15.4 | HashiCorp release zip | `--version` | FAIL | Reaches `_main`, then SIGSEGV. Same CoreFoundation missing-symbol cluster plus `getfsstat`. Logs: `tests/external/logs/packer.err`, `tests/external/logs/packer.qemu-strace`. |
| vault | 2.0.3 | HashiCorp release zip | `--version` | FAIL | Aborts before `_main`: many `isa_emul: island out of B range` lines, then `syscall_gate: island at 0x11c89c000 too far from 0x1000a8900`. Logs: `tests/external/logs/vault.err`, `tests/external/logs/vault.qemu-strace`. |
| consul | 2.0.1 | HashiCorp release zip | `--version` | FAIL | Aborts before `_main`: many LSE island range failures, then `syscall_gate: island at 0x10afc0000 too far from 0x1000d7080`. Logs: `tests/external/logs/consul.err`, `tests/external/logs/consul.qemu-strace`. |
| nomad | 2.0.3 | HashiCorp release zip | `--version` | FAIL | Loader exits with `Cannot mmap anonymous memory range: Cannot allocate memory`. Logs: `tests/external/logs/nomad.err`, `tests/external/logs/nomad.qemu-strace`. |
| boundary | 0.21.3 | HashiCorp release zip | `--version` | FAIL | Aborts before `_main`: many LSE island range failures, then `syscall_gate: island at 0x10dec4000 too far from 0x100083374`. Logs: `tests/external/logs/boundary.err`, `tests/external/logs/boundary.qemu-strace`. |
| tofu | 1.12.3 | OpenTofu release zip | `--version` | FAIL | Aborts before `_main`: LSE island range warnings, then `syscall_gate: island at 0x1089d2000 too far from 0x1000a4adc`. Logs: `tests/external/logs/tofu.err`, `tests/external/logs/tofu.qemu-strace`. |
| terragrunt | 1.0.8 | GitHub direct binary | `--version` | FAIL | Reaches `_main`, then SIGSEGV. Resolver reports 22 missing CoreFoundation symbols. Logs: `tests/external/logs/terragrunt.err`, `tests/external/logs/terragrunt.qemu-strace`. |
| pulumi | 3.247.0 | GitHub release tarball | `--version` | FAIL | Aborts before `_main`: LSE island range warnings, then `syscall_gate: island at 0x108717000 too far from 0x1000d83cc`. Logs: `tests/external/logs/pulumi.err`, `tests/external/logs/pulumi.qemu-strace`. |

## Suspected Source-Level Fixes

1. Branch-island placement needs to handle large Go Mach-O images. `vault`, `consul`, `boundary`, `tofu`, and `pulumi` all show LSE branch-island range failures and then syscall gateway island addresses outside ARM64 direct-branch range for early text addresses. Likely fixes are closer per-region island pools, multiple pools, or a different long-branch strategy for syscall gateway and LSE patch sites.
2. Minimal CoreFoundation/Security shim coverage is needed for Go CLI startup paths that reach `_main`. `terraform`, `packer`, and `terragrunt` null-segv after unresolved CF symbols. The recurring missing set is `CFArrayGetCount`, `CFArrayGetValueAtIndex`, `CFDataGetBytePtr`, `CFDataGetLength`, `CFStringCreateWithBytes`, `CFArrayCreateMutable`, `CFArrayAppendValue`, `CFDateCreate`, `CFDataCreate`, `CFErrorCopyDescription`, `CFErrorGetCode`, and `CFStringCreateExternalRepresentation`. `packer` also needs `getfsstat`.
3. Segment reservation/range sizing needs investigation for `nomad`. It fails during mapping with `Cannot mmap anonymous memory range: Cannot allocate memory` before resolver output.

## Notes

All artifacts are pinned in the worker manifest. No canonical manifest or source files were edited.
