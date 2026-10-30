# MachGate: running ARM64 Mach-O command-line programs directly on ARM64 Linux

## MachGate: running Mach-O binaries natively on Linux

MachGate runs ARM64 Mach-O command-line executables directly on ARM64 Linux.

- Both sides use the same instrupction set, ARM64: guest code runs natively on the host CPU.
- No instruction emulation, no hypervisor, no guest kernel.
- Three core parts: a loader, guest virtual memory, and a syscall gateway.
- Defining feature: the Darwin-to-Linux syscall gateway.
- This deck is for engineers who will touch the code.

> **Speaker notes:** **MachGate moves Apple command-line execution onto native arm64 Linux.** [PAUSE] No CPU emulation, no guest kernel, and no hypervisor. The architecture has three parts: a Mach-O loader, guest virtual memory tracking, and the Darwin-to-Linux syscall gateway. The gateway is the defining piece. I'll walk through these in dependency order, then cover the invariants that we expect every contributor to preserve.

## The problem: a Mach-O file cannot run on Linux as it is

- Linux cannot `exec` a Mach-O file: the kernel has no Mach-O loader.
- Darwin ARM64 puts the syscall number in `x16`; Linux expects `x8`.
- Darwin entry is `svc #0x80`, opcode `0xd4001001`.
- Darwin success: `x0` holds the result, carry clear; failure is a Darwin errno with carry set.
- Linux instead returns a negative errno, with different flags and structures.
- Interposition alone cannot fix it: static guests never call an imported function.

> **Speaker notes:** **There are two independent barriers, and we solve both.** Linux cannot load Mach-O, and a loaded Darwin program cannot use Linux syscalls as-is. Darwin uses `x16` and `svc #0x80`; Linux uses `x8`, different syscall numbers, and a different error convention. Static code can make those calls directly. [PAUSE] That is why the solution requires both a loader and interception at the instruction level.

## Three core parts, in the order they matter

- Part 1, the loader: put a Mach-O image into host memory and enter it.
- Part 2, guest virtual memory: own the record of guest mappings and protections.
- Part 3, the syscall gateway: translate every Darwin syscall into Linux.
- Everything else in the tree exists to support one of these three.
- This deck covers them in that order.

> **Speaker notes:** **Think of MachGate as three responsibilities with clear ownership.** The loader creates a valid Guest Image. Guest VM tracks its mappings and protections. The syscall gateway enforces the Darwin contract at the Linux boundary. [PAUSE] Linkage, shims, and supporting components fit around those responsibilities. This separation is how we keep the system understandable as coverage grows.

## Economics: these tests don't need Apple hardware

- Our engine tests are command-line binaries: no GUI, no graphical macOS session.
- Running them on Apple hardware pays for capability the tests never use.
- Macs are expensive to buy, and cloud macOS capacity is expensive to rent.
- Apple equipment is also harder to scale, so it limits how much we can test.
- General-purpose Linux machines give more test capacity for the same outlay.
- The economy of the approach is general: same coverage, without buying or renting hardware the tests never touch.

> **Speaker notes:** **We should not require premium platform hardware for platform-independent computation.** Our engine unit tests are command-line programs; they do not use a macOS desktop. Buying Macs or renting macOS workers adds cost and constrains scaling without serving these tests. [PAUSE] The direction is to reserve Apple capacity for the tests that genuinely need it, and run the rest on scalable Linux arm64 compute.

## Scope: deliberately narrow

- In: ARM64 Mach-O executables on ARM64 Linux.
- In: thin files and ARM64 slices of fat files.
- In: command-line workloads, including runtimes that create threads and child processes.
- Out: x86-64, GUI, Metal, Apple frameworks, Mach IPC.
- Threads and child processes come from the host, through the Compatibility Runtime.

