# MachGate: Running Apple Command-Line Programs on Ordinary Linux

*A compatibility layer that runs Apple command-line software directly on Linux — validated with real behavior tests, and with no Mac anywhere in the loop.*

> **Speaker notes:** **We can run Apple command-line test workloads on ordinary arm64 Linux hardware.** [PAUSE] MachGate makes that possible without a Mac, emulator, or guest operating system. The evidence is already specific: a corpus of known macOS command-line programs runs under it, Bitcoin's unit tests pass under it, and more than 99% of our engine's macOS and iOS test cases pass. Today I want to establish the value, the boundary, and the investment needed to scale it.

## What MachGate Is, in Plain Language

- It runs Apple command-line programs directly on an ARM64 Linux machine.
- Both sides use the same processor instruction set, so code runs natively.
- Analogy: a translator at the counter where software asks the OS for help.
- The two operating systems use a different request format and different answers.
- MachGate translates those requests; it is not an emulator, so there is no slowdown.

> **Speaker notes:** **The computation is already compatible; the operating-system interface is not.** Apple and Linux on arm64 use the same processor instructions. MachGate translates the requests a program makes for files, memory, and other OS services. [PAUSE] We do not emulate the application. We translate the boundary around it, which is what makes this approach fundamentally different.

## The Cost Problem: Apple Hardware for Tests That Don't Need It

- Testing is assumed to require Apple machines; that assumption carries cost.
- Purchase: Macs are expensive to buy, and they age out.
- Rental: cloud macOS capacity from suppliers is expensive as well.
- Running: Apple machines need images, updates, spares, and upkeep.
- Capacity: Apple hardware is harder to scale, so it caps how much we can test.
- Our engine tests are command-line only, so that spend buys nothing we use.

> **Speaker notes:** **We are spending on Apple-specific capacity for tests that do not use Apple-specific features.** Macs cost more to buy, rent, refresh, and operate, and they are harder to scale at the rate we want. For graphical or device-level tests, that cost is justified. For these command-line engine tests, it is not. [PAUSE] The opportunity is to match the infrastructure to the workload.

## The Economics of Our Tests Specifically

- Our tests exercise engine code; they never need a GUI or macOS.
- Acquisition cost falls: nothing to buy, nothing to refresh, nothing to rent.
- Running cost falls: general-purpose machines are cheaper to operate at scale.
- Throughput rises: general-purpose machines scale out, so the same outlay buys more testing.
- Faster feedback is the economic gain, not only a smaller invoice.
- Apple hardware stays only where it is genuinely required.

> **Speaker notes:** **The real gain is more developer feedback per dollar.** Moving eligible tests to Linux reduces dependence on scarce macOS workers and lets us scale execution horizontally. That means more concurrent runs and shorter feedback cycles as capacity grows. [PAUSE] This does not eliminate all Apple testing. It keeps Apple hardware focused on the parts of the product that actually need it.

## Why the Technical Problem Is Real

- Programs request operating system services through "system calls."
- Apple programs ask Apple's kernel, in Apple's own request format.
- Linux's kernel expects a different format and returns different answers.
- Adding a compatibility library alone is not enough to bridge this.
- Statically linked programs embed these requests directly in their machine code.
- MachGate rewrites those requests before the program ever runs.

> **Speaker notes:** **This required solving a real compatibility problem, not just moving files.** Apple and Linux disagree on how programs issue operating-system requests and receive results. Statically linked programs may make those requests directly, beyond the reach of a replacement library. MachGate rewrites those call sites before execution. [PAUSE] That creates a reusable mechanism rather than a per-test workaround.

## Scope and Risk Management: Fail Closed, Never Fake Success

- Deliberately out: graphical applications and Apple's interface frameworks.
- Also out: Apple's inter-process messaging, and 64-bit Intel programs.
- Out of scope too: general-purpose macOS application compatibility.
- Unsupported requests return an honest "not available" answer, never a fake success.
- Narrow scope is what makes behavior testable and keeps risk bounded.

*Figure: `docs/diagrams/host-components.svg`*

> **Speaker notes:** **Risk is controlled by narrowing the promise.** MachGate targets command-line binaries, not general macOS applications, graphics, Apple frameworks, or Mach IPC. Unsupported behavior returns an explicit error instead of a false success. [PAUSE] That last rule matters: our test infrastructure must never manufacture a passing result by hiding missing functionality.

## How It Works, in Plain Terms

- The program file is validated: a plain file or the ARM64 part of a universal one.
- Its memory segments are placed at the addresses the program requests.
- Requests to the operating system are rewritten to their Linux equivalents beforehand.
- The program then starts and runs natively on the processor.
- It is all-or-nothing: a failed load leaves nothing half-finished behind.

