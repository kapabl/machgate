# MachGate: Running ARM64 Mach-O Binaries on Linux

One gateway for ARM64 Mach-O guests — whether the binary came from a Mac or an iPhone.

## The problem: Darwin binaries don't speak Linux

- MachGate runs **ARM64 Mach-O command-line executables** inside **ARM64 Linux containers**
- The hard part: guest code issues **Darwin syscalls directly** with `svc #0x80`
- Linux has a different syscall table, different registers, different error conventions
- Statically linked runtimes never call an imported libSystem — there is nothing to intercept

> **Speaker notes:** Frame the problem before the solution. A macOS binary is machine code the CPU can run — and an iOS binary is the same Mach-O format running the same ARM64 instructions, which is why one gateway will serve both. What breaks is the boundary with the kernel: Darwin programs ask *Darwin* for services using the `svc #0x80` instruction, the `x16` register for the syscall number, and a carry-flag error convention. A Linux kernel understands none of that. Interposing libraries is not enough — some runtimes contain raw `svc #0x80` instructions compiled straight into their code. MachGate exists for exactly that case.

## Scope: deliberately narrow

**Supported today**

- ARM64 Mach-O guests on ARM64 Linux hosts
- Thin executables and ARM64 slices of fat binaries
- Command-line workloads
- Single guest thread is the **design target** — provisional host-backed pthread behavior exists where real runtimes need it; full thread semantics are out of scope

**Explicitly out of scope**

- x86-64 guests, graphical apps, Apple frameworks
- Mach IPC, general macOS compatibility

> **Speaker notes:** For management: the narrow scope is a feature, not a limitation — it is what makes the problem tractable and the behavior testable. One precision for developers: the single-thread constraint is the design target, not an enforced runtime boundary. The shim exposes host-backed pthread lifecycle — including pthread_create — plus spawn/wait/vfork, because real CLI runtimes and probes require those symbols; complete Darwin thread semantics remain out of scope and reconciling this is tracked in the domain model. GUI, framework, and Mach IPC behavior is different: those fail closed rather than pretend to succeed. When evaluating a new guest, check it against this list first.

## Not just macOS: an iOS binary is a Mach-O file

- iOS apps ship **ARM64 Mach-O executables** — the same container: `MH_EXECUTE`, `LC_SEGMENT_64`, `LC_MAIN`, chained binds
- MachGate validates the ARM64 slice and prepares it like any other guest — file origin does not change the format
- **In scope:** command-line-style binaries, static runtimes, ARM64 slices from universal/fat files
- **Fails closed:** binaries importing iOS-only frameworks (UIKit & friends) — policy, not accident

> **Speaker notes:** Key reframe: "Mach-O" does not mean "macOS only" — an iOS executable is the same file format with the same load commands, built against a different set of imported frameworks. MachGate does not care where the file came from; it validates structure and ABI. Whether a given iOS binary actually runs depends on its imports and syscalls being in scope: a CLI tool or statically linked runtime has a real chance, while anything touching UIKit, Metal, or Mach IPC fails closed by policy. External probes — pinned public ARM64 CLI binaries — are the compatibility evidence.

## Controlling what the guest believes: device profiles

- Every "what machine am I on?" question crosses the gateway: `sysctl hw.ncpu` / `hw.machine` / `hw.memsize`, `sysconf`, affinity queries
- Today MachGate answers with **real host facts** (`host_facts.c`, the sysctl translation in range 200–399)
- That path is a **single choke point** — a *device profile* can rewrite the answers: 2 cores, an iPhone model string, capped memory
- Optional enforcement: `sched_setaffinity` pins guest and threads to the claimed cores, so the story matches reality
- Use cases: device-QA matrices without hardware, reproducing "only on iPhone" behavior, simulator-style resource limits

> **Speaker notes:** This is a capability statement, so be precise about current state: today the translation reports the honest host answer — host_cpu_count() from host_facts.c feeds hw.ncpu, hw.activecpu, hw.logicalcpu, hw.physicalcpu, and machdep.cpu.core_count/thread_count in the 200–399 range dispatcher. But every one of those answers passes through MachGate, which makes it the natural place to answer differently on purpose. A device profile is a small, contained change: override the reported facts, then optionally enforce them with sched_setaffinity so thread counts and timing behave like the claimed device — the guest cannot tell the difference by asking the OS. That turns a server-class ARM64 box into a repeatable stand-in for a 2-core iPhone in tests.

## Core idea: patch before the guest runs

- MachGate never lets a Darwin syscall reach the Linux kernel
- Before guest entry, it **scans executable instruction sections**
- Every `svc #0x80` is replaced with a **direct branch to a nearby island**
- The island preserves guest state, calls the MachGate dispatcher, resumes the guest

> **Speaker notes:** This is the central design decision. Instead of trapping into the kernel and handling SIGSYS after the fact, MachGate rewrites the guest text once, ahead of time. By the time the guest runs, there are no Darwin syscalls left in it — only branches to code MachGate controls. Everything downstream is ordinary C code running on the host.