> **Speaker notes:** **Our scope is deliberate, and it is the basis for reliability.** We run arm64 Mach-O command-line workloads, including programs that use host-backed threads and child processes. We are not recreating macOS, Metal, UIKit, or Mach IPC. The Compatibility Runtime delegates threading and process facilities to Linux. [PAUSE] Every new feature must fit this boundary, not silently expand it.

## The Mach-O file: what the names mean

- The file is a small header plus a flat list of load commands; `LC_` means Load Command, and the rest of each name says what it commands.
- `LC_SEGMENT_64` = "map this segment": `__TEXT` (code) and `__DATA` (writable data) say what lands where in memory, and with which permissions.
- `LC_MAIN` = "start execution here", given as a file offset plus a stack size; `LC_UNIXTHREAD` is the older form, a saved register snapshot that boils down to the program counter.
- `LC_LOAD_DYLIB` = "load this dynamic library at launch"; `LC_SYMTAB` and `LC_DYSYMTAB` point at the symbol tables.
- `LC_DYLD_INFO_ONLY` and `LC_DYLD_CHAINED_FIXUPS` are the binding instructions for dyld, Apple's dynamic linker.
- Sections carry the flags the rest of this deck relies on: "this is machine code" (`S_ATTR_PURE_INSTRUCTIONS`) and "these are static constructors" (`S_MOD_INIT_FUNC_POINTERS`).
- The constants live in `src/macho_defs.h`; the command walk lives in `src/loader.c`.

> **Speaker notes:** **The loader is driven by explicit Mach-O metadata, not heuristics.** `LC_SEGMENT_64` defines memory placement, `LC_MAIN` or `LC_UNIXTHREAD` defines entry, and dyld commands describe imports. The section flags distinguish executable instructions from constructors. For engineers familiar with ELF, these correspond roughly to loadable segments, an entry point, and dynamic dependencies. [PAUSE] The rule is to validate each command before trusting its addresses or lengths.

## Not just macOS: an iOS binary is a Mach-O file

- An iOS executable uses the same file format as a macOS one.
- Same `LC_SEGMENT_64` segments and `LC_MAIN` entry point, same chained binds.
- The loader validates the ARM64 slice regardless of where it came from.
- In scope: command-line binaries, static runtimes, universal slices.
- Fails closed: UIKit and other iOS-only frameworks, Metal, Mach IPC.

> **Speaker notes:** **Origin does not determine compatibility; dependencies do.** macOS and iOS command-line binaries share the Mach-O structure we load. We select the arm64 executable slice and apply the same validation. If the binary depends on supported services, it is in scope. If it depends on UIKit, Metal, or Mach IPC, it fails closed. [PAUSE] This gives us a useful cross-platform execution boundary without pretending to provide an entire Apple OS.

## Part 1: loading a Mach-O file into a Guest Image

- Linux cannot `exec` a Mach-O file, so MachGate loads and enters it itself.
- Select the thin file or the ARM64 slice of a fat file.
- Validate the file is an ARM64 executable (`MH_EXECUTE`); map `__TEXT`/`__DATA` at the addresses the file requests.
- Set `r-x`/`rw-` protections; take the entry point from `LC_MAIN` or a supported `LC_UNIXTHREAD`.
- Lay out the guarded guest stack, arguments, environment, and executable path.
- Invariant: all-or-nothing, a failure leaves nothing mapped.

*Figure: `docs/diagrams/guest-loading.svg`*

> **Speaker notes:** **The loader has one non-negotiable invariant: all or nothing.** Select the slice, validate the commands and address ranges, map segments, establish protections, and prepare the guarded stack and entry point. Only after that do constructors and guest execution begin. [PAUSE] A failed load must not leave a partial Guest Image. That makes every downstream stage safer and easier to reason about.

## Part 2: guest virtual memory is a record MachGate owns

