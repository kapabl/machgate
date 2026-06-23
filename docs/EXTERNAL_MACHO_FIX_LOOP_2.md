# External Mach-O Fix Loop 2

This loop starts after the first fix loop removed the early blockers:

- LSE island crash/exhaustion
- syscall-gate island range aborts
- Worker-owned CoreFoundation/process/libproc/getfsstat missing imports
- loader mmap range failure
- Worker 5 environment-blocked runs

The remaining failures are mostly post-`_main` startup/runtime crashes.

## Exit Criteria

Stop this loop when one of these is true:

- all loop-2 buckets are fixed or reclassified into narrower buckets
- every remaining bucket has reached 5 failed attempts and is marked `HARDER-EXTERNAL`

Each worker must update its report after every attempt with:

- attempt number
- command used
- targets run
- pass/fail/reclassified result
- exact logs
- changed files
- next suspected cause

## Work Queue

| Priority | Bucket | Targets | Owner | Attempts | Status |
| ---: | --- | --- | --- | ---: | --- |
| 1 | `GO-LCMAIN-ABI` | `fzf`, `gum`, `shfmt`, original `yq`, then representative Go rows | Worker F | 0 / 5 | queued |
| 2 | `RUST-JUST-STARTUP` | original `just` | Worker G | 0 / 5 | queued |
| 3 | `REMAINING-SHIM-SURFACE` | `sqlite3`, `bun`, `nvim`, `node`, plus skipped `Security`/`libresolv`/`libiconv` cases | Worker H | 0 / 5 | queued |
| 4 | `WORKER5-REAL-FAILURES` | `cmake`, `duckdb`, `node`, `bun`, `protoc`, `nvim`, `sqlite3` | Worker I | 0 / 5 | queued |
| 5 | `CANONICAL-MERGE-AND-REGRESSION` | passing rows and full suite | Worker J | 0 / 5 | queued |

## Worker F: Go LC_MAIN ABI

Owns:

- `src/machismo.c`
- `src/stack.c`
- `src/commpage.c`
- `src/commpage.h`
- `docs/external-fix-workers/go-lcmain-abi.md`

Do not edit `src/shim/libsystem_shim.c`, `src/syscall/syscall_gate.c`, `src/isa_emul.c`, loader files, or canonical manifests.

Targets:

- `fzf`
- `gum`
- `shfmt`
- original `yq`

Known evidence:

- shared instruction: `0x3980001b`
- shared state: `x0=NULL`
- likely area: Darwin LC_MAIN launch ABI versus direct C-call entry, stack/vector layout, `argc`/`argv`/`envp`/`apple`, and Go runtime startup assumptions

Exit:

- at least one Go LC_MAIN target passes, or all four are reclassified with stronger evidence, or 5 failed attempts

## Worker G: Rust `just` Startup

Owns:

- `src/shim/libsystem_shim.c` only for pthread/signal/TLV functions
- `src/trampoline.c` only for additional diagnostics if needed
- `docs/external-fix-workers/rust-just-startup.md`

Coordinate carefully: Worker H also works in `src/shim/libsystem_shim.c`. Worker G must restrict edits to pthread/signal/TLV sections and document exact touched functions.

Target:

- original `just`

Known evidence:

- Rust startup/signal setup reaches `pthread_threadid_np`
- crash instruction: `0xf940010a`
- state: `x8=NULL`

Exit:

- `just --version` passes, or it is reclassified to a concrete shim/runtime function with PC evidence, or 5 failed attempts

## Worker H: Remaining Shim Surface

Owns:

- `src/shim/libsystem_shim.c`
- `src/shim/libsystem_shim.ver`
- `docs/external-fix-workers/remaining-shims.md`

Do not edit pthread/signal/TLV functions owned by Worker G unless the report explicitly notes no conflict.

Targets:

- `sqlite3`
- `bun`
- `nvim`
- `node`
- representative skipped `Security`/`libresolv`/`libiconv` cases

Known surface:

- malloc zone APIs
- `fsctl`
- `gethostuuid`
- CommonCrypto and Blocks symbols
- Mach/semaphore/spawn symbols
- skipped dylib handling decisions for CLI-safe stubs

Exit:

- repeated missing imports for the target set are implemented or safely stubbed, at least one target passes or reclassifies beyond missing imports, or 5 failed attempts

## Worker I: Worker 5 Real Failures

Owns:

- `docs/external-fix-workers/worker5-real-failures.md`
- optional source edits only after identifying a narrow owner and ensuring no overlap with Workers F/H/G

Targets:

- `cmake`
- `duckdb`
- `node`
- `bun`
- `protoc`
- `nvim`
- `sqlite3`

Primary job:

- classify these seven real failures using the new SIGSEGV diagnostics and existing QEMU logs
- make only non-overlapping narrow fixes if a single obvious cause is found

Exit:

- each target has a concrete class and next fix owner, at least one target passes/reclassifies, or 5 failed attempts

## Worker J: Canonical Merge And Regression

Owns:

- `tests/external/arm64_macho_cli_manifest.txt`
- `docs/EXTERNAL_MACHO_CLI_PROBES.md`
- `docs/external-fix-workers/canonical-merge.md`

Do not edit source files.

Tasks:

- merge passing rows only, starting with the 9 known passing new candidates
- keep total canonical rows at 50 unless explicitly noted
- run normal unit tests
- run the canonical external suite after merge
- update reports with exact pass/fail

Known passing new rows:

- `zoxide`
- `sd`
- `git-cliff`
- `sccache`
- `cargo-binstall`
- `hexyl`
- `pastel`
- `atuin`
- `ninja`

Exit:

- canonical manifest reaches 50 rows with reproducible pins, or report explains exactly how many more passing rows are needed

## Active Agents

- [x] Worker F launched: `Dewey` / `019ef31f-23b5-7ca3-b1a4-f56c7e34e91c`
- [x] Worker G launched: `James` / `019ef31f-56de-7e02-918c-e31399925f91`
- [x] Worker H launched: `Hubble` / `019ef31f-8576-74a3-bdfa-b1a5032937f7`
- [x] Worker I launched: `McClintock` / `019ef31f-c22a-7673-95e0-01c6343f429d`
- [x] Worker J launched: `Gibbs` / `019ef31f-f002-7b63-940a-2f2d4927221c`

## Attempt Log

No loop-2 attempts have completed yet.
