# Syscall translation execution plan

## Resume checkpoint

Last updated: 2026-06-22

This file is the restart point if the agent, editor, or session crashes.

### Execution loop

Use this loop until the exit criteria below are met. Update this file after
each loop pass.

1. Pick the next batch from `docs/DARWIN_SYSCALL_TABLE_STATUS.md`.
   - Prefer rows marked `DEFERRED` that are compatible with the current target:
     single-threaded ARM64 Mach-O CLI binaries in ARM64 Linux containers.
   - Batch size should be 1-5 syscalls unless the group is mechanically
     identical, such as `*_nocancel` aliases or simple `*at` variants.
   - Do not start with rows that require a new subsystem unless all simpler
     rows are exhausted.
2. For the batch, record a loop entry in this file before editing:
   - syscall numbers and names
   - expected implementation files
   - expected fixture/test files
   - attempt count, starting at 1
3. If the remaining work can be split safely, launch 10 agents.
   - Use fewer than 10 only when fewer independent work items remain.
   - Each agent must own a disjoint syscall family and disjoint write scope.
   - Prefer splitting by subsystem, not just numeric range:
     sockets, filesystem metadata, filesystem mutation, memory mapping, time,
     process/session, SysV/POSIX IPC, signals, threading/psynch, kqueue/workq.
   - Each agent must be told that other agents are editing in parallel and that
     it must not revert or overwrite work outside its assigned files.
   - Agents may edit only their assigned source, fixture, and test files.
   - The main thread owns `docs/SYSCALL_TRANSLATION_EXECUTION_PLAN.md` and
     `docs/DARWIN_SYSCALL_TABLE_STATUS.md`.
   - The main thread integrates, resolves conflicts, runs verification, and
     updates status.
4. Implement only that batch.
   - Prefer raw Linux syscalls.
   - Translate Darwin arguments, flags, structures, return values, and errno.
   - Add or extend a real ARM64 Mach-O fixture that reaches the syscall through
     `svc #0x80`.
   - Keep unrelated loader/Machismo behavior unchanged.
5. Verify the batch.
   - Run local build:

```sh
cmake --build build --verbose -j1
```

   - Run the targeted ARM64 Docker/QEMU range test:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -w /work \
  machgate-arm64-toolchain \
  bash -lc '
    set -e
    export LLVM_MC="$(command -v llvm-mc-18 || command -v llvm-mc)"
    export LD64_LLD="$(command -v ld64.lld-18 || command -v ld64.lld)"
    export LLVM_LIPO="$(command -v llvm-lipo-18 || command -v llvm-lipo)"
    cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-arm64 --verbose -j1
    BUILD_DIR=/work/build-arm64 bash tests/fixtures/build_fixtures.sh
    BUILD_DIR=/work/build-arm64 bash tests/test_darwin_range_NNN.sh
  '
```

   - Replace `tests/test_darwin_range_NNN.sh` with the actual range test.
6. If targeted verification fails, fix and retry.
   - Maximum targeted attempts per batch: 3.
   - Record each failed attempt with the failure summary.
   - After 3 failed targeted attempts, stop working that batch, mark it
     `HARDER-LATER` in the loop log, and move on to the next batch.
7. If targeted verification passes, run the full suite:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -w /work \
  machgate-arm64-toolchain \
  bash -lc '
    set -e
    export LLVM_MC="$(command -v llvm-mc-18 || command -v llvm-mc)"
    export LD64_LLD="$(command -v ld64.lld-18 || command -v ld64.lld)"
    export LLVM_LIPO="$(command -v llvm-lipo-18 || command -v llvm-lipo)"
    cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-arm64 --verbose -j1
    BUILD_DIR=/work/build-arm64 bash tests/fixtures/build_fixtures.sh
    BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh
  '
```

8. If the full suite fails, fix and retry.
   - Maximum full-suite attempts per batch: 3.
   - Record each failed attempt with the failure summary.
   - After 3 failed full-suite attempts, revert only the current batch's
     edits if they cannot be made safe without touching unrelated work, mark
     it `HARDER-LATER`, and move on.
9. If full suite passes:
   - Mark the batch `DONE` in this file.
   - Update `docs/DARWIN_SYSCALL_TABLE_STATUS.md`.
   - Update the summary counts in this file.
   - Run `git diff --check`.
   - Remove generated fixture scratch files.
   - Start the next loop pass.

### Exit criteria

The loop is finished only when all of these are true:

- `docs/DARWIN_SYSCALL_TABLE_STATUS.md` has `TODO: 0`.
- Every source row is one of:
  - `DONE`: implemented and covered by targeted ARM64 fixture/test behavior.
  - `ENOSYS`: explicitly unsupported by production policy and covered by
    dispatcher behavior.
  - `DEFERRED`: requires a named subsystem outside the current CLI/single-thread
    target.
  - `HARDER-LATER`: attempted 3 times in this loop and left with a recorded
    failure reason.
- No batch is marked `IN-PROGRESS`.
- Local build passes.
- Targeted range tests for all changed ranges pass.
- Full ARM64 Docker/QEMU suite passes: `26/26 passed, 0 failed` or the current
  expected full-suite count if tests are added.
- `git diff --check` passes.

### Harder-later policy

Use `HARDER-LATER` only after three failed attempts in the loop. The entry must
include:

- syscall number and name
- attempted implementation approach
- exact failure or missing subsystem
- last failing command
- next recommended subsystem or design decision

Do not leave a row as `TODO` when it is moved to `HARDER-LATER`.

### Loop log

Add newest entries at the top.

- [x] Loop pass 2026-06-22-E: pending real-translation outer loop 1/10
  - Status: complete; exit condition reached with `Done: 669 / 669 source rows (100.0%)`
  - Agents: launching 10 total for this pass; platform cap is 5 active workers, so refill slots as agents complete
  - Stop condition: `Done: 669 / 669 source rows (100.0%)`
  - External loop rule: repeat up to 10 outer passes or stop early when done reaches 100%
  - Per-syscall stop rule: after 10 unit-test failures for one syscall, mark it `HARDER2` with commands, failures, sources researched, and next design needed
  - Research rule: when blocked, search Apple/XNU/libc/open-source repos or Linux libraries/designs; download context only, do not vendor
  - Current starting counts:
    - Done: 641 / 669 source rows (95.8%)
    - Translated: 427 / 669 source rows (63.8%)
    - ENOSYS: 214 / 669 source rows (32.0%)
    - Wrongly-classified: 21 / 669 source rows (3.1%)
    - Deferred: 4 / 669 source rows (0.6%)
    - Harder2: 3 / 669 source rows (0.4%)
    - Pending: 28 / 669 source rows (4.2%)
  - Agent slices:
    - [x] Agent E1 `019eee98-dc16-71b2-aedb-9b28bc47f1bd` Hypatia: filesystem hard trio: `undelete`, `exchangedata`, `searchfs`, `fhopen`
      - Result: DONE for all four; targeted `tests/test_darwin_range_200_399.sh` passed in ARM64 Docker
    - [x] Agent E2 `019eee99-11e3-7e63-affc-6ebe9aa72972` Averroes: attrlist/clone/open-by-id family: `fsgetpath`, `getattrlistbulk`, `clonefileat`, `getattrlistat`, `openbyid_np`, `fclonefileat`, `setattrlistat`
      - Result: DONE for all seven; targeted `tests/test_darwin_range_400_plus.sh` passed in ARM64 Docker
    - [x] Agent E3 `019eee99-3944-7391-a914-7bf6166c4bf4` Jason: legacy SysV multiplexers and access extension: `semsys`, `msgsys`, `shmsys`, `access_extended`
      - Result: DONE for all four; targeted `tests/test_darwin_range_200_399.sh` passed in ARM64 Docker
    - [x] Agent E4 `019eee99-6f2a-7403-8f67-14bc39cf8b11` Pauli: process spawn/MAC exec/terminate/identity: `posix_spawn`, `__mac_execve`, `terminate_with_payload`, `identitysvc`
      - Result: DONE for all four; targeted `tests/test_darwin_range_200_399.sh` and `tests/test_darwin_range_400_plus.sh` passed in ARM64 Docker
    - [x] Agent E5 `019eee9a-f4a3-7f62-a379-ccebd81c3a82` Hilbert: signal/process control: `sigsuspend_nocancel`, `pid_suspend`, `pid_resume`, `pid_hibernate`, `kas_info`
      - Result: implementation complete; isolated Mach-O probe passed; full `tests/test_darwin_range_400_plus.sh` still failing and assigned to E8
    - [x] Agent E6 `019eeea1-5c4b-7323-8dce-5b5fdb1d0f97` Godel: Darwin policy/control: `usrctl`, `proc_rlimit_control`, `bsdthread_ctl`, `kqueue_workloop_ctl`
      - Result: DONE for all four; targeted `tests/test_darwin_range_400_plus.sh` passed in ARM64 Docker
    - [x] Agent E7 `019eeea2-b813-7380-b670-bd18a8e99652` Bacon: filesystem/IPCs verifier: retest E1/E3 200-399 rows and fix regressions
      - Result: no code changes; targeted `tests/test_darwin_range_200_399.sh` passed 5/5 in ARM64 Docker
    - [x] Agent E8 `019eeea5-074e-79a2-9ec1-39d93075dce5` Carson: 400+ verifier/fixer: fix `darwin_range_400_plus` failures and remaining rows
      - Result: no code changes; targeted `tests/test_darwin_range_400_plus.sh` passed in ARM64 Docker
    - [x] Agent E9 `019eeea6-f1ae-7f62-92fc-05664f92a76e` Zeno: 100-199 verifier/fixer: fix `darwin_range_100_199` failures and remaining rows
      - Result: no code changes; targeted `tests/test_darwin_range_100_199.sh` passed 5/5 after forced fixture rebuild
    - [x] Agent E10 `019eeea8-b4e4-70d1-bc2c-c520216acfad` Kant: full-suite verifier and pending-row auditor
      - Result: no code changes; ARM64 Docker full suite passed `26/26 passed, 0 failed`; `git diff --check` passed
  - Verification required before closing loop:
    - [x] local `cmake --build build --verbose -j1`
    - [x] targeted ARM64 range tests for changed ranges
    - [x] full ARM64 Docker/QEMU suite
    - [x] `git diff --check`