- Guest VM is the guest's virtual-memory record, not a virtual machine.
- No hypervisor, no guest kernel: MachGate tracks guest mappings itself.
- The guest's `mmap`, `munmap`, and `mprotect` calls update it, in `guest_vm_dispatch.c` and `guest_vm_track.c`.
- Host mappings join through `src/vm_interpose.c` to `build/libmachgate_vm_interpose.so`.
- One record, two writers: the domain model keeps the ownership and capacity rules explicit.
- Those mapping calls arrive through the syscall gateway covered in Part 3.

> **Speaker notes:** **Guest VM is a record, not virtualization.** It tracks the guest's mapped addresses and protections. Guest memory syscalls update it, and the host interposer supplies relevant host mappings. There are two paths into one record, so ownership and capacity handling must remain explicit. [PAUSE] The responsibility is to maintain a consistent guest-visible memory model, not to introduce another operating system.

## Part 3: the Darwin ARM64 syscall ABI in one slide

- Number in `x16`, arguments beginning at `x0`.
- Entry via `svc #0x80`, opcode `0xd4001001`.
- Success: `x0` holds the return value, carry clear.
- Failure: `x0` holds a Darwin errno, carry set.
- Nothing is assumed to agree between Darwin and Linux.

> **Speaker notes:** **The syscall ABI is the contract we must preserve exactly.** Darwin passes its syscall number in `x16`, arguments from `x0`, and uses carry plus a Darwin errno to report failure. Linux's representation is different. [PAUSE] We translate numbers, flags, structures, return values, and errors intentionally. Similar-looking types are not evidence of ABI compatibility.

## Why the ABIs differ: two Unix lineages

- Both kernels descend from Unix, so both offer the same core services.
- Darwin inherited the BSD table: `exit` is 1, `read` is 3, `write` is 4.
- Linux's 64-bit architectures adopted a new unified numbering: `read` is 63, `write` is 64, `exit` is 93.
- Same call, different number: `execve` is 59 on Darwin and 221 on Linux ARM64.
- Error reporting differs by lineage too: BSD's carry flag and errno versus Linux's negative errno.
- Darwin's kernel keeps a second, Mach-side trap table for Mach IPC, which stays out of scope.

> **Speaker notes:** **The mismatch has historical roots, but our response is engineering discipline.** Darwin carries BSD syscall numbering and carry-based errors; Linux arm64 uses another numbering scheme and negative errors. Even common operations such as `read`, `write`, and `execve` differ. Mach IPC is a separate, unsupported trap surface. [PAUSE] We do not rely on shared Unix ancestry; we implement the boundary explicitly.

## Part 3: patch every Darwin syscall before the guest runs

- Scan declared executable instruction sections for `svc #0x80`.
- Replace each site with a direct branch to a generated island.
- The guest never reaches the Linux kernel with Darwin conventions.
- Runs before any static initializer or entry point is called.
- Scans only ARM64 instruction sections, never data.

> **Speaker notes:** **We intercept Darwin syscalls before a single guest instruction runs.** MachGate scans executable arm64 instruction sections for `svc #0x80` and replaces those instructions with branches into generated islands. The scan stays aligned to four-byte instructions and never scans data as code. [PAUSE] This also catches static runtimes that bypass imported libraries. It is the mechanism that makes the entire approach viable.

## The patch: one instruction becomes a branch

- Use `b`, not `bl`: the guest link register `x30` must survive.
- The island saves the guest registers `x0..x18` and `x30`, the condition flags (`NZCV`), and the resume point `N+4`.
- It builds `syscall_gate_state` on the guest stack, puts its address in `x0`, and calls C with `blr`.
- It restores registers and flags, then `br` back to `N+4`.
- `blr` is used only for MachGate's own call into the dispatcher.

*Figure: `docs/diagrams/syscall-patching.svg`*

> **Speaker notes:** **Preserving guest state is more important than making the patch clever.** The guest patch uses `b`, not `bl`, so `x30` survives. The island saves the promised registers, flags, and continuation, calls the C dispatcher, restores the translated result, and resumes at `N+4`. [PAUSE] The visible contract is that execution continues as though the Darwin syscall had returned normally.