*Figure: `docs/diagrams/guest-loading.svg`*

> **Speaker notes:** **Execution follows a validated, ordered pipeline.** We inspect the binary, select the arm64 code, map its memory, prepare its runtime, and redirect operating-system requests before starting it. A failed load leaves no partial image. [PAUSE] The architecture is designed so unsupported or malformed inputs fail predictably, rather than contaminating later execution.

## Quality and Evidence: Claims Backed by Tests, Not Demos

- 48 behavior test suites, 137 fixture files, and 57 pinned public probes back every claim.
- The fixture programs themselves are produced on Linux; no Xcode and no Mac are involved.
- A corpus of known macOS command-line programs runs under MachGate; Bitcoin's unit tests pass.
- More than 99% of our engine's macOS and iOS test cases pass under MachGate.
- All 669 operating system requests are classified — none left ambiguous.
- 455 of those (68%) are translated; the rest return a clear "not supported" answer.
- A behavior change is not considered complete until a test proves it.

> **Speaker notes:** **The evidence is specific, and I will state it precisely.** More than 99% of our engine's macOS and iOS test cases pass under MachGate, and Bitcoin's unit tests pass under it. Behind that sit 48 behavior suites, 137 fixture files, and 57 pinned external probes — a corpus of known macOS command-line programs that runs end to end. All 669 entries in the published syscall source table are classified: 455 translated and 214 explicitly unsupported. [PAUSE] The numbers distinguish what works from what we intentionally do not support, and none of them are rounded up.

## Status: Proven by Evidence

- Runs static and dynamic command-line programs through the request translation gateway.
- Provides memory tracking, program replacement, and the native compatibility library.
- Threads and child processes come from the host runtime, not from a MachGate thread model.
- A corpus of known macOS command-line programs runs under MachGate.
- Bitcoin's unit tests pass, and more than 99% of our engine's macOS and iOS test cases pass.

> **Speaker notes:** **What is proven, stated exactly.** The paths that real command-line programs actually exercise are implemented and tested: the request translation gateway, memory tracking, program replacement, and the native compatibility library. Threads and child processes come from the host operating system, so a program that uses them gets real threads and real processes. [PAUSE] The evidence is specific: a corpus of known macOS command-line programs runs under MachGate, Bitcoin's unit tests pass under it, and more than 99% of our engine's macOS and iOS test cases pass. I choose those words deliberately — this is validated software, and I will not describe it as more than that evidence shows. [PAUSE] That discipline is why the numbers can be trusted.

## Device Profiles: Testing for Smaller Devices Without Owning Them

- Programs constantly ask the machine "how many CPU cores? what model am I?"
- Every such question passes through MachGate's single translation point.
- Restricting how many cores the process may use already changes the answer the guest gets.
- So a commodity machine can already stand in for a smaller device.
- One named profile could also set the model and the memory size at the same control point.

*Figure: `docs/diagrams/device-simulation.svg`*

> **Speaker notes:** **The same architecture also lets us control the machine a test sees.** Affinity-based CPU answers already respond to process restrictions, so we can exercise smaller parallelism configurations on commodity hardware. A single named profile covering cores, model, and memory is a potential extension, not a delivered feature. [PAUSE] That creates a path toward more representative testing without multiplying hardware types.

## What This Enables, and What It Takes to Run

- Validate Apple command-line software on commodity ARM64 Linux build machines.
- No Apple hardware in the loop, and more test iterations for the same outlay.
- Some Apple-only testing stays necessary for the deliberately out-of-scope areas.
- Operating it takes engineering time to keep coverage current as the engine evolves.
- Operating it takes continued ARM64 Linux CI capacity and upkeep of the external probes.

> **Speaker notes:** **The direction is to make Linux arm64 a dependable execution lane for eligible Apple workloads.** Keep Apple machines for the tests that require their platform. Operate MachGate with engineering ownership, CI capacity, and continuously maintained external probes. [PAUSE] This is infrastructure to sustain and expand, not a research experiment to restart.

## The Decision Requested

- Approve continued investment in MachGate.
- Fund ARM64 Linux CI capacity; no Apple hardware is required.
- Endorse "fail closed" as a non-negotiable engineering rule.
- Confirm the out-of-scope boundaries: graphics, frameworks, Apple messaging.
- Decision: provide the team time and CI budget to scale it.

> **Speaker notes:** **The decision is to invest in scaling a working capability.** Provide engineering ownership and arm64 Linux CI budget. Keep fail-closed behavior as the engineering rule, and keep graphics, frameworks, and Apple messaging outside scope. [LONG PAUSE] We have demonstrated the execution model. Now we should turn that advantage into repeatable test capacity.
