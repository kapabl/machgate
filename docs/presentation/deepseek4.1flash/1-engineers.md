# MachGate: running ARM64 Mach-O CLIs inside ARM64 Linux

## MachGate: Darwin syscalls, natively on Linux

MachGate runs ARM64 Mach-O command-line executables inside ARM64 Linux containers.

- Same ISA on both sides: native ARM64 execution.
- No instruction emulation, no hypervisor, no guest kernel.
- Defining feature: a Darwin-to-Linux syscall gateway.
- Guests may issue `svc #0x80` directly, including static runtimes.
- This deck is for engineers who will touch the code.

> **Speaker notes:** This is an engineering briefing, not a product pitch. MachGate takes an ARM64 Mach-O command-line executable and runs it on an ARM64 Linux host. The two sides share an instruction set, so nothing is translated instruction by instruction: the CPU executes guest code directly. What cannot be shared is the syscall contract, and that is the whole point of the project. Everything you will see in this deck comes from the repository documentation: the README, the domain model, the syscall patching design, and the syscall coverage tracker. Read those four files before you change anything.

## The problem: Darwin binaries don't speak Linux

- Darwin ARM64 puts the syscall number in `x16`; Linux expects `x8`.
- Darwin entry is `svc #0x80`, opcode `0xd4001001`.
- Darwin success: `x0` holds the result, carry clear.
- Darwin failure: `x0` holds a Darwin errno, carry set.
- Linux instead returns a negative errno with different flags and structures.

> **Speaker notes:** The blocker is not the instruction set, it is the ABI. Darwin and Linux both use `svc` to enter the kernel, but they disagree about which register holds the syscall number, which table that number indexes, and how failure is reported. A Darwin `svc` executed unmodified on Linux would land in the Linux kernel with the wrong number register, the wrong table, and the wrong error convention. Interposition alone is not enough because statically linked guests and direct-syscall runtimes never call an imported libSystem function we could redirect. Every `svc` site has to be intercepted, not just imported calls.

## Economics: why our test workloads run on Linux

- Engine tests are command-line binaries exercising engine code.
- They need no GUI, no Apple frameworks, no full macOS.
- Even Mach-O test fixtures build on Linux with an LLVM toolchain.
- No Xcode and no Mac is required to build or run them.
- ARM64 Linux containers start in seconds and scale out in parallel CI.

> **Speaker notes:** If our tests require Apple hardware, the test fleet becomes a cost problem, not just an engineering problem. A Mac fleet, or hourly cloud Mac instances, comes with long minimum allocations. Our tests do not need any of that: they are command-line binaries that exercise the engine, and even the Mach-O fixtures are produced on Linux with an LLVM toolchain. ARM64 Linux containers start in seconds and parallelize cheaply, and because both sides are ARM64 we still exercise real ARM64 code paths natively rather than under emulation. The net effect is the same behavioral coverage without Apple hardware and far more test iterations per dollar.

## Scope: deliberately narrow, with one honest nuance

- In: ARM64 Mach-O executables on ARM64 Linux.
- In: thin files and ARM64 slices of fat files.
- In: command-line workloads, with a single guest thread as the design target.
- Out: x86-64, GUI, Metal, Apple frameworks, Mach IPC.
- Provisional: host-backed pthread lifecycle, `spawn`, `wait`, and `vfork`.

> **Speaker notes:** Read the scope as a contract, not a limitation we are apologizing for. The design target is one guest thread with one guarded main stack and one guest entry thread. The room for confusion is that the Compatibility Runtime currently exposes selected host-backed pthread behavior, and host-backed spawn, wait, and vfork remain because current CLI probes and behavior tests import those symbols. That is provisional compatibility, not an enforced single-thread guarantee, and the domain model lists reconciling it as open work. If you touch that area, treat the domain model's stated gap as the source of truth.

## Not just macOS: an iOS binary is a Mach-O file

- An iOS executable is the same container as a macOS one.
- Same `MH_EXECUTE`, `LC_SEGMENT_64`, `LC_MAIN`, and chained binds.
- The loader validates the ARM64 slice regardless of where it came from.
- In scope: CLI-style binaries, static runtimes, universal slices.
- Fails closed: UIKit and other iOS-only frameworks, Metal, Mach IPC.

> **Speaker notes:** The container format does not carry a macOS flag that we branch on. A Mach-O executable is a Mach-O executable: header, segments, an entry point, and import bindings. MachGate validates that it is an `MH_EXECUTE` with an ARM64 slice and proceeds. That means CLI-style iOS binaries and static runtimes are in scope, including ARM64 slices extracted from universal or fat files. What is out of scope is defined by dependencies and behavior, not by origin: iOS-only frameworks such as UIKit, Metal, and Mach IPC fail closed rather than silently succeeding.