## Islands: the ±128 MiB branch-reach constraint

- ARM64 instructions are fixed width and `b` has a scaled 26-bit immediate.
- Reach is roughly plus or minus 128 MiB from the patched instruction.
- MachGate allocates executable island pools near the guest image.
- One island is emitted per patched Syscall Site.
- Rule: every patched branch must be in range of its island.

> **Speaker notes:** **Island placement is a correctness condition.** An arm64 direct branch reaches only about 128 MiB in either direction. We therefore allocate island pools close enough to each patched site and validate the reach. [PAUSE] If image mapping or pool allocation changes, this invariant must be rechecked. A misplaced island is not a performance bug; it is an execution bug.

## Dispatcher, range dispatchers, and coverage

- `src/syscall/syscall_gate.c` reads the Darwin number from `state->x[16]`.
- It routes across four range dispatchers: `000_099`, `100_199`, `200_399`, `400_plus`.
- Translation is explicit for numbers, arguments, flags, structures, results, and errno.
- Raw Linux syscalls are issued internally with `x8 = SYS_*`.
- Coverage: 669/669 source rows classified, 455 translated (68%), 214 returning `ENOSYS` "not implemented" (32%).
- Invariant: a raw host ABI error never crosses the boundary.

*Figure: `docs/diagrams/syscall-dispatch.svg`*

> **Speaker notes:** **Keep mechanism separate from semantics.** The generated island captures state; the C dispatcher translates the operation. It routes through four syscall-number ranges, and every Apple source row has an explicit classification: 455 translated, 214 returning Darwin `ENOSYS`, 669 classified in total. [PAUSE] The essential rule is that raw Linux ABI results never leak back into the guest.

## Native linkage, provider decisions, and the shim

- Classic and chained Mach-O imports bind to an explicitly selected Native Provider.
- Provider Decisions: `Unmapped`, `Map`, `Skip`, `Stub`, and fail-closed `Partial`.
- Lookup order: Guest Definitions, then selected providers in load-command order, then the ambient host namespace.
- Shim: `build/libsystem_shim.so`, selected with `MACHGATE_SYSTEM_SHIM_LIB`.
- Knobs: `MACHGATE_DYLIB_MAP`, `MACHGATE_TRAMPOLINE_LIB`, `MACHGATE_TRAMPOLINE_PREFIX`, `MACHGATE_TRACE_CXX_ALLOCATORS`.

*Figure: `docs/diagrams/host-components.svg`*

> **Speaker notes:** **Native imports must bind through an intentional provider decision.** Guest definitions win, then selected dependency providers, then the ambient host namespace. That order prevents accidental bypass of Darwin compatibility behavior. A `Partial` provider supports only its declared surface and fails closed elsewhere. [PAUSE] Symbol resolution is part of correctness, not a fallback search for anything with the same name.

## Guest `execve` is Process Replacement

- A guest `execve` on a Mach-O target becomes Process Replacement.
- `src/syscall/execve_reexec.c` validates the new target and re-invokes MachGate.
- Success never returns; the process image is replaced.
- Failure returns a Darwin error and preserves the required loader state.
- Direct native `execve` of Mach-O bytes is not an option.

> **Speaker notes:** **Process replacement brings us back to the loader contract.** Linux cannot execute Mach-O bytes directly, so a supported guest `execve` validates the new target and re-invokes MachGate. Success never returns. Failure returns a Darwin error and preserves the required state. [PAUSE] We preserve the semantics the guest expects while staying within the Linux execution model.

## Device profiles: what the guest believes about the machine