- [ ] Loop pass 2026-06-22-D: real translation pass to 100 percent
  - Status: workers running
  - Agents: 5 active; 0 pending platform slots; 5 completed pending main integration
  - Stop condition: `Done: 669 / 669 source rows (100.0%)`
  - Per-syscall stop rule: after 5 unit-test failures for one syscall, mark it `HARDER2` with commands, failures, and next required design/research source
  - Research rule: if blocked, search for Apple/XNU sources, libc/libpthread behavior, or Linux libraries/designs that make a real implementation possible
  - Current starting counts:
    - Done: 499 / 669 source rows (74.6%)
    - Translated: 285 / 669 source rows (42.6%)
    - ENOSYS: 214 / 669 source rows (32.0%)
    - Wrongly-classified: 163 / 669 source rows (24.4%)
    - Deferred: 4 / 669 source rows (0.6%)
    - Harder: 3 / 669 source rows (0.4%)
    - Pending: 170 / 669 source rows (25.4%)
  - Agent slices:
    - [x] Agent D1 Darwin (`019eee5d-822c-7da3-a650-d181f492a4fd`): process/control/security wrong classifications, range 0-199; returned with process/control compatibility implementations
    - [x] Agent D2 Kepler (`019eee5d-a4a4-77c0-aef8-d4b12512ad04`): filesystem metadata/path/attr wrong classifications, range 200-399; returned with attrlist/path/copyfile/fsctl implementations and 3 HARDER2 candidates
    - [x] Agent D3 Maxwell (`019eee5d-ca26-7633-ac95-ab66cf90a07d`): pthread/psynch/workqueue runtime rows, range 200-399; returned with futex-backed psynch/workq subsets
    - [x] Agent D4 Boole (`019eee5d-f45e-72b2-a5ce-061ae94b12ed`): Mach/MAC/audit/fileport/eventlink/task rows, range 200-557; returned with compatibility state/port implementations
    - [x] Agent D5 Epicurus (`019eee5e-1711-7283-a895-aed6e6bace2c`): networking policy, NECP/Skywalk/channel/QoS rows, range 400+; returned with socket/NECP/channel compatibility implementations
    - [ ] Agent D6 Faraday (`019eee7e-2d95-7cb1-873b-a49313e9e081`): guarded fd and Darwin-only fd APIs, range 400+
    - [ ] Agent D7 Raman (`019eee7e-522d-7012-9daf-bb665cbbb880`): telemetry/coalition/persona/stackshot/CSR/SFI rows, range 400+
    - [ ] Agent D8 Aristotle (`019eee7e-7cc7-7ac0-8541-05ad756e9206`): mount/snapshot/disk-image/root mutation rows, range 0-557
    - [ ] Agent D9 Linnaeus (`019eee7e-9e44-7960-9b19-83573741f2b0`): remaining deferred/harder rows after D1-D8 inventory
    - [ ] Agent D10 Planck (`019eee7e-d7f3-7d33-b941-9467a992b06c`): verifier/integrator to run range tests and identify failing syscalls
  - Verification required before closing loop:
    - [ ] local `cmake --build build --verbose -j1`
    - [ ] targeted ARM64 range tests for changed ranges
    - [ ] full ARM64 Docker/QEMU suite
    - [ ] `git diff --check`

- [ ] Loop pass 2026-06-22-C: final deferred and harder-later reduction
  - Status: launching workers
  - Agents: 3 active; 0 pending platform slots; 7 completed pending main verification
  - Target rows: all current `DEFERRED` and `HARDER-LATER` rows in `docs/DARWIN_SYSCALL_TABLE_STATUS.md`
  - Current starting counts:
    - Done: 629 / 669 source rows (94.0%)
    - Translated: 271 / 669 source rows (40.5%)
    - ENOSYS: 358 / 669 source rows (53.5%)
    - Pending: 40 / 669 source rows (6.0%)
    - Deferred: 20 / 669 source rows (3.0%)
    - Harder: 20 / 669 source rows (3.0%)
  - Current integrated counts:
    - Done: 656 / 669 source rows (98.1%)
    - Translated: 280 / 669 source rows (41.9%)
    - ENOSYS: 376 / 669 source rows (56.2%)
    - Pending: 13 / 669 source rows (1.9%)
    - Deferred: 10 / 669 source rows (1.5%)
    - Harder: 3 / 669 source rows (0.4%)
  - Agent slices:
    - [x] Agent C1 Anscombe (`019eee4b-900b-73c1-be91-1a8db7a9983c`): process control and tracing; covered `26`, `433`, `434`, `435`, `439` as explicit ENOSYS
    - [x] Agent C2 Mencius (`019eee4b-ac99-7060-9185-ee2684ba5c41`): privileged time mutation; added safe `122` no-op subset and `140` query subset without host clock mutation
    - [x] Agent C3 Poincare (`019eee4b-d2d5-7491-bd1c-af8de7702362`): child wait boundary; implemented `173`, `416` single-process `WNOHANG`/`ECHILD` subset
    - [x] Agent C4 Pasteur (`019eee4b-f711-7ac3-99ef-c7014b387e41`): guest thread lifecycle/accounting; implemented `186`, covered `360`, `361` as explicit ENOSYS
    - [x] Agent C5 Locke (`019eee4e-855b-76b0-b698-0ef795022ea3`): signal handler/runtime boundary; improved `46` supported subset
    - [x] Agent C6 Herschel (`019eee4f-2f4f-7492-8977-6d06e2e2d2dd`): psynch rwlock rows; implemented `299`, `300`, `309`; covered `297`, `298`, `306`, `307`, `308` as explicit ENOSYS
    - [x] Agent C7 Confucius (`019eee4f-826b-7041-8099-2fb0ab2eb241`): psynch mutex/condvar rows; strengthened explicit ENOSYS fixture coverage for `301-305`, `312`; real psynch runtime remains hard
    - [ ] Agent C8 Lovelace (`019eee52-5bd7-7453-9dd0-0e222606a7e6`): semwait/workqueue boundary: `__semwait_signal`, `workq_open`, `workq_kernreturn`
    - [ ] Agent C9 Newton (`019eee54-64d6-7301-bb0c-27e479397402`): filesystem identity/path and rename flags: `fsgetpath_ext`, `fsgetpath`, `renameatx_np`, `__mac_getfsstat`
    - [ ] Agent C10: credentials/VM/SysV queue nocancel/panic: `initgroups`, `minherit`, `msgsnd_nocancel`, `msgrcv_nocancel`, `sys_panic_with_data`, `kevent64`
  - Verification required before closing loop:
    - [ ] local `cmake --build build --verbose -j1`
    - [ ] targeted ARM64 range tests for changed ranges
    - [ ] full ARM64 Docker/QEMU suite
    - [ ] `git diff --check`