## Core idea: patch every Darwin syscall before the guest runs

- Scan declared executable instruction sections for `svc #0x80`.
- Replace each site with a direct branch to a generated island.
- The guest never reaches the Linux kernel with Darwin conventions.
- Runs before any static initializer or entry point is called.
- Scans only ARM64 instruction sections, never data.

> **Speaker notes:** This is the central mechanism, so get it exactly right. MachGate does not wait for a Darwin syscall to trap and then fix it up. It scans executable instruction sections before the guest runs, finds each `svc #0x80`, and overwrites it with a branch. Because ARM64 has fixed four-byte instructions, the scan walks those sections in four-byte units and matches the exact opcode, which keeps replacement at real instruction boundaries. Data sections are never touched, so a matching byte pattern in data cannot be corrupted. From the guest's point of view, the syscall wrapper still works exactly as Darwin specifies.

## Loading: file to Guest Image

- Select the thin file or the ARM64 slice of a fat file.
- Validate `MH_EXECUTE` and arm64; map `__TEXT`/`__DATA` at requested addresses with the slide.
- Set `r-x`/`rw-` protections; take the entry point from `LC_MAIN` or a supported `LC_UNIXTHREAD`.
- Lay out the guarded guest stack, arguments, environment, and executable path.
- Invariant: all-or-nothing, a failure leaves nothing mapped.

*Figure: `docs/diagrams/guest-loading.svg`*

> **Speaker notes:** Loading is a five-step pipeline with one hard invariant: a Guest Image exists only after the entire input has been validated and mapped successfully. If anything fails, nothing stays mapped, so there is no partially built aggregate for later stages to trip over. Entry point selection is equally strict: it comes from `LC_MAIN` or a supported `LC_UNIXTHREAD`, and it is one validated address, never an arbitrary symbol. Every file range, command range, and mapped interval is validated before use. Once the image exists, MachGate allocates the guarded guest stack, lays out arguments and environment, runs static initializers, and only then enters guest code.

## The Darwin ARM64 syscall ABI in one slide

- Number in `x16`, arguments beginning at `x0`.
- Entry via `svc #0x80`, opcode `0xd4001001`.
- Success: `x0` holds the return value, carry clear.
- Failure: `x0` holds a Darwin errno, carry set.
- Nothing is assumed to agree between Darwin and Linux.

> **Speaker notes:** Keep this slide in your head while reading the patching code. The guest passes the Darwin syscall number in `x16` and the arguments from `x0` onward, and it enters the kernel with `svc #0x80`. The result convention is carry-based: clear carry means success with the value in `x0`, set carry means failure with a Darwin errno in `x0`. The Linux side is different in all three respects, and the differences are not cosmetic. The project rule is explicit: translate numbers, flags, structures, return values, and errno values deliberately. Never cast two similarly named structures and assume they fit.

## The patch: one instruction becomes a branch

- Use `b`, not `bl`: the guest link register `x30` must survive.
- The island saves `x0..x18`, `x30`, `NZCV`, and resume PC `N+4`.
- It sets `x0 = &syscall_gate_state` and reaches C with `blr`.
- It restores registers and flags, then `br` back to `N+4`.
- `blr` is used only for MachGate's own call into the dispatcher.

*Figure: `docs/diagrams/syscall-patching.svg`*

> **Speaker notes:** Two instructions matter here and they are easy to confuse. The patch in guest text uses a direct `b`, not a branch-with-link, because guest code may rely on `x30` surviving the syscall wrapper; destroying the link register would break ordinary returns. Inside the generated island, `blr` is used once, to call the C dispatcher, which is MachGate's own call and is free to clobber a link register it saves first. The island preserves the full promised state: `x0` through `x18`, `x30`, the condition flags, and the continuation address `N+4`. It restores the translated result and resumes exactly as if the original instruction had completed.

## Islands: the ±128 MiB branch-reach constraint

- ARM64 instructions are fixed width and `b` has a scaled 26-bit immediate.
- Reach is roughly plus or minus 128 MiB from the patched instruction.
- MachGate allocates executable island pools near the guest image.
- One island is emitted per patched Syscall Site.
- Rule: every patched branch must be in range of its island.

> **Speaker notes:** The island design is forced by encoding, not preference. An ARM64 `b` carries a signed 26-bit immediate scaled by four bytes, which works out to roughly plus or minus 128 MiB of reach. We cannot place islands wherever it is convenient, so MachGate allocates executable pools near the guest code and emits one island per patched syscall site. The rule that follows is simple to state and easy to violate: every patched branch must be within direct-branch range of its island. If you ever change mapping addresses or pool placement, re-check that constraint, because the failure mode is a silently wrong target rather than a clean error.

