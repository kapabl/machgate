# MachGate: Running Apple Command-Line Programs on Ordinary Linux

*A compatibility layer that runs Apple command-line software in Linux containers — validated with real behavior tests, and with no Mac anywhere in the loop.*

> **Speaker notes:** Thank you for the time. This briefing is for engineering leadership, not for specialists, so I will stay in plain language and focus on three things: what MachGate is, what it costs to validate, and the one decision I need from you. The headline is that we can run and test Apple format command-line software on cheap, ordinary Linux hardware, and every claim is backed by an executable test rather than a demo. I will be equally clear about what is out of scope and what is not finished yet, because that honesty is what makes the numbers trustworthy.

## What MachGate Is, in Plain Language

- It runs Apple command-line programs inside ordinary Linux containers.
- Both sides use the same processor instruction set, so code runs natively.
- Analogy: a translator at the counter where software asks the OS for help.
- The two operating systems use a different request format and different answers.
- MachGate translates those requests; it is not an emulator, so there is no slowdown.

> **Speaker notes:** One analogy carries this whole slide. A program does not do everything itself; it regularly asks the operating system for services — open a file, read it, write output, reserve memory. Those requests have a specific format tied to the operating system. Apple programs speak Apple's format; Linux expects a different one. MachGate sits at exactly that boundary and translates, the way a translator sits between two people who share the same subject but not the same vocabulary. Because both machines run the same processor instructions, the program itself runs at full speed. We only translate the conversation, never the computation.

## The Cost Problem: Apple Hardware for Tests That Don't Need It

- Compatibility testing today is assumed to require Apple machines.
- Buying a Mac mini per developer is high up-front cost.
- Renting cloud Macs bills by the hour with long minimum allocations.
- Apple hardware is genuinely required for graphics and app work.
- Our engine tests are command-line only, so that cost buys us nothing.

*Placeholder: (insert our current CI cost figures)*

> **Speaker notes:** This is the slide that matters most for this audience, so I want to be precise. The usual assumption is that testing Apple software requires Apple hardware. That assumption is expensive: buying a Mac mini per developer is a capital cost, and renting cloud Macs is a recurring cost billed by the hour, often with long minimum allocation windows and queues. All of that is legitimate when the work involves graphics, user interfaces, or device-specific behavior. But our engine is a command-line tool, and its tests are command-line programs. We are paying Apple prices to run tests that never touch anything Apple-specific. That is the waste we are removing.

## The Economics of Our Tests Specifically

- Engine tests are pure command-line programs that exercise engine code.
- They need no graphical interface, no Apple frameworks, no full macOS install.
- Even our Apple-format test fixtures are built on Linux with an LLVM toolchain.
- ARM64 Linux containers cost a fraction per compute hour and start in seconds.
- Same instruction set on both sides means real code paths, not emulation.
- Containers scale out in parallel CI, so we get many more iterations per dollar.

*Placeholder: (insert our current CI cost figures)*

> **Speaker notes:** Let me make the economics concrete and defensible. Our tests are command-line programs that exercise our own engine. They need no graphical interface, no Apple frameworks, and no full macOS installation. Even the test fixtures, which are genuine Apple format files, are produced on Linux using an open LLVM toolchain — no Xcode and no Mac required. Running them on ARM64 Linux containers costs a fraction of a rented Mac per compute hour, starts in seconds, and scales out across parallel build machines. Because both sides share the same instruction set, the real code paths run natively rather than being approximated by emulation. The net effect is identical behavioral coverage, no Apple hardware, and far more test iterations for the same budget. I have left the exact figures as a placeholder to be filled from our current CI billing.

## Why the Technical Problem Is Real

- Programs request operating system services through "system calls."
- Apple programs ask Apple's kernel, in Apple's own request format.
- Linux's kernel expects a different format and returns different answers.
- Adding a compatibility library alone is not enough to bridge this.
- Statically linked programs embed these requests directly in their machine code.
- MachGate rewrites those requests before the program ever runs.

> **Speaker notes:** The reason this is hard, and not just a packaging exercise, is that the two operating systems genuinely disagree about how a program asks for a service. The request format is different, the meaning of the arguments is different, and the way success or failure is reported is different. The obvious fix is to drop in a compatibility library, and that works only when the program routes its requests through that library. Many real command-line programs do not: they are statically linked, meaning the requests are baked directly into the program's own machine code, with nothing between them and the operating system. There is no library to intercept. MachGate solves that by rewriting those requests in the program itself before it starts running.

## Scope and Risk Management: Fail Closed, Never Fake Success

- Deliberately out: graphical applications and Apple's interface frameworks.
- Also out: Apple's inter-process messaging, and 64-bit Intel programs.
- Out of scope too: general-purpose macOS application compatibility.
- Unsupported requests return an honest "not available" answer, never a fake success.
- Narrow scope is what makes behavior testable and keeps risk bounded.
- Source is licensed GPL-3.0-or-later under a documented clean-room boundary.

*Figure: `docs/diagrams/host-components.svg`*

> **Speaker notes:** Scope control is a risk management decision, and I want it stated plainly. We do not attempt graphical applications, Apple's interface frameworks, Apple's inter-process messaging, 64-bit Intel programs, or general macOS application compatibility. Those areas are where the cost and uncertainty live, and they are not where our value is. The second half of this slide is the credibility asset: when a program asks for something we do not support, we return an honest "not available" answer rather than pretending it worked. That single rule, never fake success, is what lets you trust the test results. The project is open source under the GPL and is being rebuilt on a documented clean-room basis, deriving from public references and our own tests.

## How It Works, in Plain Terms