- [x] Loop pass 2026-06-22-B: deferred and harder-later attack
  - Status: complete
  - Agents: 0 active; 0 pending
  - Target rows: all current `DEFERRED` and `HARDER-LATER` rows in `docs/DARWIN_SYSCALL_TABLE_STATUS.md`
  - Current starting counts:
    - Done: 523 / 669 source rows (78.2%)
    - Translated: 255 / 669 source rows (38.1%)
    - ENOSYS: 268 / 669 source rows (40.1%)
    - Pending: 146 / 669 source rows (21.8%)
  - Verified result counts:
    - Done: 629 / 669 source rows (94.0%)
    - Translated: 271 / 669 source rows (40.5%)
    - ENOSYS: 358 / 669 source rows (53.5%)
    - Pending: 40 / 669 source rows (6.0%)
  - Agent slices:
    - [x] Agent B1 Halley (`019eee27-69f3-7671-8f19-dd5484780902`): fd/control compatibility; implemented `54`, `93`, `344`; added guarded-fd ENOSYS coverage
    - [x] Agent B2 Popper (`019eee27-8fe9-7651-98f3-08f5788b7c26`): sysctl/identity/proc-info compatibility; implemented `202`, `274`, `336`, `482`, `545`; left cross-range `142`, `186` for main follow-up
    - [x] Agent B3 Fermat (`019eee27-b42c-7c93-bda1-8a3958142fa5`): socket nocancel and extended message paths; implemented `413`, `480`, `481`; covered `436`, `447-450` as explicit ENOSYS
    - [x] Agent B4 Mill (`019eee27-dad5-7b43-8f85-d54eb8e13865`): signal runtime boundary; strengthened `46`, implemented `422`, covered `111`, `184`, `410` as explicit ENOSYS
    - [x] Agent B5 Russell (`019eee27-ffb3-7f23-bf4b-21333e9d06de`): psynch and semwait boundary; added aliases/coverage for `420`, `421`, `423`; psynch rows remain hard runtime work
    - [x] Agent B6 Ramanujan (`019eee2e-94ff-7f70-9c4f-c403d68c63f0`): thread/workqueue boundary; covered `478`, `530` as explicit ENOSYS and verified existing `360`, `361`, `367`, `368` coverage
    - [x] Agent B7 Lorentz (`019eee30-6523-78e0-9715-fbdbb48caad6`): mount/filesystem policy rows; covered `85`, `159`, `164`, `165`, `167`, `424`, `425`, `518`, `526`, `537`, `549`, `555` as explicit ENOSYS
    - [x] Agent B8 Feynman (`019eee30-8a89-7901-ba8e-84bcedf08682`): Mach/eventlink/task/audit-session rows; covered `428-432`, `496-498`, `531`, `538`, `539` as explicit ENOSYS
    - [x] Agent B9 Turing (`019eee31-0c76-76c3-8c2a-36a31a4323bb`): NECP/Skywalk/channel/network policy rows; covered `460`, `501-514`, `522`, `523`, `525`, `546` as explicit ENOSYS
    - [x] Agent B10 Dirac (`019eee33-2387-7290-8cf5-73e821f29de0`): Darwin-only policy/telemetry/coalition/misc rows; covered oslog, telemetry, coalition, stackshot, persona, CSR, SFI, debug reject, map/linking rows as explicit ENOSYS
  - Verification required before closing loop:
    - [x] local `cmake --build build --verbose -j1`
    - [x] targeted ARM64 range tests for changed ranges
    - [x] full ARM64 Docker/QEMU suite: `26/26 passed, 0 failed`
    - [x] `git diff --check`

- [x] Loop pass 2026-06-22-A: subsystem translation fanout
  - Status: complete
  - Agents: 0 active; 0 pending
  - Completed agents in this pass:
    - Aquinas (`019eee09-a9fe-7f52-9f76-a41d91635b4f`): pthread / psynch / workqueue / thread runtime; implemented `332`, `333`, `366`; identified psynch/thread/workqueue hard cases
    - Pascal (`019eee09-a045-7ad3-9ba0-bbfb7ca2c45e`): kqueue / kevent subset; implemented `362`, `363`, `374`, `375` safe subset
    - Nietzsche (`019eee09-9d15-7c01-bc0a-4782dd8d9cfa`): AIO compatibility; implemented `313-320`
    - Sagan (`019eee09-9b3b-7173-b17c-e25fe3b11f75`): POSIX named semaphores / semwait; implemented `268-273` plus partial `334`
    - Dalton (`019eee09-a562-7091-b12d-ef8b583bff09`): signals / pthread signal rows; implemented `37`, `46`, `48`, `52`, `53`, `328-331`
    - Hegel (`019eee0e-fa56-74e2-98a8-19c98db6398d`): Mach/MAC/audit/security policy rows; explicit fixture-covered `ENOSYS` for audit `350`, `351`, `353`, `354`, `357`, `358`, `359` and MAC `381-390`
    - Goodall (`019eee10-186f-7910-be4e-c429a86474d8`): process/session/user/group rows; implemented or made explicit `2`, `7`, `49`, `50`, `51`, `55`, `59`, `61`, `66`, `285-290`, `311`, `400`, `520`, `521`
    - Copernicus (`019eee0f-df16-7a31-a4d9-6604a39c4da9`): filesystem metadata/mutation rows; implemented or covered `18`, `34`, `35`, `56`, `220-223`, `225`, `227-229`, `242`, `245`, `248`, `284`, `461`, `462`, `476`, `479`, `517`, `524`
    - Boyle (`019eee11-9918-7b23-9ef2-277a7c2cb653`): memory/time/resource/misc rows; implemented or improved `294`, `296`, `322`, `323`, `373`, `401-404`, `409`, `440`, `453`, `515`, `516`, `521`, `529`, `534`, `536`, `544`
    - Volta (`019eee11-96ab-7e93-b1a2-da5316ecf15b`): socket/network rows; implemented `27-32`, `97`, `98`, `401-404`, `409`
  - Goal: convert additional `DEFERRED` rows to `DONE` where feasible, or to
    `HARDER-LATER` after three verified failed attempts
  - Verification required before closing loop:
    - [x] local `cmake --build build --verbose -j1`
    - [x] targeted ARM64 range tests for changed ranges
    - [x] full ARM64 Docker/QEMU suite: `26/26 passed, 0 failed`
    - [x] `git diff --check`

### Agent fanout rule

For implementation passes, launch 10 agents whenever there are at least 10
independent syscall-family work items. The expected default split is:

- Agent 1: filesystem metadata structures and stat/statfs variants
- Agent 2: filesystem mutation/path operations
- Agent 3: socket operations and socket option translation
- Agent 4: memory mapping, memory advice, and page residency
- Agent 5: time, timers, clocks, and resource limits
- Agent 6: process/session/user/group queries and safe setters
- Agent 7: SysV IPC and POSIX IPC
- Agent 8: signals and signal masks
- Agent 9: pthread/psynch/thread/workqueue support
- Agent 10: kqueue/kevent/audit/MAC-policy or explicit hardening policy

If an agent hits three failed verification attempts for its assigned slice, the
main thread marks that slice `HARDER-LATER` with the required failure details
and continues integrating the remaining agents.

### Live progress board

Full-table correction:

- [x] Created visible full-table tracker: `docs/DARWIN_SYSCALL_TABLE_STATUS.md`
- [ ] Full XNU syscall table coverage
  - Status: in progress
  - Scope: all 669 rows parsed from Apple `bsd/kern/syscalls.master`
  - Current full-source-row baseline from `docs/DARWIN_SYSCALL_TABLE_STATUS.md`:
    - Done: 629 / 669 source rows (94.0%)
    - Translated: 271 / 669 source rows (40.5%)
    - ENOSYS: 358 / 669 source rows (53.5%)
    - Pending: 40 / 669 source rows (6.0%)
  - All source rows are classified; no TODO rows remain.
  - Range dispatcher source files are integrated into `machismo`
  - Syscall implementation files live under `src/syscall/`
  - Current range status:
    - [x] Peirce: syscall numbers 0-99, integrated under `src/syscall/`, targeted range test passed
    - [x] Singer: syscall numbers 100-199, integrated under `src/syscall/`, targeted range test passed
    - [x] Bernoulli: syscall numbers 200-399, integrated under `src/syscall/`, targeted range test passed
    - [x] Mendel: syscall numbers 400+, integrated under `src/syscall/`, targeted range test passed
  - Important: this is full-table accounting and dispatch coverage, not full functional emulation for every active Darwin syscall.
  - Full ARM64 Docker/QEMU shell suite passed after range integration and `src/syscall/` move: `26/26 passed, 0 failed`.
  - Full ARM64 Docker/QEMU shell suite passed after worker translation batch: `26/26 passed, 0 failed`.
  - Full ARM64 Docker/QEMU shell suite passed after second-wave translation batch: `26/26 passed, 0 failed`.
  - Full ARM64 Docker/QEMU shell suite passed after 10-slice loop pass 2026-06-22-A: `26/26 passed, 0 failed`.
  - Full ARM64 Docker/QEMU shell suite passed after deferred/harder-later loop pass 2026-06-22-B: `26/26 passed, 0 failed`.
  - Fast verification setup: use Docker image `machgate-arm64-toolchain` and mounted workspace `/work`; keep ARM64 build artifacts in mounted `build-arm64`.
  - Active implementation workers:
    - [x] Kuhn (`019eedca-24c0-7bd3-9119-8ccb5f1c180c`): range 0-99, added `madvise`, `mincore`, `setitimer`, `getitimer`, `setpriority`, and fixed `dup2(fd, fd)` semantics
    - [x] Ampere (`019eedca-26a5-7c72-b8ba-121cc0941f37`): range 100-199, added `setreuid`, `setregid`, `utimes`, `futimes`, `statfs`, `fstatfs`, `pathconf`, `fpathconf`, `getdirentries`
    - [x] Euler (`019eedca-2ad5-7890-9562-21f5ea951dee`): range 200-399, added real translations for `216`, `218`, `226`, `234-241`, `277-283`, `286`, `291-292`, `341-343`, `394-395`
    - [x] Sartre (`019eedca-2e16-7560-b264-aa0c85cce18b`): range 400-557+, added `fcntl_nocancel`, `select_nocancel`, `ntp_adjtime`, `ntp_gettime`, `freadlink`
  - Active second-wave workers:
    - [x] Leibniz (`019eedd3-be03-7893-83a9-ae200a4c5f6d`): remaining TODO rows in range 100-199, added `bind`, `setsockopt`, `listen`, `getsockopt`, `sendto`, `shutdown`, `socketpair`
    - [x] Lagrange (`019eedd3-bfa8-7c51-a23f-6d2fbf9b5318`): remaining TODO rows in range 200-399, added SysV IPC, POSIX shm, `sendfile`, `statfs64`, `fstatfs64`, `getfsstat64`