## Dispatcher, range dispatchers, and coverage

- `src/syscall/syscall_gate.c` reads the Darwin number from `state->x[16]`.
- It routes across four range dispatchers: `000_099`, `100_199`, `200_399`, `400_plus`.
- Translation is explicit for numbers, arguments, flags, structures, results, and errno.
- Raw Linux syscalls are issued internally with `x8 = SYS_*`.
- Coverage: 669/669 source rows classified, 455 translated (68%), 214 explicit ENOSYS (32%).
- Invariant: a raw host ABI error never crosses the boundary.

*Figure: `docs/diagrams/syscall-dispatch.svg`*

> **Speaker notes:** The dispatcher is plain C, and that is deliberate: generated code saves state, and the dispatcher reasons about it. It reads the Darwin number from `state->x[16]` and routes it to one of four range dispatchers, numbered 000 to 099, 100 to 199, 200 to 399, and 400 and up. Numeric ranges are only an infrastructure router, not separate domains. Coverage is tracked against the Apple xnu `bsd/kern/syscalls.master` rows: 669 of 669 classified, 455 translated, 214 explicit Darwin `ENOSYS`, with zero pending and zero misclassified. The invariant that holds the boundary together is absolute: a raw host ABI error never crosses back to the guest.

## Native linkage, provider decisions, and the shim

- Classic and chained Mach-O imports bind to an explicitly selected Native Provider.
- Provider Decisions: `Unmapped`, `Map`, `Skip`, `Stub`, and fail-closed `Partial`.
- Lookup order: Guest Definitions, then selected providers in load-command order, then the ambient host namespace.
- Shim: `build/libsystem_shim.so`, selected with `MACHGATE_SYSTEM_SHIM_LIB`.
- Knobs: `MACHGATE_DYLIB_MAP`, `MACHGATE_TRAMPOLINE_LIB`, `MACHGATE_TRAMPOLINE_PREFIX`, `MACHGATE_TRACE_CXX_ALLOCATORS`.

*Figure: `docs/diagrams/host-components.svg`*

> **Speaker notes:** Native Linkage is not a general dynamic loader, and it only writes to validated bind locations inside a Guest Image. Each dependency gets an explicit Provider Decision. `Partial` deserves attention: it selects a provider for its supported symbols and fails closed for everything outside that declared surface, which is not permission to implement an unavailable API. Lookup order exists to prevent a bug class: Guest Definitions win, then selected dependency providers in load-command order, and only then the ambient host namespace, so a same-named host symbol cannot bypass the Darwin compatibility provider. The shim is `build/libsystem_shim.so`, and the environment variables on this slide are explicit selection and diagnostics, not a stable public interface.

## Guest VM is the guest's virtual-memory ledger

- Guest VM is the guest's virtual-memory ledger, not a virtual machine.
- No hypervisor, no guest kernel: it tracks guest mappings.
- `mmap`, `munmap`, and `mprotect` update it in `guest_vm_dispatch.c` and `guest_vm_track.c`.
- Host mappings join through `src/vm_interpose.c` to `build/libmachgate_vm_interpose.so`.
- One ledger, two writers.

> **Speaker notes:** The name Guest VM misleads people, so define it precisely: it is the guest's virtual memory ledger, the record of which guest-visible addresses are mapped with which protection. There is no hypervisor and no guest kernel. Direct guest `mmap`, `munmap`, and `mprotect` syscalls update the ledger. Separately, some host mappings have to participate in guest tracking, and those are brought in by the VM interposer, a small shared object built as `build/libmachgate_vm_interpose.so`. One ledger with two writers is a real concurrency and ownership hazard, and the domain model lists making ownership, capacity failure, and model updates explicit as open work.

## Guest `execve` is Process Replacement

- A guest `execve` on a Mach-O target becomes Process Replacement.
- `src/syscall/execve_reexec.c` validates the new target and re-invokes MachGate.
- Success never returns; the process image is replaced.
- Failure returns a Darwin error and preserves the required loader state.
- Direct native `execve` of Mach-O bytes is not an option.

> **Speaker notes:** A guest that calls `execve` is asking to replace itself with another Mach-O executable. Handing those bytes to the Linux kernel would fail, because Linux has no Mach-O loader. Instead, MachGate validates the new target, preserves the required runtime environment, constructs a new MachGate invocation, and re-executes itself with that target. The two outcomes are strictly defined: a successful replacement never returns to the caller, exactly as `execve` promises, and a failed replacement returns a Darwin error to the guest. The translation table records the plain `execve` and `__mac_execve` rows as validated requests that return a Darwin error for unsupported replacement.

