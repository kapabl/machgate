# Worker D: Darwin Shim Surface

Owned source scope:

- `src/shim/libsystem_shim.c`
- `src/shim/libsystem_shim.ver`
- `docs/external-fix-workers/shim-surface.md`

Targets:

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

## Summary

Status: `RECLASSIFIED` after 2 attempts.

Worker D-owned repeated missing imports are implemented or safely stubbed:

- CoreFoundation data/array/string/date/error surface:
  `CFArrayAppendValue`, `CFArrayCreateMutable`, `CFArrayGetCount`,
  `CFArrayGetValueAtIndex`, `CFDataCreate`, `CFDataGetBytePtr`,
  `CFDataGetLength`, `CFDateCreate`, `CFErrorCopyDescription`,
  `CFErrorGetCode`, `CFStringCreateExternalRepresentation`,
  `CFStringCreateWithBytes`
- process-region surface:
  `mach_vm_region`, `proc_regionfilename`
- libproc/sysctl surface:
  `proc_listpids`, `proc_pid_rusage`, `proc_pidfdinfo`, `proc_pidinfo`,
  `sysctlnametomib`
- filesystem surface:
  `getfsstat`

No Worker D-owned `resolver: symbol ... not found` lines remain in Attempt 2
logs. All 16 external targets still exit 139 after `machismo: calling _main`.
The next blocker is no longer a missing shim import. The remaining failure is
the shared post-entry `SIGSEGV` path, plus skipped non-owned dylibs
(`Security`, `libresolv`, and for `procs`, `libiconv`) where applicable.

## Attempt 1

Status: `FAILED-1` baseline reproduction.

Command shape:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'cmake --build build-arm64 --parallel && MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-d/manifest.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-d-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-d-attempt1 MACHGATE_EXTERNAL_TIMEOUT=12 bash tests/test_external_macho_cli.sh'
```

Result:

- `0/16 external Mach-O CLI probes passed, 16 failed`
- logs: `tests/external/logs/worker-d-attempt1/`
- repeated missing symbol counts:
  - 14 each: `CFArrayAppendValue`, `CFArrayCreateMutable`,
    `CFArrayGetCount`, `CFArrayGetValueAtIndex`, `CFDataCreate`,
    `CFDataGetBytePtr`, `CFDataGetLength`, `CFDateCreate`,
    `CFErrorCopyDescription`, `CFErrorGetCode`,
    `CFStringCreateExternalRepresentation`, `CFStringCreateWithBytes`
  - 5 each: `mach_vm_region`, `proc_regionfilename`
  - 2: `getfsstat`
  - 1 each: `proc_listpids`, `proc_pid_rusage`, `proc_pidfdinfo`,
    `proc_pidinfo`, `sysctlnametomib`

Patch:

- added minimal CF array/data/string/date/error objects and accessors
- added conservative process-region stubs
- added conservative libproc/sysctl stubs
- added conservative `getfsstat` stub

Changed files:

- `src/shim/libsystem_shim.c`

## Attempt 2

Status: `RECLASSIFIED`.

Command shape:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-d/manifest.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-d-attempt2 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-d-attempt2 MACHGATE_EXTERNAL_TIMEOUT=12 bash tests/test_external_macho_cli.sh'
```

Result:

- `0/16 external Mach-O CLI probes passed, 16 failed`
- logs: `tests/external/logs/worker-d-attempt2/`
- `rg -o "resolver: symbol '[^']+' not found" tests/external/logs/worker-d-attempt2/*.err` produced no rows
- `nm -D build-arm64/libsystem_shim.so` exports every Worker D-owned symbol added in Attempt 1

Per-bucket result:

- `GO-CF-PROCESS-STUBS`: `RECLASSIFIED`
  - `gh`, `lazygit`, `glow`, `goreleaser`, `chezmoi`, `kubectl`, `helm`,
    `kind`, `argocd`, `flux`, `stern`, `terraform`, `packer`, and
    `terragrunt` no longer report missing CF/process-region shim symbols.
  - They still crash after `_main` with `SIGSEGV`.
  - Several still report failed binds from skipped `Security`/`libresolv`,
    which is outside this Worker D source scope because the harness maps those
    dylibs to `SKIP`.
- `DARWIN-LIBPROC-SYSCTL`: `RECLASSIFIED`
  - `procs` now resolves `proc_listpids`, `proc_pid_rusage`,
    `proc_pidfdinfo`, `proc_pidinfo`, and `sysctlnametomib`; resolver summary
    is `185 binds resolved, 0 stubbed, 0 failed`.
  - It still crashes after `_main`.
- `DARWIN-GETFSSTAT`: `RECLASSIFIED`
  - `duf` now resolves `getfsstat`; resolver summary is
    `62 binds resolved, 0 stubbed, 0 failed`.
  - `packer` no longer reports missing `getfsstat`, but still has skipped
    non-owned dylib binds.
  - Both still crash after `_main`.

Verification:

- build: `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'cmake --build build-arm64 --parallel'`
- fixtures: `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'bash tests/fixtures/build_fixtures.sh'`
- tests: `docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh'`
- test result: `27/27 passed, 0 failed`

Blocked/reclassified next cause:

- Exact next blocker: post-`_main` `SIGSEGV` on all 16 targets after Worker
  D-owned missing imports are resolved.
- Additional non-owned unresolved surface remains where the harness skips
  `Security`, `libresolv`, or `libiconv`; resolving those requires harness
  mapping or another owned surface decision, not changes in this Worker D source
  scope.