- [x] Group 0: syscall gate foundation, `exit`, `write`
  - Status: passed targeted ARM64 Docker/QEMU tests
  - Evidence: `tests/test_darwin_exit42.sh`, `tests/test_darwin_write_stdout.sh`
- [x] Group 1: `read`, `close`
  - Status: passed targeted ARM64 Docker/QEMU test
  - Evidence: `GROUP1_OK` from `tests/test_darwin_read_close.sh`
- [x] Group 2: `open`, `lseek`
  - Status: passed targeted ARM64 Docker/QEMU test after fixture path fix
  - Evidence: `GROUP2_OK` from `tests/test_darwin_open_lseek.sh`
- [x] Group 3: `fstat`
  - Status: passed targeted ARM64 Docker/QEMU test
  - Owner: sub-agent Parfit checks fixture/test; main thread integrates dispatcher fixes
  - Evidence: `GROUP3_OK` from `tests/test_darwin_fstat.sh`
- [x] Group 4: `mmap`, `mprotect`, `munmap`
  - Status: passed targeted ARM64 Docker/QEMU test
  - Owner: sub-agent Laplace checks fixture/test; main thread integrates dispatcher fixes
  - Evidence: `GROUP4_OK` from `tests/test_darwin_mmap_protect.sh`
- [x] Final full-suite verification
  - Status: passed ARM64 Docker/QEMU full suite
  - Evidence: verification agent reported `22/22 passed, 0 failed` using `/tmp/machgate-build-arm64`
  - Owner: main thread, with verification hygiene notes from sub-agent Harvey

### Current status

Completed and verified:

- syscall gate foundation
- Darwin `exit`
- Darwin `write`
- Darwin `read`
- Darwin `close`
- Darwin `open`
- Darwin `lseek`
- Darwin `fstat`
- Darwin `mmap`
- Darwin `munmap`
- Darwin `mprotect`
- real ARM64 Mach-O fixtures for `exit` and `write`
- targeted ARM64 Docker/QEMU tests for `exit` and `write`
- targeted ARM64 Docker/QEMU tests for `read` and `close`
- targeted ARM64 Docker/QEMU tests for `open` and `lseek`
- targeted ARM64 Docker/QEMU tests for `fstat`
- targeted ARM64 Docker/QEMU tests for `mmap`, `munmap`, and `mprotect`
- full ARM64 Docker/QEMU shell suite passed once with `18/18`
- full ARM64 Docker/QEMU shell suite passed after initial table completion with `22/22`

Partially edited but not yet verified:

- None for the initial table.

Not started:

- next functional syscall implementation batch beyond representative range probes

In progress:

- functional syscall translation beyond representative range probes

### Source files already created

Keep these files:

- `src/syscall/syscall_gate.c`
- `src/syscall/syscall_gate.h`
- `tests/fixtures/darwin_exit42.s`
- `tests/fixtures/darwin_write_stdout.s`
- `tests/test_darwin_exit42.sh`
- `tests/test_darwin_write_stdout.sh`
- `docs/SYSCALL_TRANSLATION_EXECUTION_PLAN.md`

Important existing edits:

- `CMakeLists.txt` includes `src/syscall/syscall_gate.c`
- `src/machismo.c` calls `syscall_gate_patch()` before guest entry
- `tests/fixtures/build_fixtures.sh` resolves versioned LLVM tools such as
  `llvm-mc-18`, `ld64.lld-18`, and `llvm-lipo-18`
- `.gitignore` ignores generated Darwin fixture binaries and `build-arm64/`
- `src/trampoline.c` contains an earlier unrelated `ucontext_t` portability fix

### Exact next step

Resume from the execution loop.

1. Pick the next eligible `DEFERRED` batch from `docs/DARWIN_SYSCALL_TABLE_STATUS.md`.
2. Add a loop-log entry with attempt `1`.
3. Implement, verify, retry up to three times, then mark `DONE` or `HARDER-LATER`.

Previously completed Group 1/2 steps:

1. Added `tests/fixtures/darwin_read_close.s`.
2. Added `tests/test_darwin_read_close.sh`.
3. Extended `tests/fixtures/build_fixtures.sh` to build `darwin_read_close`.
4. Added `tests/fixtures/darwin_open_lseek.s`.
5. Added `tests/test_darwin_open_lseek.sh`.
6. Extended `tests/fixtures/build_fixtures.sh` to build `darwin_open_lseek`.

Build locally with:

```sh
cmake --build build --verbose -j1
```

Run targeted ARM64 Docker/QEMU verification for the current group before moving
on.

Historical checkpoint for Group 1:

```text
GROUP1_OK
```

Historical checkpoint for Group 2:

```text
GROUP2_OK
```

Old Group 1 resume checklist retained for audit:

1. Add `tests/fixtures/darwin_read_close.s`. Done.
2. Add `tests/test_darwin_read_close.sh`. Done.
3. Extend `tests/fixtures/build_fixtures.sh` to build `darwin_read_close`. Done.
4. Build locally with:

```sh
cmake --build build --verbose -j1
```c

5. Run targeted ARM64 Docker/QEMU verification for only:

```sh
tests/test_darwin_exit42.sh
tests/test_darwin_write_stdout.sh
tests/test_darwin_read_close.sh
```

6. Fix Group 1 before starting Group 2.

### ARM64 Docker/QEMU verification command template

Use this host command for targeted verification. Adjust the test list per group.

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -w /work \
  ubuntu:24.04 \
  bash -lc '
    set -e
    export DEBIAN_FRONTEND=noninteractive
    apt-get update >/dev/null
    apt-get install -y build-essential cmake ninja-build clang lld llvm python3 zlib1g-dev >/dev/null
    export LLVM_MC="$(command -v llvm-mc-18 || command -v llvm-mc)"
    export LD64_LLD="$(command -v ld64.lld-18 || command -v ld64.lld)"
    export LLVM_LIPO="$(command -v llvm-lipo-18 || command -v llvm-lipo)"
    cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug >/tmp/cmake.log
    cmake --build build-arm64 --verbose -j1 >/tmp/build.log
    BUILD_DIR=/work/build-arm64 bash tests/fixtures/build_fixtures.sh
    BUILD_DIR=/work/build-arm64 bash tests/test_darwin_exit42.sh
    BUILD_DIR=/work/build-arm64 bash tests/test_darwin_write_stdout.sh
  '
```

### Apple context files to download when resuming

Download these into `/tmp` for context only:

```sh
curl -L --silent https://raw.githubusercontent.com/apple-oss-distributions/xnu/main/bsd/kern/syscalls.master -o /tmp/machgate-xnu-syscalls.master
curl -L --silent https://raw.githubusercontent.com/apple-oss-distributions/xnu/main/bsd/sys/errno.h -o /tmp/machgate-xnu-errno.h
curl -L --silent https://raw.githubusercontent.com/apple-oss-distributions/xnu/main/bsd/sys/fcntl.h -o /tmp/machgate-xnu-fcntl.h
curl -L --silent https://raw.githubusercontent.com/apple-oss-distributions/xnu/main/bsd/sys/mman.h -o /tmp/machgate-xnu-mman.h
curl -L --silent https://raw.githubusercontent.com/apple-oss-distributions/xnu/main/bsd/sys/stat.h -o /tmp/machgate-xnu-stat.h
curl -L --silent https://raw.githubusercontent.com/apple-oss-distributions/xnu/main/libsyscall/custom/SYS.h -o /tmp/machgate-xnu-SYS.h
```

Known syscall numbers from Apple `syscalls.master`:

- `exit = 1`
- `read = 3`
- `write = 4`
- `open = 5`
- `close = 6`
- `munmap = 73`
- `mprotect = 74`
- `fstat = 189`
- `mmap = 197`
- `lseek = 199`

## Scope clarification

This plan does not translate the entire Darwin syscall table.

The phrase "all syscalls" in this plan means all syscalls in the current
initial translation target:

- `exit`
- `write`
- `read`
- `open`
- `close`
- `lseek`
- `fstat`
- `mmap`
- `munmap`
- `mprotect`

Do not treat this as a request to implement every syscall in XNU
`syscalls.master`.

The full Darwin syscall table includes many syscall families that are
explicitly out of scope for this phase, including:

- signals
- threads
- kqueue
- sockets
- fork
- execve
- posix_spawn
- Mach messages
- Mach ports

Those remain unsupported and must return Darwin `ENOSYS` through the syscall
gate until a later plan adds them with dedicated fixtures and tests.

## Objective

Translate the initial Darwin ARM64 syscall set incrementally, using real
ARM64 Mach-O fixtures for verification at each step.

Do not translate a syscall and move on without a passing fixture that exercises
the raw Darwin syscall ABI:

- syscall number in `x16`
- arguments in `x0`, `x1`, `x2`, and so on
- `svc #0x80`
- carry clear on success
- carry set on failure

At the end, run the full test suite and fix any broken tests.

## Current completed base

Already implemented and tested:

- `exit`
- `write`

Current syscall gate behavior:

- scans executable Mach-O instruction sections
- patches `svc #0x80`
- branches to an ARM64 island
- calls a C dispatcher
- restores guest state
- resumes after the original syscall instruction
- returns Darwin `ENOSYS` for unsupported syscalls

## Workflow for every group

For each group below:

1. Download/read the exact Apple source files needed for the group into `/tmp`
   for context only.
2. Add real ARM64 Mach-O assembly fixtures.
3. Add shell tests that assert observable behavior.
4. Implement only the syscalls in that group.
5. Build locally.
6. Build fixtures.
7. Run the targeted tests in the ARM64 Docker/QEMU environment.
8. Fix failures before starting the next group.

Do not vendor Apple source into this repo unless a small specific file is
explicitly needed later.

## Group 1: descriptor basics

Status: passed targeted ARM64 Docker/QEMU test.

Implement:

- `read`
- `close`

Fixture:

- read fixed bytes from stdin into guest memory
- write those bytes back to stdout using already-working `write`
- close a descriptor
- exit zero only if all operations succeed

Tests:

- pipe known stdin into the Mach-O fixture
- assert stdout matches expected bytes
- assert exit status is zero

Apple references:

- `apple-oss-distributions/xnu/bsd/kern/syscalls.master`
- `apple-oss-distributions/xnu/bsd/sys/errno.h`
- `apple-oss-distributions/xnu/libsyscall/custom/SYS.h`

## Group 2: file open and seek

Status: passed targeted ARM64 Docker/QEMU test after fixture path fix.

Implement:

- `open`
- `lseek`

Fixture:

- open a known test data file
- seek to a fixed offset
- read bytes from that offset
- write those bytes to stdout
- close the descriptor
- exit zero only if output path succeeds

Tests:

- create or use a deterministic test data file
- run the Mach-O fixture from the repo root
- assert stdout matches the expected slice
- assert exit status is zero

Apple references:

- `apple-oss-distributions/xnu/bsd/kern/syscalls.master`
- `apple-oss-distributions/xnu/bsd/sys/fcntl.h`
- `apple-oss-distributions/xnu/bsd/sys/errno.h`

## Group 3: file metadata

Status: passed targeted ARM64 Docker/QEMU test.

Implement:

- `fstat`

Fixture:

- open a known test data file
- call `fstat`
- inspect Darwin-layout `struct stat` in guest memory
- verify at least `st_size`
- exit zero only if the expected field values match

Tests:

- create or use a deterministic test data file
- run the Mach-O fixture
- assert zero exit status

Apple references:

- `apple-oss-distributions/xnu/bsd/kern/syscalls.master`
- `apple-oss-distributions/xnu/bsd/sys/stat.h`
- `apple-oss-distributions/xnu/bsd/sys/errno.h`

Implementation note:

- reuse or extract the Darwin `struct stat` conversion logic already present in
  `src/shim/libsystem_shim.c`

## Group 4: memory mapping

Status: passed targeted ARM64 Docker/QEMU test.

Implement:

- `mmap`
- `munmap`
- `mprotect`

Fixture:

- map anonymous writable memory
- write bytes into it
- protect it read-only
- write those bytes to stdout
- unmap it
- exit zero only if all operations succeed

Tests:

- run the Mach-O fixture
- assert stdout matches expected bytes
- assert exit status is zero

Apple references:

- `apple-oss-distributions/xnu/bsd/kern/syscalls.master`
- `apple-oss-distributions/xnu/bsd/sys/mman.h`
- `apple-oss-distributions/xnu/bsd/sys/errno.h`

## Initial table completion target

The initial syscall translation table is complete only when all of these are
implemented and covered by real Mach-O fixtures:

- `exit`
- `write`
- `read`
- `open`
- `close`
- `lseek`
- `fstat`
- `mmap`
- `munmap`
- `mprotect`

Unsupported syscalls must still:

- log the Darwin syscall number
- log guest PC
- log argument registers
- return Darwin `ENOSYS`

## Final verification

After all groups pass targeted verification:

1. Run the full ARM64 Docker/QEMU test suite. Done.
2. Fix every broken test. Done.
3. Rerun the full suite until it passes. Done.
4. Run a local build check. Done before full suite; rerun if source changes again.

Final full-suite result:

```text
22/22 passed, 0 failed
```

Expected commands:

```sh
cmake --build build --verbose -j1
bash tests/fixtures/build_fixtures.sh
bash tests/run_tests.sh
```

For this x86_64 host, the full runtime verification must use an ARM64 Linux
container under Docker/QEMU.

## Deferred work

Do not start these until the initial table completion target passes:

- signals
- threads
- kqueue
- sockets
- fork
- execve
- posix_spawn
- Mach messages
- Mach ports

## Full Darwin syscall table test strategy

This section is a design note for future full-table coverage. It is not an
implementation request and does not expand the current translation scope.

The goal is to cover large syscall groups mechanically with real ARM64 Mach-O
fixtures without hand-writing one assembly file per syscall.

### Fixture generator

Add a generator such as `tests/fixtures/generate_syscall_probes.py` that reads a
checked-in manifest, for example `tests/fixtures/darwin_syscall_probes.yaml`.
The generator emits one or more ARM64 assembly fixtures under a generated
fixture directory.

Each manifest row should describe:

- Darwin syscall number and name from `syscalls.master`
- syscall family such as filesystem, memory, process, signal, socket, thread,
  Mach, or unsupported
- expected status: `translated`, `enosys`, `host_skip`, or `dangerous_skip`
- argument recipe for a deterministic probe case
- expected result class, not just one exact raw value
- expected Darwin errno on failure
- required host setup files, directories, or environment variables
- whether the probe is safe to run in Docker/QEMU

The generated assembly should use the real Darwin ABI for every call:

- x16 contains the Darwin syscall number
- x0 through x8 contain arguments
- `svc #0x80` enters the syscall gate
- success is carry clear
- failure is carry set with Darwin errno in x0

The fixture should not import libSystem symbols. The generated Mach-O must stay
static enough to prove syscall interception is independent of symbol
resolution.

### Table-driven probe harness

Use a small generated assembly or C-compatible runtime harness shared by all
probe fixtures. The harness should:

- run an array of probe descriptors
- load the syscall number and argument registers from each descriptor
- issue `svc #0x80`
- capture x0, x1, and NZCV immediately after return
- compare carry state, return class, and errno against the descriptor
- write compact failure records to stdout or stderr using already-supported
  Darwin `write`
- exit zero only when every selected probe matches

For syscalls with pointer arguments, the descriptor should point at generated
guest memory blocks for paths, buffers, `iovec` arrays, `stat` buffers,
`timeval` buffers, or intentionally invalid addresses. This keeps most probes
data-driven while still using real guest pointers.

Prefer one fixture per syscall family rather than one fixture per syscall:

- `darwin_probe_fs` for open/read/write/close/lseek/stat-style calls
- `darwin_probe_memory` for mmap/mprotect/munmap-style calls
- `darwin_probe_errno` for invalid-argument and bad-file-descriptor cases
- `darwin_probe_unsupported` for intentionally unsupported calls
- later dedicated fixtures only for families with stateful side effects, such as
  process, signal, socket, thread, or Mach traps

Hand-written fixtures remain appropriate for the first syscall in a new
behavior class, for regression tests that need exact instruction layout, or for
dangerous syscalls where generation would hide important setup.

### Expected errno behavior

The manifest should express expected failures in Darwin terms. Tests should
compare the guest-visible errno value against Darwin errno numbers, not Linux
errno numbers.

Use negative probes heavily because they are deterministic and safe:

- invalid file descriptor should produce Darwin `EBADF`
- invalid pointer should produce Darwin `EFAULT` when the translated syscall
  promises that behavior
- missing path should produce Darwin `ENOENT`
- unsupported syscall should produce Darwin `ENOSYS`
- invalid flags should produce the Darwin errno selected by the translation
  contract for that syscall

When Darwin and Linux differ, record the chosen behavior in the manifest. The
test should enforce the MachGate contract, not the host kernel's raw result.

### Unsupported but production-grade syscalls

Unsupported syscalls should be tested deliberately, not ignored. A syscall is
production-grade unsupported when the gate:

- recognizes that the Darwin number is not implemented
- logs the Darwin syscall number, guest PC, and argument registers
- returns to guest code without corrupting preserved registers
- reports failure using Darwin syscall ABI conventions
- returns Darwin `ENOSYS`

The `darwin_probe_unsupported` fixture should sample every deferred family:

- signals
- threads
- kqueue
- sockets
- fork and exec-style calls
- posix_spawn
- Mach ports and Mach messages

Do not require semantic emulation for those rows until a later milestone marks
the syscall as `translated`. Changing a row from `enosys` to `translated` should
require adding positive and negative probe cases for that syscall in the same
change.

Some syscalls should remain `dangerous_skip` in normal automation even after
they appear in the manifest, for example host-reboot-like operations, process
replacement, or calls that cannot be safely sandboxed. A skipped row still needs
a documented reason and a manual verification plan.

### Docker/QEMU verification

