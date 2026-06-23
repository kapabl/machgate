# External Mach-O Error Fix Loop

This plan drives the classified external Mach-O failures to either fixed or hard-stopped after bounded retries.

## Exit Criteria

Stop the full loop when one of these is true:

- all classified buckets are empty or passing
- every remaining bucket has reached 5 failed fix attempts and is marked `HARDER-EXTERNAL`

After every attempt:

- rebuild the affected target
- rerun only the affected binaries first
- update this file and `docs/EXTERNAL_MACHO_LIVE_STATUS.md`
- record logs, source files changed, and the next suspected cause

After a bucket is fixed:

- run the canonical affected-worker manifest
- merge validated passing rows into `tests/external/arm64_macho_cli_manifest.txt`
- run normal unit tests before moving to the next merged batch

## Attempt Rules

Each bucket has a maximum of 5 attempts.

Attempt loop:

1. Reproduce with current logs.
2. Make the smallest source or harness change for that bucket.
3. Rebuild.
4. Rerun the target binary set.
5. Update status:
   - `FIXED` if all bucket targets pass or move to a later, more specific bucket
   - `PARTIAL` if at least one target improves
   - `RECLASSIFIED` if the primary error changed
   - `FAILED-N` if no progress on attempt `N`
   - `HARDER-EXTERNAL` after `FAILED-5`

## Bucket Work Queue

| Priority | Bucket | Count | Targets | Owner | Attempts | Status |
| ---: | --- | ---: | --- | --- | ---: | --- |
| 1 | `LSE-ISLAND-CRASH` | 3 | `fx`, `minikube`, `kubeseal` | Worker A | 1 / 5 | fixed/reclassified |
| 2 | `LSE-POOL-EXHAUSTED` | 1 | `nu` | Worker A | 1 / 5 | fixed/reclassified |
| 3 | `SYSCALL-ISLAND-RANGE` | 7 | `k9s`, `tilt`, `vault`, `consul`, `boundary`, `tofu`, `pulumi` | Worker B | 2 / 5 | fixed/reclassified |
| 4 | `GO-LCMAIN-NULL-SEGV` | 3 | `fzf`, `gum`, `shfmt` | Worker C | 1 / 5 | reclassified with PC evidence |
| 5 | `GO-CF-PROCESS-STUBS` | 14 | `gh`, `lazygit`, `glow`, `goreleaser`, `chezmoi`, `kubectl`, `helm`, `kind`, `argocd`, `flux`, `stern`, `terraform`, `packer`, `terragrunt` | Worker D | 2 / 5 | fixed/reclassified |
| 6 | `DARWIN-LIBPROC-SYSCTL` | 1 | `procs` | Worker D | 2 / 5 | fixed/reclassified |
| 7 | `DARWIN-GETFSSTAT` | 1 | `duf` | Worker D | 2 / 5 | fixed/reclassified |
| 8 | `LOADER-MMAP-RANGE` | 1 | `nomad` | Worker E | 2 / 5 | fixed/reclassified |
| 9 | `ENV-BLOCKED` | 8 | `ninja`, `cmake`, `sqlite3`, `duckdb`, `node`, `bun`, `protoc`, `nvim` | Worker E | 2 / 5 | fixed as classification |

## Agent Ownership

### Worker A: LSE/ISA Emulation

Owns:

- `src/isa_emul.c`
- `src/isa_emul.h`
- any LSE pool sizing call sites in `src/machismo.c`, `src/loader.c`, `src/loader.h`, `src/dylib_loader.c`, `src/dylib_loader.h`
- `docs/external-fix-workers/lse-islands.md`

Primary targets:

- `fx`
- `minikube`
- `kubeseal`
- `nu`

Exit for Worker A:

- all four targets reach resolver/main without LSE pool crash/exhaustion, or 5 attempts are exhausted

### Worker B: Syscall Gateway Island Range

Owns:

- `src/syscall/syscall_gate.c`
- `src/syscall/syscall_gate.h`
- syscall pool call sites in `src/machismo.c` only if needed after coordinating in report
- `docs/external-fix-workers/syscall-islands.md`

Primary targets:

- `k9s`
- `tilt`
- `vault`
- `consul`
- `boundary`
- `tofu`
- `pulumi`

Exit for Worker B:

- syscall gate patching no longer aborts with island out-of-range for all seven targets, or 5 attempts are exhausted

### Worker C: Null SIGSEGV And Go Startup Diagnostics

Owns:

- `src/trampoline.c`
- `src/trampoline.h`
- `src/machismo.c` only for startup diagnostics and only if it does not conflict with Worker A/B changes
- `src/stack.c` if stack/apple vector fixes are needed
- `docs/external-fix-workers/go-startup.md`

Primary targets:

- `fzf`
- `gum`
- `shfmt`
- original `yq`
- original `just`

Exit for Worker C:

- null segfaults produce guest PC and instruction context, and `fzf`/`gum`/`shfmt` either pass or reclassify with concrete PC evidence, or 5 attempts are exhausted

### Worker D: Darwin Shim Surface

Owns:

- `src/shim/libsystem_shim.c`
- `src/shim/libsystem_shim.ver`
- `docs/external-fix-workers/shim-surface.md`

Primary targets:

- `gh`
- `lazygit`
- `glow`
- `goreleaser`
- `chezmoi`
- `kubectl`
- `helm`
- `kind`
- `argocd`
- `flux`
- `stern`
- `terraform`
- `packer`
- `terragrunt`
- `procs`
- `duf`

Exit for Worker D:

- repeated missing CF/process/libproc/getfsstat imports are either implemented, safely stubbed with documented semantics, or 5 attempts are exhausted

### Worker E: Loader Mapping And Environment Retest

Owns:

- `src/loader.c`
- `src/loader.h`
- `src/dylib_loader.c`
- `src/dylib_loader.h`
- `tests/external/worker_manifests/worker-5.txt`
- `docs/external-fix-workers/loader-env.md`

Primary targets:

- `nomad`
- `ninja`
- `cmake`
- `sqlite3`
- `duckdb`
- `node`
- `bun`
- `protoc`
- `nvim`

Exit for Worker E:

- `nomad` reaches resolver or is reclassified, and Worker 5 rows are rerun in a valid ARM64 Docker/sysroot path, or 5 attempts are exhausted

## Active Agents

- [x] Worker A launched: `Socrates` / `019ef30d-90ce-7301-beb7-e4a868795854`
- [x] Worker B launched: `Meitner` / `019ef30e-06ee-7420-97e8-de43077198c5`
- [x] Worker C launched: `Einstein` / `019ef30e-3b72-7552-9857-6a20479905fa`
- [x] Worker D launched: `Curie` / `019ef30e-6e9d-7c03-8603-1ea729a1239f`
- [x] Worker E launched: `Banach` / `019ef30e-9ce2-7403-8c24-1a909c120693`

## Attempt Log

- Worker A attempt 1 completed. `fx`, `minikube`, and `kubeseal` no longer crash at the LSE island pool, and `nu` no longer exhausts the LSE pool. All four now reach resolver and/or `_main`, then fail in later startup/shim buckets. See `docs/external-fix-workers/lse-islands.md`.
- Worker B completed in 2 attempts. Syscall gateway island range aborts are gone for `k9s`, `tilt`, `vault`, `consul`, `boundary`, `tofu`, and `pulumi`. Six reclassified to `_main` reached plus `SIGSEGV si_addr=NULL`; `tilt` reclassified to startup `SIGSEGV si_addr=0x0000000179a548dc`. See `docs/external-fix-workers/syscall-islands.md`.
- Worker C completed in 1 instrumentation attempt. `fzf`, `gum`, `shfmt`, and original `yq` share the Go LC_MAIN pattern: crash after `_main`, `x0=NULL`, instruction `0x3980001b`. Original `just` is separate: Rust startup/signal path, `x8=NULL`, instruction `0xf940010a`. See `docs/external-fix-workers/go-startup.md`.
- Worker D completed in 2 attempts. Worker-owned missing CF/process/libproc/getfsstat imports are resolved; all 16 targets still crash after `_main`. See `docs/external-fix-workers/shim-surface.md`.
- Worker E completed in 2 attempts. `nomad` no longer fails loader mmap and reaches `_main`; Worker 5 environment block is removed. `ninja` passes and the other seven Worker 5 rows now have real MachGate failure logs. See `docs/external-fix-workers/loader-env.md`.