- Every "what machine am I on" question funnels through a single control point.
- `syscall_range_200_399.c` answers `hw.ncpu`, the `*cpu` and `*cpu_max` names, `hw.availcpu`, and `machdep.cpu.{core,thread}_count`.
- Those come from `src/shim/host_facts.c` through `sysconf`, and `hw.machine` answers `arm64`.
- Parallelism answers instead count `SYS_sched_getaffinity` in `syscall_range_400_plus.c`, so they follow the process's CPU restriction.
- The core count a guest sees is therefore already steerable: restrict the process and the affinity-derived answers shrink with it.
- The same control point makes a named profile — cores, model, memory in one setting — a contained extension.

*Figure: `docs/diagrams/device-simulation.svg`*

> **Speaker notes:** **Machine identity is becoming a controllable test input.** Some guest queries use host `sysconf` values; affinity-based parallelism queries respect the process's CPU mask. Those answers are not identical today: pinning cores changes the affinity-based result, not necessarily `hw.ncpu`. [PAUSE] A named profile for cores, model, and memory is a contained extension at that control point, but we must not describe it as shipped.

## Testing: what backs the claims

- 48 behavior test suites at the `tests/` top level.
- 137 fixture files under `tests/fixtures/`.
- Fixture Mach-O binaries are built on Linux with an LLVM toolchain, so no Xcode or Mac is involved.
- The engine's macOS and iOS unit tests run under MachGate: more than 99% pass.
- External gate: 57 checksum-pinned public ARM64 Mach-O probes, plus a 19-probe smoke set.
- `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug`, then `cmake --build build --parallel`.
- `bash tests/fixtures/build_fixtures.sh`, then `bash tests/run_tests.sh`; run a guest with `build/machgate path/to/program`.

> **Speaker notes:** **The strongest proof is our own workload.** A corpus of known macOS command-line programs runs under MachGate, Bitcoin's unit tests pass, and more than 99% of the engine's macOS and iOS test cases run and pass on Linux. Behind that sit 48 behavior suites, 137 fixture files, and 57 pinned external probes. These tests separate loader, linkage, memory, and syscall regressions. [PAUSE] Compatibility is a measured claim, and failing probes are evidence to classify, not signals to patch blindly.

## Ground rules for contributors

- Add or update a behavioral test before changing behavior; keep changes narrowly scoped.
- Keep existing syscall-range coverage; no GUI, game, or workload-specific shims in the core.
- Unsupported behavior fails closed and never fakes success.
- Prefer raw Linux syscalls in translation backends and translate flags, structures, results, and errno explicitly.

> **Speaker notes:** **We will grow coverage without losing the contract.** Start behavior changes with tests. Keep diffs narrow, preserve the syscall classifications, and avoid GUI- or game-specific behavior in the core. Translate the host ABI explicitly and fail closed on unsupported requests. [PAUSE] Those rules are what turn an ambitious compatibility layer into maintainable infrastructure.

## Where to look, and a good first contribution

- Read `README.md`, `docs/DOMAIN_MODEL.md`, `docs/SYSCALL_PATCHING.md`, `docs/DARWIN_SYSCALL_TABLE_STATUS.md`.
- Walk the three parts in order: `src/loader.c`, then `src/syscall/guest_vm_*.c`, then `src/syscall/syscall_gate.c`.
- Mach-O structure and constants: `src/macho_defs.h`; load-command walking: `src/loader.c` and `src/native_imports.c`.
- Diagrams live in `docs/diagrams/`, including `syscall-patching.svg` and `syscall-dispatch.svg`.
- Code entry points: `src/machgate.c`, `src/loader.c`, `src/syscall/`, `src/shim/`.
- A good first contribution: make the device profile selectable at the sysctl control point, test first.

> **Speaker notes:** **Contribution starts from the existing architecture.** Read the domain model, patching design, README, and syscall table tracker. Follow the loader, Guest VM, and gateway in that order. A practical first change is a selectable device profile with tests around current host-derived answers. [LONG PAUSE] The direction is clear: expand the useful command-line surface while keeping every boundary explicit and verifiable.