Full-table probe tests must run under ARM64 Linux, even on x86_64 developer
hosts. Use the existing Docker/QEMU pattern:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work \
  -w /work \
  ubuntu:24.04 \
  bash -lc '
    set -e
    export DEBIAN_FRONTEND=noninteractive
    apt-get update >/dev/null
    apt-get install -y build-essential cmake ninja-build clang lld llvm python3 zlib1g-dev >/dev/null
    cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-arm64 --verbose -j1
    BUILD_DIR=/work/build-arm64 bash tests/fixtures/build_fixtures.sh
    BUILD_DIR=/work/build-arm64 bash tests/test_darwin_syscall_probes.sh
  '
```

The shell test should:

- generate fixtures from the manifest
- build them with the same LLVM Mach-O toolchain used by existing fixtures
- create deterministic host-side files and directories
- run only Docker-safe probes by default
- fail on any unexpected stdout, stderr, exit status, errno, or missing log
  record
- offer an opt-in environment variable for slow or host-mutating probes

### Mechanical coverage reports

Add a report mode to the generator that prints coverage by syscall number and
family:

- implemented and positively tested
- implemented with negative-only coverage
- intentionally unsupported and ENOSYS-tested
- skipped with reason
- missing from the manifest

The full-table work should not be considered complete while any syscall from
the selected XNU `syscalls.master` snapshot is missing from the manifest. The
manifest snapshot version should be checked in so that table drift is explicit.

## Planning appendix: full Darwin syscall inventory

Source: `/tmp/machgate-xnu-syscalls.master`, parsed as 558 unique syscall numbers (`0`-`557`). Conditional `#if/#else` duplicate numbers are collapsed by preferring the real syscall name over a `nosys`/`enosys` fallback. This is a production planning inventory, not an implementation request.

Translation-plan rollup:

- Already implemented: 10
- Mechanical Linux syscall mappings: 44
- Linux mappings with Darwin path/flags/struct conversion: 109
- Requires semantic emulation before any Linux mapping: 196
- Unsupported/no-op/nosys/obsolete for this target: 199