## Device profiles: controlling what the guest believes

- Every "what machine am I on" question funnels through one choke point.
- `syscall_range_200_399.c` answers `hw.ncpu`, the `*cpu` and `*cpu_max` names, `hw.availcpu`, and `machdep.cpu.{core,thread}_count`.
- It also answers `hw.memsize`, `hw.pagesize`, and `hw.machine` as `arm64`.
- CPU count, memory, and page size come from `src/shim/host_facts.c` — the real host today.
- Identity strings are already fixed constants: `src/shim/sysctl_compat.c` answers model `VirtualMac2,1`.
- An affinity query is translated in `syscall_range_400_plus.c`; a selectable profile does not exist yet.

*Figure: `docs/diagrams/device-simulation.svg`*

> **Speaker notes:** Be precise about the status here, because it is the easiest thing on this deck to overstate. Two of the three answer classes exist already. CPU count, memory, and page size are read from the real host: `machgate_host_cpu_count()` goes through `sysconf(_SC_NPROCESSORS_ONLN)`. Identity strings are already synthetic: the shim's sysctl table answers `hw.machine` as `arm64`, the model as `VirtualMac2,1`, and Darwin version strings as fixed constants. What does not exist is a selectable, enforced profile. Because CPU count and identity funnel through one choke point, a profile could answer with a smaller core count, a different model string, or capped memory, and enforce the core count with `sched_setaffinity` pinning so thread counts and timing line up. The design permits that; nothing ships it today, and any implementation adds tests first.

## Testing: what backs the claims

- About 50 behavior suites at the `tests/` top level.
- 137 fixture sources under `tests/fixtures/`.
- Pinned public ARM64 Mach-O CLI binaries in `tests/external` as external probes.
- `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug`, then `cmake --build build --parallel`.
- `bash tests/fixtures/build_fixtures.sh`, then `bash tests/run_tests.sh`; run a guest with `build/machgate path/to/program`.

> **Speaker notes:** Every behavioral claim in this deck is meant to be reproducible from the test tree. There are around fifty behavior suites at the top level, with names like `test_darwin_range_000_099.sh`, `test_fat_binary.sh`, and `test_dynamic_chained_libsystem_call.sh`. The fixture builder needs an LLVM toolchain that can emit ARM64 Mach-O objects and executables, and it produces the 137 fixture sources under `tests/fixtures/`. On top of that, `tests/external` holds pinned public ARM64 Mach-O CLI binaries that we run as external probes. A probe failure is evidence to classify by bounded context before you pick a fix, not automatically an engine bug.

## Ground rules for contributors

- Clean-room: implement from Apple open-source format and ABI references.
- Also from LLVM/LLD docs, public ABI documentation, and MachGate behavior tests.
- Never implement from the legacy sibling checkout; it only classifies provenance.
- Add or update a behavioral test before changing behavior; keep changes narrowly scoped.
- Keep existing syscall-range coverage; no GUI, game, or workload-specific shims in the core.
- Unsupported behavior fails closed and never fakes success.

> **Speaker notes:** These rules are what keep the rewrite defensible. Derive implementation from Apple open-source format and ABI references, LLVM and LLD behavior and documentation, public ABI documentation, and MachGate's own behavior tests. The sibling reference checkout is used only to classify provenance and decide what legacy material can be deleted. It is not an implementation or behavior specification, and implementation text must not be copied from it. Behavior changes require a test first. Keep changes narrow, preserve syscall-range coverage, keep GUI, game, and workload-specific shims out of the core, and make unsupported behavior fail closed rather than pretending to succeed. MachGate is GPL-3.0-or-later with required notices in `ATTRIBUTION.md`.

## Where to look, and a good first change

- Read `README.md`, `docs/DOMAIN_MODEL.md`, `docs/SYSCALL_PATCHING.md`, `docs/DARWIN_SYSCALL_TABLE_STATUS.md`.
- Diagrams live in `docs/diagrams/`, including `syscall-patching.svg` and `syscall-dispatch.svg`.
- Code entry points: `src/machgate.c`, `src/loader.c`, `src/syscall/`, `src/shim/`.
- Good first change: implement the device-profile override at the sysctl choke point, test first.

> **Speaker notes:** Start with the four documents named on this slide. The domain model is the source of the ubiquitous language and the invariants; the syscall patching document explains the gateway; the syscall table status tells you exactly what is translated and what returns `ENOSYS`. Use `rg`, `git diff`, and direct file reads for discovery. A genuinely useful first change is the device-profile override, because the choke point already exists in the sysctl translation and the change is contained: add a test that asserts the current host-derived answers, then make the override selectable. Follow the invariants in the domain model while you do it, and keep the fail-closed rule intact.