- The program file is validated: a plain file or the ARM64 part of a universal one.
- Its memory segments are placed at the addresses the program requests.
- Requests to the operating system are rewritten to their Linux equivalents beforehand.
- The program then starts and runs natively on the processor.
- It is all-or-nothing: a failed load leaves nothing half-finished behind.

*Figure: `docs/diagrams/guest-loading.svg`*

> **Speaker notes:** Here is the whole pipeline without the technical detail. First we validate the incoming program file. If it is a universal file containing several processor variants, we pick the ARM64 one. Then we place its memory segments at the addresses the program itself requests, choose its starting point, set up a protected stack, and only then let it begin. Before it runs, we have already rewritten its operating system requests into Linux equivalents. The important design property is that loading is all-or-nothing: a usable program image exists only after the entire file validates and maps. If anything fails, we leave nothing half-done, because a half-loaded program is the most dangerous kind of silent failure.

## Quality and Evidence: Claims Backed by Tests, Not Demos

- About 50 behavior test suites back the claims made here.
- 137 test fixture programs, plus pinned public third-party binaries as external probes.
- All 669 operating system requests are classified — none left ambiguous.
- 455 of those (68%) are translated to working Linux operations.
- The other 214 (32%) return an honest "not supported" answer.
- A behavior change is not considered complete until a test proves it.

> **Speaker notes:** This slide is the evidence base, and I want to be precise about the numbers. Roughly fifty behavior test suites exercise the engine end to end, supported by 137 test fixture programs, some of which are pinned public third-party binaries run as external compatibility probes. On coverage: the source is Apple's own published list of operating system requests. We have classified all 669 of them — zero unknown, zero misclassified. Of those, 455, or 68 percent, are translated into working Linux operations. The remaining 214, or 32 percent, return the honest "not supported" answer a program expects for a feature that is unavailable. Nothing is left ambiguous. And by policy, a behavior change is not complete until a test proves it.

## Honest Status: What Works, What's Provisional, What's Next

- Works today: static and dynamic command-line programs, and the request translation gateway.
- Works today: memory tracking, program restart, and the native compatibility library.
- Provisional: the single-thread, no-child-process design target is not yet enforced.
- Real command-line programs need some multi-threaded behavior, so we host-back it for now.
- Remaining work is structural hardening, tracked in the project's domain model.

> **Speaker notes:** This is the honest status slide, and I would rather you hear it from me. The paths that real command-line programs actually exercise are implemented and tested: the request translation gateway, memory tracking, program restart, and the native compatibility library that supplies the Apple runtime functions command-line tools use. One item is provisional: our stated design target is a single thread of execution with no child processes, but that is not yet enforced. Real command-line runtimes and our external probes need some multi-threaded behavior, so we currently host-back it rather than claim we support it. Complete thread semantics are deliberately out of scope. What remains is structural hardening, listed in the project's domain model.

## Device Profiles: Testing for Smaller Devices Without Owning Them

- Programs constantly ask the machine "how many CPU cores? what model am I?"
- Every such question passes through MachGate's single translation point.
- Today it reports the host's real CPU count and memory.
- A "device profile" could answer as a smaller device on purpose — capability, not yet shipped.
- The override is a small, contained change and can optionally cap the process.

*Figure: `docs/diagrams/device-simulation.svg`*

> **Speaker notes:** This is a capability statement, so I will mark current state clearly. Programs constantly interrogate their host: how many processor cores, how much memory, what model is this. Those questions already flow through MachGate's single translation point. Today the core count and memory are the real host's, while the model and version strings are already fixed stand-ins. Because that path is a single choke point, we could make those answers selectable and deliberately describe a smaller device — say two cores and a phone-style model string — and optionally enforce it by restricting the process to that many cores, so measured behavior matches the claimed device. That selectable override does not exist yet. It is a small, contained change, and it would let a commodity server stand in for a device we do not own.

## What This Enables, and What It Needs

- Validate Apple command-line software on commodity ARM64 Linux build machines.
- No Apple hardware in the loop, and many more test iterations per dollar.
- Some Apple-only testing stays necessary for the deliberately out-of-scope areas.
- Needs: dedicated engineering time to finish the structural hardening.
- Needs: continued ARM64 Linux CI capacity and upkeep of the external probes.

> **Speaker notes:** Pulling it together: the practical payoff is that we can build, run, and validate Apple format command-line software on ordinary ARM64 Linux machines, with no Apple hardware involved and far more iterations per dollar. I want to be honest that this does not eliminate every Apple dependency — work in the deliberately out-of-scope areas still needs Apple hardware, and we should keep that budget. What MachGate needs to finish is modest: dedicated engineering time for the structural hardening already listed, continued CI capacity on ARM64 Linux, and upkeep of the external compatibility probes that keep our claims honest. This is a resourcing conversation more than a research one.

## Next Steps and the Decision Requested

- Approve continued investment in hardening the clean-room rewrite.
- Fund ARM64 Linux CI capacity; no Apple hardware is required.
- Endorse "fail closed" as a non-negotiable engineering rule.
- Confirm the out-of-scope boundaries: graphics, frameworks, Apple messaging.
- Decision: provide the team time and CI budget to complete hardening.

> **Speaker notes:** I will close with the specific decision I am asking for. First, approve continued investment in hardening the rewrite rather than leaving it as retained technical risk. Second, fund ARM64 Linux CI capacity, which is the cheap part and needs no Apple hardware. Third, formally endorse "fail closed" as a non-negotiable rule, because it is what makes our results trustworthy. Fourth, confirm the out-of-scope boundaries so we do not drift into graphical or framework work. In one line: give the team the time and the CI budget to finish hardening, and we can hand you a validated Apple command-line compatibility layer running entirely on commodity Linux.