| # | syscall | category | mapping plan | notes |
|---:|---|---|---|---|
| 0 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | indirect syscall |
| 1 | `exit` | already implemented | implemented: exit_group direct exit |  |
| 2 | `fork` | process | requires semantic emulation before mapping |  |
| 3 | `read` | already implemented | implemented: SYS_read direct |  |
| 4 | `write` | already implemented | implemented: SYS_write direct |  |
| 5 | `open` | already implemented | implemented: openat plus Darwin flag conversion |  |
| 6 | `sys_close` | already implemented | implemented: SYS_close direct |  |
| 7 | `wait4` | process | requires semantic emulation before mapping |  |
| 8 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old creat |
| 9 | `link` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 10 | `unlink` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 11 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old execv |
| 12 | `sys_chdir` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 13 | `sys_fchdir` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 14 | `mknod` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 15 | `chmod` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 16 | `chown` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 17 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old break |
| 18 | `getfsstat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 19 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old lseek |
| 20 | `getpid` | process | requires semantic emulation before mapping |  |
| 21 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old mount |
| 22 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old umount |
| 23 | `setuid` | process | requires semantic emulation before mapping |  |
| 24 | `getuid` | process | requires semantic emulation before mapping |  |
| 25 | `geteuid` | process | requires semantic emulation before mapping |  |
| 26 | `ptrace` | process | requires semantic emulation before mapping |  |
| 27 | `recvmsg` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 28 | `sendmsg` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 29 | `recvfrom` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 30 | `accept` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 31 | `getpeername` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 32 | `getsockname` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 33 | `access` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 34 | `chflags` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 35 | `fchflags` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 36 | `sync` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 37 | `kill` | signal | requires semantic emulation before mapping |  |
| 38 | `sys_crossarch_trap` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 39 | `getppid` | process | requires semantic emulation before mapping |  |
| 40 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old lstat |
| 41 | `sys_dup` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 42 | `pipe` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 43 | `getegid` | process | requires semantic emulation before mapping |  |
| 44 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old profil |
| 45 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ktrace |
| 46 | `sigaction` | signal | requires semantic emulation before mapping |  |
| 47 | `getgid` | process | requires semantic emulation before mapping |  |
| 48 | `sigprocmask` | signal | requires semantic emulation before mapping |  |
| 49 | `getlogin` | process | requires semantic emulation before mapping |  |
| 50 | `setlogin` | process | requires semantic emulation before mapping |  |
| 51 | `acct` | process | requires semantic emulation before mapping |  |
| 52 | `sigpending` | signal | requires semantic emulation before mapping |  |
| 53 | `sigaltstack` | signal | requires semantic emulation before mapping |  |
| 54 | `ioctl` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 55 | `reboot` | process | requires semantic emulation before mapping |  |
| 56 | `revoke` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 57 | `symlink` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 58 | `readlink` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 59 | `execve` | process | requires semantic emulation before mapping |  |
| 60 | `umask` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 61 | `chroot` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 62 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old fstat |
| 63 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | used internally and reserved |
| 64 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getpagesize |
| 65 | `msync` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 66 | `vfork` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 67 | `oslog_coproc_reg` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 68 | `oslog_coproc` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 69 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sbrk |
| 70 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sstk |
| 71 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old mmap |
| 72 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old vadvise |
| 73 | `munmap` | already implemented | implemented: SYS_munmap direct |  |
| 74 | `mprotect` | already implemented | implemented: SYS_mprotect direct |  |
| 75 | `madvise` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 76 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old vhangup |
| 77 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old vlimit |
| 78 | `mincore` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 79 | `getgroups` | process | requires semantic emulation before mapping |  |
| 80 | `setgroups` | process | requires semantic emulation before mapping |  |
| 81 | `getpgrp` | process | requires semantic emulation before mapping |  |
| 82 | `setpgid` | process | requires semantic emulation before mapping |  |
| 83 | `setitimer` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 84 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old wait |
| 85 | `swapon` | process | requires semantic emulation before mapping |  |
| 86 | `getitimer` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 87 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old gethostname |
| 88 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sethostname |
| 89 | `sys_getdtablesize` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 90 | `sys_dup2` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 91 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getdopt |
| 92 | `sys_fcntl` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 93 | `select` | kqueue/event | requires semantic emulation before mapping |  |
| 94 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old setdopt |
| 95 | `fsync` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 96 | `setpriority` | process | requires semantic emulation before mapping |  |
| 97 | `socket` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 98 | `connect` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 99 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old accept |
| 100 | `getpriority` | process | requires semantic emulation before mapping |  |
| 101 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old send |
| 102 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old recv |
| 103 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sigreturn |
| 104 | `bind` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 105 | `setsockopt` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 106 | `listen` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 107 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old vtimes |
| 108 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sigvec |
| 109 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sigblock |
| 110 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sigsetmask |
| 111 | `sigsuspend` | signal | requires semantic emulation before mapping |  |
| 112 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sigstack |
| 113 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old recvmsg |
| 114 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sendmsg |
| 115 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old vtrace |
| 116 | `gettimeofday` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 117 | `getrusage` | process | requires semantic emulation before mapping |  |
| 118 | `getsockopt` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 119 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old resuba |
| 120 | `readv` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 121 | `writev` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 122 | `settimeofday` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 123 | `fchown` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 124 | `fchmod` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 125 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old recvfrom |
| 126 | `setreuid` | process | requires semantic emulation before mapping |  |
| 127 | `setregid` | process | requires semantic emulation before mapping |  |
| 128 | `rename` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 129 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old truncate |
| 130 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ftruncate |
| 131 | `sys_flock` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 132 | `mkfifo` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 133 | `sendto` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 134 | `shutdown` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 135 | `socketpair` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 136 | `mkdir` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 137 | `rmdir` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 138 | `utimes` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 139 | `futimes` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 140 | `adjtime` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 141 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getpeername |
| 142 | `gethostuuid` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 143 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sethostid |
| 144 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getrlimit |
| 145 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old setrlimit |
| 146 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old killpg |
| 147 | `setsid` | process | requires semantic emulation before mapping |  |
| 148 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old setquota |
| 149 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old qquota |
| 150 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getsockname |
| 151 | `getpgid` | process | requires semantic emulation before mapping |  |
| 152 | `setprivexec` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 153 | `pread` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 154 | `pwrite` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 155 | `nfssvc` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion | conditional alt: nosys |
| 156 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getdirentries |
| 157 | `statfs` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 158 | `fstatfs` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 159 | `unmount` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 160 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old async_daemon |
| 161 | `getfh` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion | conditional alt: nosys |
| 162 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getdomainname |
| 163 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old setdomainname |
| 164 | `funmount` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 165 | `quotactl` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 166 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old exportfs |
| 167 | `mount` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 168 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ustat |
| 169 | `csops` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 170 | `csops_audittoken` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 171 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old wait3 |
| 172 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old rpause |
| 173 | `waitid` | process | requires semantic emulation before mapping |  |
| 174 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getdents |
| 175 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old gc_control |
| 176 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old add_profil |
| 177 | `kdebug_typefilter` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 178 | `kdebug_trace_string` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 179 | `kdebug_trace64` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 180 | `kdebug_trace` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 181 | `setgid` | process | requires semantic emulation before mapping |  |
| 182 | `setegid` | process | requires semantic emulation before mapping |  |
| 183 | `seteuid` | process | requires semantic emulation before mapping |  |
| 184 | `sigreturn` | signal | requires semantic emulation before mapping |  |
| 185 | `sys_panic_with_data` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 186 | `thread_selfcounts` | pthread/thread | requires semantic emulation before mapping |  |
| 187 | `fdatasync` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 188 | `stat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 189 | `sys_fstat` | already implemented | implemented: SYS_fstat plus Darwin struct stat conversion |  |
| 190 | `lstat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 191 | `pathconf` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 192 | `sys_fpathconf` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 193 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getfsstat |
| 194 | `getrlimit` | process | requires semantic emulation before mapping |  |
| 195 | `setrlimit` | process | requires semantic emulation before mapping |  |
| 196 | `getdirentries` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 197 | `mmap` | already implemented | implemented: SYS_mmap plus Darwin flag conversion |  |
| 198 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old __syscall |
| 199 | `lseek` | already implemented | implemented: SYS_lseek direct |  |
| 200 | `truncate` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 201 | `ftruncate` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 202 | `sysctl` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 203 | `mlock` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 204 | `munlock` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 205 | `undelete` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 206 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ATsocket |
| 207 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ATgetmsg |
| 208 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ATputmsg |
| 209 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ATsndreq |
| 210 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ATsndrsp |
| 211 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ATgetreq |
| 212 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old ATgetrsp |
| 213 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | Reserved for AppleTalk |
| 214 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 215 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 216 | `open_dprotected_np` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 217 | `fsgetpath_ext` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 218 | `openat_dprotected_np` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 219 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old fstatv |
| 220 | `getattrlist` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 221 | `setattrlist` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 222 | `getdirentriesattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 223 | `exchangedata` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 224 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old checkuseraccess or fsgetpath |
| 225 | `searchfs` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 226 | `delete` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion | private delete (Carbon semantics) |
| 227 | `copyfile` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 228 | `fgetattrlist` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 229 | `fsetattrlist` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 230 | `poll` | kqueue/event | requires semantic emulation before mapping |  |
| 231 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old watchevent |
| 232 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old waitevent |
| 233 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old modwatch |
| 234 | `getxattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 235 | `fgetxattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 236 | `setxattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 237 | `fsetxattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 238 | `removexattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 239 | `fremovexattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 240 | `listxattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 241 | `flistxattr` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 242 | `fsctl` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 243 | `initgroups` | process | requires semantic emulation before mapping |  |
| 244 | `posix_spawn` | process | requires semantic emulation before mapping |  |
| 245 | `ffsctl` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 246 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 247 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old nfsclnt |
| 248 | `fhopen` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion | conditional alt: nosys |
| 249 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 250 | `minherit` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 251 | `semsys` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 252 | `msgsys` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 253 | `shmsys` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 254 | `semctl` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 255 | `semget` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 256 | `semop` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 257 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old semconfig |
| 258 | `msgctl` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 259 | `msgget` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 260 | `msgsnd` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 261 | `msgrcv` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 262 | `shmat` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 263 | `shmctl` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 264 | `shmdt` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 265 | `shmget` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 266 | `shm_open` | pthread/thread | requires semantic emulation before mapping |  |
| 267 | `shm_unlink` | pthread/thread | requires semantic emulation before mapping |  |
| 268 | `sem_open` | pthread/thread | requires semantic emulation before mapping |  |
| 269 | `sem_close` | pthread/thread | requires semantic emulation before mapping |  |
| 270 | `sem_unlink` | pthread/thread | requires semantic emulation before mapping |  |
| 271 | `sem_wait` | pthread/thread | requires semantic emulation before mapping |  |
| 272 | `sem_trywait` | pthread/thread | requires semantic emulation before mapping |  |
| 273 | `sem_post` | pthread/thread | requires semantic emulation before mapping |  |
| 274 | `sys_sysctlbyname` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 275 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sem_init |
| 276 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old sem_destroy |
| 277 | `open_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 278 | `umask_extended` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 279 | `stat_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 280 | `lstat_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 281 | `sys_fstat_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 282 | `chmod_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 283 | `fchmod_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 284 | `access_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 285 | `sys_settid` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 286 | `gettid` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 287 | `setsgroups` | process | requires semantic emulation before mapping |  |
| 288 | `getsgroups` | process | requires semantic emulation before mapping |  |
| 289 | `setwgroups` | process | requires semantic emulation before mapping |  |
| 290 | `getwgroups` | process | requires semantic emulation before mapping |  |
| 291 | `mkfifo_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 292 | `mkdir_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 293 | `identitysvc` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 294 | `shared_region_check_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 295 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old shared_region_map_np |
| 296 | `vm_pressure_monitor` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 297 | `psynch_rw_longrdlock` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 298 | `psynch_rw_yieldwrlock` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 299 | `psynch_rw_downgrade` | pthread/thread | requires semantic emulation before mapping | conditional alt: enosys |
| 300 | `psynch_rw_upgrade` | pthread/thread | requires semantic emulation before mapping | conditional alt: enosys |
| 301 | `psynch_mutexwait` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 302 | `psynch_mutexdrop` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 303 | `psynch_cvbroad` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 304 | `psynch_cvsignal` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 305 | `psynch_cvwait` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 306 | `psynch_rw_rdlock` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 307 | `psynch_rw_wrlock` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 308 | `psynch_rw_unlock` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 309 | `psynch_rw_unlock2` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 310 | `getsid` | process | requires semantic emulation before mapping |  |
| 311 | `sys_settid_with_pid` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 312 | `psynch_cvclrprepost` | pthread/thread | requires semantic emulation before mapping | conditional alt: nosys |
| 313 | `aio_fsync` | pthread/thread | requires semantic emulation before mapping |  |
| 314 | `aio_return` | pthread/thread | requires semantic emulation before mapping |  |
| 315 | `aio_suspend` | pthread/thread | requires semantic emulation before mapping |  |
| 316 | `aio_cancel` | pthread/thread | requires semantic emulation before mapping |  |
| 317 | `aio_error` | pthread/thread | requires semantic emulation before mapping |  |
| 318 | `aio_read` | pthread/thread | requires semantic emulation before mapping |  |
| 319 | `aio_write` | pthread/thread | requires semantic emulation before mapping |  |
| 320 | `lio_listio` | pthread/thread | requires semantic emulation before mapping |  |
| 321 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old __pthread_cond_wait |
| 322 | `iopolicysys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 323 | `process_policy` | process | requires semantic emulation before mapping |  |
| 324 | `mlockall` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 325 | `munlockall` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 326 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 327 | `issetugid` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 328 | `__pthread_kill` | signal | requires semantic emulation before mapping |  |
| 329 | `__pthread_sigmask` | signal | requires semantic emulation before mapping |  |
| 330 | `__sigwait` | signal | requires semantic emulation before mapping |  |
| 331 | `__disable_threadsignal` | signal | requires semantic emulation before mapping |  |
| 332 | `__pthread_markcancel` | pthread/thread | requires semantic emulation before mapping |  |
| 333 | `__pthread_canceled` | pthread/thread | requires semantic emulation before mapping |  |
| 334 | `__semwait_signal` | pthread/thread | requires semantic emulation before mapping |  |
| 335 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old utrace |
| 336 | `proc_info` | process | requires semantic emulation before mapping |  |
| 337 | `sendfile` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 338 | `stat64` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 339 | `sys_fstat64` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 340 | `lstat64` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 341 | `stat64_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 342 | `lstat64_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 343 | `sys_fstat64_extended` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 344 | `getdirentries64` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 345 | `statfs64` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 346 | `fstatfs64` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 347 | `getfsstat64` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 348 | `__pthread_chdir` | pthread/thread | requires semantic emulation before mapping |  |
| 349 | `__pthread_fchdir` | pthread/thread | requires semantic emulation before mapping |  |
| 350 | `audit` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 351 | `auditon` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 352 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 353 | `getauid` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 354 | `setauid` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 355 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old getaudit |
| 356 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old setaudit |
| 357 | `getaudit_addr` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 358 | `setaudit_addr` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 359 | `auditctl` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 360 | `bsdthread_create` | pthread/thread | requires semantic emulation before mapping |  |
| 361 | `bsdthread_terminate` | pthread/thread | requires semantic emulation before mapping |  |
| 362 | `kqueue` | kqueue/event | requires semantic emulation before mapping |  |
| 363 | `kevent` | kqueue/event | requires semantic emulation before mapping |  |
| 364 | `lchown` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 365 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old stack_snapshot |
| 366 | `bsdthread_register` | pthread/thread | requires semantic emulation before mapping |  |
| 367 | `workq_open` | pthread/thread | requires semantic emulation before mapping |  |
| 368 | `workq_kernreturn` | pthread/thread | requires semantic emulation before mapping |  |
| 369 | `kevent64` | kqueue/event | requires semantic emulation before mapping |  |
| 370 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old __semwait_signal |
| 371 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old __semwait_signal |
| 372 | `thread_selfid` | pthread/thread | requires semantic emulation before mapping |  |
| 373 | `ledger` | process | requires semantic emulation before mapping |  |
| 374 | `kevent_qos` | kqueue/event | requires semantic emulation before mapping |  |
| 375 | `kevent_id` | kqueue/event | requires semantic emulation before mapping |  |
| 376 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 377 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 378 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 379 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 380 | `__mac_execve` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 381 | `__mac_syscall` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: enosys |
| 382 | `__mac_get_file` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 383 | `__mac_set_file` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 384 | `__mac_get_link` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 385 | `__mac_set_link` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 386 | `__mac_get_proc` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 387 | `__mac_set_proc` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 388 | `__mac_get_fd` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 389 | `__mac_set_fd` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 390 | `__mac_get_pid` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 391 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 392 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 393 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 394 | `pselect` | kqueue/event | requires semantic emulation before mapping |  |
| 395 | `pselect_nocancel` | kqueue/event | requires semantic emulation before mapping |  |
| 396 | `read_nocancel` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 397 | `write_nocancel` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 398 | `open_nocancel` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 399 | `sys_close_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 400 | `wait4_nocancel` | process | requires semantic emulation before mapping |  |
| 401 | `recvmsg_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 402 | `sendmsg_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 403 | `recvfrom_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 404 | `accept_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 405 | `msync_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 406 | `sys_fcntl_nocancel` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 407 | `select_nocancel` | kqueue/event | requires semantic emulation before mapping |  |
| 408 | `fsync_nocancel` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 409 | `connect_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 410 | `sigsuspend_nocancel` | signal | requires semantic emulation before mapping |  |
| 411 | `readv_nocancel` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 412 | `writev_nocancel` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 413 | `sendto_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 414 | `pread_nocancel` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 415 | `pwrite_nocancel` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 416 | `waitid_nocancel` | process | requires semantic emulation before mapping |  |
| 417 | `poll_nocancel` | kqueue/event | requires semantic emulation before mapping |  |
| 418 | `msgsnd_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 419 | `msgrcv_nocancel` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 420 | `sem_wait_nocancel` | pthread/thread | requires semantic emulation before mapping |  |
| 421 | `aio_suspend_nocancel` | pthread/thread | requires semantic emulation before mapping |  |
| 422 | `__sigwait_nocancel` | signal | requires semantic emulation before mapping |  |
| 423 | `__semwait_signal_nocancel` | pthread/thread | requires semantic emulation before mapping |  |
| 424 | `__mac_mount` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 425 | `__mac_get_mount` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 426 | `__mac_getfsstat` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 427 | `fsgetpath` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion | private fsgetpath (File Manager SPI) |
| 428 | `audit_session_self` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 429 | `audit_session_join` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 430 | `sys_fileport_makeport` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 431 | `sys_fileport_makefd` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 432 | `audit_session_port` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 433 | `pid_suspend` | process | requires semantic emulation before mapping |  |
| 434 | `pid_resume` | process | requires semantic emulation before mapping |  |
| 435 | `pid_hibernate` | process | requires semantic emulation before mapping | conditional alt: nosys |
| 436 | `pid_shutdown_sockets` | process | requires semantic emulation before mapping | conditional alt: nosys |
| 437 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old shared_region_slide_np |
| 438 | `nosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | old shared_region_map_and_slide_np |
| 439 | `kas_info` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 440 | `memorystatus_control` | process | requires semantic emulation before mapping | conditional alt: nosys |
| 441 | `guarded_open_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 442 | `guarded_close_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 443 | `guarded_kqueue_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 444 | `change_fdguard_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 445 | `usrctl` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 446 | `proc_rlimit_control` | process | requires semantic emulation before mapping |  |
| 447 | `connectx` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 448 | `disconnectx` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 449 | `peeloff` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 450 | `socket_delegate` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 451 | `telemetry` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 452 | `proc_uuid_policy` | process | requires semantic emulation before mapping | conditional alt: nosys |
| 453 | `memorystatus_get_level` | process | requires semantic emulation before mapping | conditional alt: nosys |
| 454 | `system_override` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 455 | `vfs_purge` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 456 | `sfi_ctl` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 457 | `sfi_pidctl` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 458 | `coalition` | process | requires semantic emulation before mapping | conditional alt: enosys |
| 459 | `coalition_info` | process | requires semantic emulation before mapping | conditional alt: enosys |
| 460 | `necp_match_policy` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 461 | `getattrlistbulk` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 462 | `clonefileat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 463 | `openat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 464 | `openat_nocancel` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 465 | `renameat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 466 | `faccessat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 467 | `fchmodat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 468 | `fchownat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 469 | `fstatat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 470 | `fstatat64` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 471 | `linkat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 472 | `unlinkat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 473 | `readlinkat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 474 | `symlinkat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 475 | `mkdirat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 476 | `getattrlistat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 477 | `proc_trace_log` | process | requires semantic emulation before mapping |  |
| 478 | `bsdthread_ctl` | pthread/thread | requires semantic emulation before mapping |  |
| 479 | `openbyid_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 480 | `recvmsg_x` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 481 | `sendmsg_x` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 482 | `thread_selfusage` | pthread/thread | requires semantic emulation before mapping |  |
| 483 | `csrctl` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: enosys |
| 484 | `guarded_open_dprotected_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 485 | `guarded_write_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 486 | `guarded_pwrite_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 487 | `guarded_writev_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 488 | `renameatx_np` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 489 | `mremap_encrypted` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: enosys |
| 490 | `netagent_trigger` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: nosys |
| 491 | `stack_snapshot_with_config` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 492 | `microstackshot` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: enosys |
| 493 | `grab_pgo_data` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target | conditional alt: enosys |
| 494 | `persona` | process | requires semantic emulation before mapping | conditional alt: enosys |
| 495 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 496 | `mach_eventlink_signal` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 497 | `mach_eventlink_wait_until` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 498 | `mach_eventlink_signal_wait_until` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 499 | `work_interval_ctl` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 500 | `getentropy` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 501 | `necp_open` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 502 | `necp_client_action` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 503 | `__nexus_open` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 504 | `__nexus_register` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 505 | `__nexus_deregister` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 506 | `__nexus_create` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 507 | `__nexus_destroy` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 508 | `__nexus_get_opt` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 509 | `__nexus_set_opt` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 510 | `__channel_open` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 511 | `__channel_get_info` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 512 | `__channel_sync` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 513 | `__channel_get_opt` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 514 | `__channel_set_opt` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 515 | `sys_ulock_wait` | pthread/thread | requires semantic emulation before mapping |  |
| 516 | `sys_ulock_wake` | pthread/thread | requires semantic emulation before mapping |  |
| 517 | `fclonefileat` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 518 | `fs_snapshot` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 519 | `enosys` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 520 | `terminate_with_payload` | signal | requires semantic emulation before mapping |  |
| 521 | `abort_with_payload` | signal | requires semantic emulation before mapping |  |
| 522 | `necp_session_open` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 523 | `necp_session_action` | socket/network | requires semantic emulation before mapping | conditional alt: enosys |
| 524 | `setattrlistat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 525 | `net_qos_guideline` | socket/network | requires semantic emulation before mapping |  |
| 526 | `fmount` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 527 | `ntp_adjtime` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 528 | `ntp_gettime` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 529 | `os_fault_with_payload` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 530 | `kqueue_workloop_ctl` | kqueue/event | requires semantic emulation before mapping |  |
| 531 | `__mach_bridge_remote_time` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 532 | `coalition_ledger` | process | requires semantic emulation before mapping | conditional alt: enosys |
| 533 | `log_data` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 534 | `memorystatus_available_memory` | process | requires semantic emulation before mapping |  |
| 535 | `objc_bp_assist_cfg_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 536 | `shared_region_map_and_slide_2_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 537 | `pivot_root` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 538 | `task_inspect_for_pid` | process | requires semantic emulation before mapping |  |
| 539 | `task_read_for_pid` | process | requires semantic emulation before mapping |  |
| 540 | `sys_preadv` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 541 | `sys_pwritev` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 542 | `sys_preadv_nocancel` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 543 | `sys_pwritev_nocancel` | straightforward scalar/file | mechanical Linux syscall mapping |  |
| 544 | `sys_ulock_wait2` | pthread/thread | requires semantic emulation before mapping |  |
| 545 | `proc_info_extended_id` | process | requires semantic emulation before mapping |  |
| 546 | `tracker_action` | socket/network | requires semantic emulation before mapping | conditional alt: nosys |
| 547 | `debug_syscall_reject` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 548 | `sys_debug_syscall_reject_config` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 549 | `graftdmg` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 550 | `map_with_linking_np` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 551 | `freadlink` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 552 | `sys_record_system_event` | Mach/guarded/unsupported/no-op/nosys/obsolete | unsupported/no-op/nosys/obsolete for this target |  |
| 553 | `mkfifoat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 554 | `mknodat` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 555 | `ungraftdmg` | path/flags/struct conversion | Linux mapping with Darwin path/flags/struct conversion |  |
| 556 | `sys_coalition_policy_set` | process | requires semantic emulation before mapping | conditional alt: enosys |
| 557 | `sys_coalition_policy_get` | process | requires semantic emulation before mapping | conditional alt: enosys |

Summary counts:
- Mach/guarded/unsupported/no-op/nosys/obsolete: 199
- path/flags/struct conversion: 109
- pthread/thread: 65
- process: 64
- straightforward scalar/file: 44
- socket/network: 40
- signal: 15
- kqueue/event: 12
- already implemented: 10