## Refresher: the Darwin ARM64 syscall ABI

```text
mov     x16, #DARWIN_SYS_write     ; syscall number
mov     x0, ...                    ; arguments in x0..x5
svc     #0x80                      ; opcode 0xd4001001

success:  x0 = result,   carry clear
failure:  x0 = Darwin errno, carry set
```

- Linux instead uses `x8` for the number, different flags and structs, negative errno
- Neither the numbers nor the structures agree — every value must be translated

> **Speaker notes:** Keep this slide short — it is reference material for the next three. Two conventions to remember: Darwin reports failure by *setting the carry flag* and returning a Darwin errno in x0, while Linux returns negative errno values. And syscall numbers with similar names are genuinely different operations with different argument layouts. Assuming the ABIs agree is the classic bug this project designs against.

## The patch: one instruction becomes a branch

```text
Before                                After
guest text at N                       guest text at N
+--------------------+                +--------------------+
| svc #0x80          |                | b syscall_island   | ---+
+--------------------+                +--------------------+    |
| guest continuation |  N + 4         | guest continuation |    |
+--------------------+                +--------------------+    |
                                                                v
                                    island: save x0..x18, x30, NZCV
                                            call dispatcher (blr)
                                            restore state + result
                                            br back to N + 4
```

- `b`, not `bl` — the guest link register `x30` must survive untouched
- The island saves **all promised registers and condition flags** before calling into C

> **Speaker notes:** The diagram is the heart of the talk. Each four-byte `svc` becomes a direct branch to a generated island. The island builds a state frame with x0 through x18, x30, NZCV, and the resume address N+4, then calls the C dispatcher. When the dispatcher returns, the island restores everything and branches to the continuation. Two details matter: using `b` keeps the guest link register intact, and the guest must be able to trust that its entire register file comes back exactly as it left it.

## Why branch islands? The ±128 MiB problem

- ARM64 instructions are fixed 4 bytes; a direct `b` reaches about **±128 MiB**
- Guest code and MachGate's dispatcher may be mapped far apart
- Solution: **executable island pools allocated near the guest image**, one island per patched site
- Invariant: every patched branch reaches its island without changing guest `x30`

> **Speaker notes:** A short architecture history lesson. Direct branches are fast but short-range. MachGate therefore places a pool of executable islands within range of the guest's code and emits one island per syscall site — each patched branch is guaranteed to reach. This turns a global addressing problem into a local allocation one. The invariant is checked, not hoped for: any site that could not reach an island is a construction failure, not a runtime surprise.

## Inside the dispatcher: explicit translation, always

- `syscall_gate_dispatch(struct syscall_gate_state *)` — plain C, on the host
- Per syscall: translate **number, arguments, flags, structures, result, errno**
- Then select a Linux operation; backends use the raw Linux ABI (`x8` / `SYS_*`)
- Unknown numbers: report guest PC and argument registers in diagnostic mode, return **Darwin `ENOSYS`**

> **Speaker notes:** The dispatcher is where the two worlds meet, and the rule is that nothing crosses it untranslated — no casting a Darwin struct to a Linux struct because the names look similar. Every flag, layout, and error value is converted explicitly. Unsupported syscalls are honest: they return Darwin ENOSYS rather than a fake success, so guests fail the way they would on a machine lacking the feature.

## Coverage today: the syscall table

- Source of truth: Apple xnu `syscalls.master`
- **669 / 669** source rows classified — zero pending, zero misclassified
- **455 translated** (68%) · **214 explicit Darwin `ENOSYS`** (32%)
- Four range dispatchers: `0–99`, `100–199`, `200–399`, `400+`

> **Speaker notes:** Management framing: every row of the Darwin syscall table has a known disposition — there is no unknown territory. Two thirds execute for real on Linux; the rest fail explicitly with the Darwin error a guest expects for an unavailable feature. That predictability is what makes debugging a guest possible: behavior is either translated or honestly absent, never accidentally wrong.

## Beyond syscalls: loading and native linkage

- **Executable Imaging** — thin/fat selection, validation, segment mapping, entry point (`LC_MAIN` / `LC_UNIXTHREAD`)
- **Native Linkage** — classic and chained import binding to an *explicitly selected* native provider
- Provider decisions: `Unmapped`, `Map`, `Skip`, `Stub`, `Partial` — each fails closed
- **Compatibility Runtime** — a native libSystem shim: pthread sync, file ops, TLV, variadic ABI, unwind, more

> **Speaker notes:** Syscalls are only half the story. The loader maps a validated Guest Image or nothing at all — no partially mapped failures. Dynamic guests bind imports to a native provider MachGate chose on purpose, with five distinct policies; the danger case is a host library that happens to share a symbol name, and the lookup order prevents exactly that. The shim is an anti-corruption layer: Darwin names on the outside, Linux facilities on the inside, no Apple frameworks implied.

## Architecture: bounded contexts with a dependency rule

