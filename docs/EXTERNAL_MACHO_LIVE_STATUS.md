# External Mach-O Live Status

This is the visible status board for the 50-binary external Mach-O expansion loop.

## Current Snapshot

- Agents launched: 5
- Agents completed: 5
- Agents still running: 0
- Existing canonical rows: 10
- New worker candidate rows: 47
- Visible total before merge: 57
- New worker candidates passing: 9 / 47
- Original canonical passing at last run: 9 / 10
- Total visible passing: 18 / 57
- Total visible not passing yet: 39 / 57
- Fix-loop agents launched: 5
- Fix-loop agents completed: 5
- Blocking classes fixed or reclassified: 8

## Full 57-Binary Inventory

The 57 visible binaries are all accounted for:

| Group | Count | Status |
| --- | ---: | --- |
| Original canonical corpus | 10 | 9 pass, 1 fail (`yq`) |
| New passing rows merged into canonical | 9 | 9 pass |
| New worker rows not passing yet | 38 | classified/reclassified, not merged as passing |
| Total visible corpus | 57 | 18 pass, 39 not passing |

The canonical manifest currently contains 19 rows:

- original 10 rows
- 9 newly passing rows

Canonical suite result:

- `18/19` pass
- only canonical failure: `yq`

The remaining 38 new non-passing rows are tracked in the worker reports and classification sections below. They are not in the canonical passing manifest yet because they do not produce reproducible successful probes.

## Worker Results

| Worker | Agent | Scope | Candidates | Result | Status |
| --- | --- | --- | ---: | --- | --- |
| 1 | `Ohm` / `019ef2f7-a856-74f1-80ef-a93830120a39` | Rust terminal tools | 10 | 8 pass, 2 fail | complete |
| 2 | `Carver` / `019ef2f7-c70b-7131-b4c0-4543d1da69f1` | Go terminal tools | 10 | 0 pass, 10 fail | complete |
| 3 | `Archimedes` / `019ef2f7-edeb-79b2-8846-cca60912d595` | Kubernetes/cloud CLIs | 10 | 0 pass, 10 fail | complete |
| 4 | `Wegener` / `019ef2f8-16b7-7450-8558-c19c475a2674` | HashiCorp/infra CLIs | 9 | 0 pass, 9 fail | complete |
| 5 | `Schrodinger` / `019ef2f8-4553-7fc0-8b2d-79fbb4b075c0` | C/C++/Zig/single-binary utilities | 8 | environment block removed; 1 pass, 7 real failures | complete |

Worker reports:

- `docs/external-workers/worker-1.md`
- `docs/external-workers/worker-2.md`
- `docs/external-workers/worker-3.md`
- `docs/external-workers/worker-4.md`
- `docs/external-workers/worker-5.md`

Worker manifests:

- `tests/external/worker_manifests/worker-1.txt`
- `tests/external/worker_manifests/worker-2.txt`
- `tests/external/worker_manifests/worker-3.txt`
- `tests/external/worker_manifests/worker-4.txt`
- `tests/external/worker_manifests/worker-5.txt`

## Passing New Candidates

Worker 1 found 8 new ARM64 macOS CLI binaries that already pass:

- `zoxide`
- `sd`
- `git-cliff`
- `sccache`
- `cargo-binstall`
- `hexyl`
- `pastel`
- `atuin`

Worker E converted Worker 5 from environment-blocked to real ARM64 Docker data and found 1 additional passing binary:

- `ninja`

## Failing New Candidates

Current Worker 1 failures after fix loop:

- `procs`: libproc/sysctl imports now resolve; still `SIGSEGV` after `_main`.
- `nu`: LSE pool exhaustion fixed; still reaches `_main` and crashes later with missing remaining shim/startup issues.

Current Worker 2 failures after fix loop:

- `fzf`, `gum`, `shfmt`: reclassified with guest PC evidence; Go LC_MAIN pattern, `x0=NULL`, instruction `0x3980001b`.
- `gh`, `lazygit`, `glow`, `goreleaser`, `chezmoi`: Worker D-owned CF/process imports now resolve; still `SIGSEGV` after `_main`.
- `duf`: `getfsstat` now resolves; still `SIGSEGV` after `_main`.
- `fx`: LSE island crash fixed; now reaches `_main` and joins startup/SIGSEGV bucket.

Current Worker 3 failures after fix loop:

- `kubectl`, `helm`, `kind`, `argocd`, `flux`, `stern`: Worker D-owned CF/process imports now resolve; still `SIGSEGV` after `_main`.
- `k9s`: syscall island range fixed; reaches `_main`, then `SIGSEGV si_addr=NULL`.
- `tilt`: syscall island range fixed; reclassified to startup `SIGSEGV si_addr=0x0000000179a548dc`.
- `minikube`, `kubeseal`: LSE island crash fixed; now reach `_main` and join startup/SIGSEGV bucket.

Current Worker 4 failures after fix loop:

- `terraform`, `packer`, `terragrunt`: Worker D-owned CF/getfsstat imports now resolve; still `SIGSEGV` after `_main`.
- `vault`, `consul`, `boundary`, `tofu`, `pulumi`: syscall island range fixed; reach `_main`, then `SIGSEGV si_addr=NULL`.
- `nomad`: loader mmap failure fixed; reaches resolver, syscall patching, `_main`, then `SIGSEGV si_addr=NULL`.

Current Worker 5 result after ARM64 Docker rerun:

- `ninja`: PASS.
- `cmake`: runs 326 constructors, then `SIGSEGV si_addr=NULL`.
- `sqlite3`: reaches `_main`, exits with `Error: out of memory`; still has non-covered malloc-zone/fsctl/gethostuuid surface.
- `duckdb`: reaches runtime startup, then `SIGSEGV`.
- `node`: timeout; still has large-binary LSE range warnings and missing remaining CF symbols.
- `bun`: runs one constructor, then `SIGSEGV si_addr=NULL`; missing CommonCrypto/Block symbols.
- `protoc`: runs 5 constructors, then `SIGSEGV si_addr=0x10`.
- `nvim`: crashes during EH-frame/runtime path; missing Mach/semaphore/spawn surface.

## Problem Buckets Found

1. Go `LC_MAIN` startup crash after entry.
2. Broader post-`_main` startup/runtime `SIGSEGV` across Go/Rust/C++ binaries.
3. Remaining non-covered shim surface: Security/libresolv/libiconv, CommonCrypto/Blocks, malloc-zone/fsctl/gethostuuid, Mach/semaphore/spawn.
4. Large-binary LSE range warnings still visible for `node`.
5. Rust TLV/signal/bootstrap crash path from the original `just` failure.

## Error Classification

The original 39 non-passing new worker rows have been reprocessed. The environment-blocked group is gone; Worker 5 now has real ARM64 Docker results. Current new-worker result is 9 passing and 38 failing.

| Class | Count | Binaries | Meaning |
| --- | ---: | --- | --- |
| FIXED-PASS | 9 | `zoxide`, `sd`, `git-cliff`, `sccache`, `cargo-binstall`, `hexyl`, `pastel`, `atuin`, `ninja` | These new external binaries pass through MachGate. |
| FIXED-RECLASSIFIED-LSE | 4 | `fx`, `minikube`, `kubeseal`, `nu` | LSE island crash/exhaustion is fixed; these now reach resolver and/or `_main` and fail later. |
| FIXED-RECLASSIFIED-SYSCALL-ISLAND | 7 | `k9s`, `tilt`, `vault`, `consul`, `boundary`, `tofu`, `pulumi` | Syscall branch-island range abort is fixed; these now reach startup/runtime failure. |
| FIXED-RECLASSIFIED-SHIM-IMPORTS | 16 | `gh`, `lazygit`, `glow`, `goreleaser`, `chezmoi`, `kubectl`, `helm`, `kind`, `argocd`, `flux`, `stern`, `terraform`, `packer`, `terragrunt`, `procs`, `duf` | Worker D-owned missing CF/process/libproc/getfsstat imports are fixed; these still crash after `_main`. |
| FIXED-RECLASSIFIED-LOADER | 1 | `nomad` | Loader mmap range failure is fixed; it reaches `_main` and fails later. |
| GO-LCMAIN-PC-EVIDENCE | 4 | `fzf`, `gum`, `shfmt`, original `yq` | Common Go LC_MAIN crash has concrete PC evidence: `x0=NULL`, instruction `0x3980001b`. |
| RUST-STARTUP-PC-EVIDENCE | 1 | original `just` | Separate Rust startup/signal crash: `x8=NULL`, instruction `0xf940010a`. |
| NEW-REAL-FAILURES-WORKER5 | 7 | `cmake`, `sqlite3`, `duckdb`, `node`, `bun`, `protoc`, `nvim` | Environment block is gone; these now have real failure logs and need classification in the next loop. |

Priority order after this fix loop:

1. Investigate the Go LC_MAIN launch ABI: `x0=NULL`, instruction `0x3980001b`.
2. Classify the broader post-`_main` startup crashes now that LSE/syscall/import blockers are gone.
3. Add remaining non-owned shim surface for Security/libresolv/libiconv, CommonCrypto/Blocks, malloc-zone/fsctl/gethostuuid, Mach/semaphore/spawn.
4. Investigate `node` LSE range warnings and timeout.
5. Merge passing rows into canonical manifest and rerun full external suite.

## Trace Status

- Harness-generated `QEMU_STRACE=1` logs exist for every Worker 1-4 failing binary.
- Worker 3 and Worker 4 also created per-binary `strace` log files, but some are empty in this container.
- Worker 2 added representative trace-mode logs for `fzf`, `shfmt`, and `fx`.
- Worker C added guest PC/register diagnostics for non-guard SIGSEGV.
- Located trace-backed failures: many.
- Source fixes made from this fix loop: LSE loader/pool, syscall island allocation, CF/process/libproc/getfsstat shims, loader zero-sized segment handling, signal diagnostics.

## Active Main Thread Work

No agents are currently active. The last five-agent fix loop completed.

Next fix loop order:

1. Fix Go LC_MAIN startup ABI.
2. Reclassify all post-`_main` startup/runtime crashes using the new PC diagnostics.
3. Add remaining shim surface exposed by Worker 5 and skipped dylibs.
4. Merge known passing rows into the canonical manifest until it reaches 50.