- Core context: **Darwin ABI Translation**
- Supporting: Executable Imaging · Native Linkage · Guest Execution · Compatibility Runtime · Guest VM (virtual memory) · Process Continuity
- Layers: **Domain → Application → Ports → Adapters** — adapters depend inward, never outward
- Shared ubiquitous language: Guest, Guest Image, Syscall Site, Branch Island, Syscall Outcome

> **Speaker notes:** This is how the codebase stays manageable while emulating an ABI. The domain language in docs/DOMAIN_MODEL.md is authoritative — source names, tests, and diagnostics use it. The layering rule is the practical takeaway for contributors: Mach-O parsing and Linux syscalls live in adapters; the core talks about Guests and Syscall Outcomes, not about Linux structures. Structural refactoring and behavior changes are never mixed in one change.

## Guest VM (virtual memory) and Process Continuity

- **"VM" = virtual memory, not virtual machine** — no hypervisor, no guest kernel; guest instructions execute natively and share the host address space
- **Guest VM** — one tracked Guest Address Space; guest `mmap`/`munmap`/`mprotect` update the ledger
- Host mappings that must participate are brought in by the **VM interposer** (`libmachgate_vm_interpose.so`)
- **Process Continuity** — guest `execve` becomes *Process Replacement*: MachGate relaunches itself with the new Mach-O target
- Darwin errors on failure; success never returns

> **Speaker notes:** Terminology first, because it trips people: "VM" here means virtual memory — the subsystem behind Darwin's `mach_vm_*` calls — not a virtual machine. There is no hypervisor and no second kernel; guest instructions run natively on the CPU and the only mediated boundary is the ABI. The name survives because the translated operations are literally Darwin's VM syscalls. With that clear: Guest VM keeps a single model of the guest's address space so that memory syscalls and interposed host allocations agree — a mapping is either tracked or it is not part of the guest. Process Continuity handles guests that re-exec: since Linux cannot exec Mach-O bytes, MachGate reconstructs the launch under its own loader and reports Darwin errors if the target is unsupported.

## How we know it works: behavioral testing

- **~50 behavior test suites** in `tests/`, **137 fixture sources** in `tests/fixtures/`
- Coverage: thin/fat loading, entry points, static initializers, **all four syscall ranges**, VM ops, re-execution, shim surface, allocator hooks, classic and chained imports, trampolines
- **External probes**: pinned public ARM64 Mach-O CLIs run as compatibility evidence
- Build + test: `cmake` → `build_fixtures.sh` → `run_tests.sh`

> **Speaker notes:** Management framing: claims of compatibility are backed by executable evidence, not demos. The fixture builder compiles real ARM64 Mach-O programs with an LLVM toolchain; the suites then assert on guest behavior end to end. External probes are pinned third-party binaries — when one fails, the failure is classified by bounded context before anyone writes a fix. A behavior change without a test is not a completed change.

## Current status and honest boundaries

**Working:** static and dynamic CLI guests, the syscall gateway, VM tracking, process replacement, the shim surface

**Known gaps:** the single-thread/no-child target is not yet enforced (provisional host-backed pthread/spawn behavior), pthread identity/keys/once extraction, remaining raw Mach-O walks outside Executable Imaging, explicit Guest VM capacity/error modeling

**Invariant to keep:** unsupported behavior must stay fail-closed — never fake success

> **Speaker notes:** The honest version for both audiences. The core paths guests actually exercise are implemented and tested. The remaining work is structural hardening listed in the domain model's extraction sequence — shrinking the monolithic shim, narrowing raw Mach-O access to the imaging context, making VM error states explicit. The standing rule: a gap may fail loudly, but it must not masquerade as success.

## Ground rules for contributors

- **Clean-room boundary**: implement from Apple open-source references, LLVM docs, public ABIs, and behavior tests — never from the legacy checkout
- **Behavior change requires a test first**
- Narrowly scoped changes; keep syscall-range coverage intact
- No GUI, game, or workload-specific shims in the core

> **Speaker notes:** The contribution contract in one slide. The clean-room rule is an engineering boundary: the sibling checkout may classify provenance, but no implementation text or behavior spec comes from it. Tests come before behavior changes so the suite always describes the system we actually have. And scope discipline protects the core: anything graphical or workload-specific stays out, full stop.

## Where to look

- `README.md` — build, run, repository layout
- `docs/SYSCALL_PATCHING.md` — the gateway and island design
- `docs/DARWIN_SYSCALL_TABLE_STATUS.md` — per-syscall coverage tracker
- `docs/DOMAIN_MODEL.md` — ubiquitous language, contexts, invariants
- `docs/CLEAN_ROOM_SIMILARITY_AUDIT.md`, `docs/EXTERNAL_CORPUS_STATUS.md` — evidence base

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
bash tests/fixtures/build_fixtures.sh && bash tests/run_tests.sh
build/machgate path/to/program
```

> **Speaker notes:** Close with the pointers and the four commands. Everything claimed in this deck has a document or a test behind it — start with the README, then the syscall patching doc for the design, then pick a test suite and read it against the domain model. Questions welcome.
