# MachGate

## Apple command-line programs, running on plain Linux — no Mac required

- MachGate runs Apple command-line programs inside ordinary Linux containers
- Modern Macs and most Linux servers speak the same chip language
- So the program runs natively, at full speed
- No emulator, no virtual machine, no second operating system
- We translate only where the program talks to the operating system

> **Speaker notes:** Welcome. MachGate runs Apple command-line programs inside ordinary Linux containers. The key point is that modern Macs and most Linux servers speak the same chip language, so we are not emulating anything — the program executes at full speed. What we do translate is the conversation between the program and the operating system, because Apple and Linux use different words and return different answers there. Over the next few slides I will show why that gap exists, how we close it, what works today, and what deliberately does not. Please ask questions as we go.

## Why a Mac program won't run on a Linux server

- A program is just instructions, and the chip language is the same
- But programs constantly stop and ask the operating system for help
- Open this file, read these bytes, give me memory
- Apple programs ask Apple's kernel, using Apple's vocabulary
- Linux expects different wording and hands back answers in a different shape

> **Speaker notes:** Here is the puzzle. A program is just a long list of instructions, and those instructions are portable across machines with the same chip. So why does the program fail on a Linux server? Because programs constantly stop and ask the operating system for favors: open this file, read these bytes, give me memory. Apple programs ask Apple's kernel in Apple's phrasing. Linux uses different phrasing and returns results in a different shape. Installing substitute libraries only helps sometimes, because statically linked programs bake those requests directly into their own code, where no library swap can reach them.

## We don't need Macs to test our engine

- Our tests are command-line programs that exercise engine code directly
- No screen, no Apple frameworks, no copy of macOS needed
- Even the Apple-format test programs are built on Linux
- Apple hardware means cloud Macs billed hourly, with long minimum bookings
- Linux containers cost less, start in seconds, and run many jobs in parallel

> **Speaker notes:** This slide is about cost, and it is the reason the project is practical. Our engine tests are plain command-line programs, so they need no screen, no Apple frameworks, and no copy of macOS. We even build the Apple-format test programs on Linux using a standard toolchain. Running all of that on Apple hardware would mean a Mac fleet or cloud Macs billed by the hour with long minimum allocations. ARM64 Linux containers cost a fraction per hour, start in seconds, and scale out across many parallel jobs. Same behavioral coverage, no Apple hardware in the loop, far more test iterations per dollar. I am keeping this qualitative on purpose.

## The idea: rewrite every OS request before the program starts

- MachGate reads the program file and finds every OS request
- Each one is replaced with a small jump into MachGate's own code
- That code answers using the Linux equivalent, then resumes the program
- Nothing else is emulated; only the OS boundary is translated
- Unsupported requests get the true "unsupported" answer, never fake success

*Figure: `docs/diagrams/syscall-patching.svg`*

> **Speaker notes:** This is the central idea, and the diagram makes it concrete. Before the program runs at all, MachGate reads it and finds every place it would ask the operating system for something. Each of those requests is replaced with a small jump into code we control. Our code works out the Linux equivalent, produces the answer the program expects, and jumps straight back to where the program left off. The program never notices anything unusual. Nothing is emulated instruction by instruction. And when we cannot honor a request, we return the genuine not-supported answer rather than pretending it worked.

## How it works, end to end

- Validate the file, choose the right part of it
- Map it into memory, checking every range before use
- Rewrite every OS request, prepare the program's memory
- Set up arguments, environment, and its starting location
- Hand over control; answers keep flowing through MachGate

*Figure: `docs/diagrams/guest-loading.svg`*

> **Speaker notes:** Now the whole journey, in order. First we validate the file and select the correct portion of it, including the Apple-chip slice of a universal file. Then we map it into memory, checking every address and length before we trust it. Next we locate and rewrite each operating-system request and prepare the program's memory and starting environment. Finally we hand over control, and the program runs, with every operating-system answer still routed through our translation layer. That ordering matters and is part of our contract: preparation always finishes before the program's first instruction executes.

## What runs today

- Command-line tools and statically linked runtimes
- The Apple-chip portion of universal (multi-chip) program files
- Origin doesn't matter: iOS and macOS command-line files share one format
- If a program only asks for things we translate, it has a real chance

> **Speaker notes:** So what works today? Command-line tools, statically linked runtimes, and the Apple-chip slice of universal binaries. Here is the part people find surprising: it does not matter whether the file came from macOS or iOS. Both are the same file format, just built against different libraries. So origin is not the question — the question is what the program asks for. If it only asks for things we translate, it has a real chance of running. Rather than describe it, let me show you: I will run one of our small test programs now, and you will see it start, do its work, and exit cleanly on this Linux machine.

## Honest limits

- No graphical apps, no Apple user-interface frameworks, no GPU
- No Apple inter-process messaging; those requests fail with a clear error
- Designed around a single running thread, not heavy multi-threading
- Unsupported requests always return the true "unsupported" answer
- Failures are therefore diagnosable, not mysterious

> **Speaker notes:** Honesty is the differentiator here, so I will be blunt. We do not run graphical applications, Apple user-interface frameworks, or anything that needs the graphics processor. We do not support Apple's inter-process messaging; those requests fail with a clear error. The design targets a single running thread rather than heavy multi-threading. And when we meet something unsupported, we return the true unsupported answer instead of faking success. That choice is deliberate. A faked success produces mysterious, hard-to-diagnose breakage; an honest error tells you exactly which capability is missing and where the boundary is.

## Why not Rosetta, QEMU, or a virtual machine?

- Rosetta translates Intel instructions into Apple-chip instructions
- QEMU emulates whole instruction sets in software
- A virtual machine runs a second operating system under a supervisor
- MachGate runs the program itself, on the same kind of chip
- Only the OS boundary is translated; it is just a normal container process

> **Speaker notes:** People reasonably ask how this differs from the well-known alternatives. Rosetta translates Intel instructions into Apple-chip instructions. QEMU emulates an entire instruction set in software. A virtual machine runs a second operating system under a supervisor. MachGate does none of those. The program runs natively on the same kind of chip, so there is no instruction emulation and no performance tax from that. The only thing we translate is the boundary with the operating system. There is no hypervisor and no second kernel. It runs as a normal Linux process, inside a normal container.

## Evidence, not demos

- About 50 behavior test suites exercise the engine end to end
- 137 small test programs cover loading, memory, and OS requests
- Pinned public third-party Apple command-line programs act as outside checks
- 669 of 669 OS requests classified from Apple's published table
- 455 (68%) genuinely translated; 214 (32%) return the honest "unsupported" answer

> **Speaker notes:** Claims should be backed by executable evidence, so here is ours. About fifty behavior suites exercise the engine end to end, and 137 small test programs cover loading, memory, and operating-system requests. We also run pinned public third-party Apple command-line programs as outside compatibility checks. On coverage: every one of the 669 rows in Apple's published request table is classified, with none left pending. 455 of them, about 68 percent, are genuinely translated into working Linux operations. The remaining 214, about 32 percent, return the honest unsupported answer. That is a real boundary we can measure.

## Status and what is next

- Works today: loading, OS request translation, memory, self-restart
- Provisional: thread behavior and child processes that real tools need
- Planned: a "device profile" that answers as a smaller device
- Example: report 2 cores and an iPhone model string, then match that limit
- Not shipped yet — a small, contained change at one control point
- Today the core count and memory are the real host's; model strings are fixed stand-ins

*Figure: `docs/diagrams/device-simulation.svg`*

> **Speaker notes:** Where are we? Loading, request translation, memory handling, and self-restart all work today. Thread behavior and child processes are provisional: real command-line tools need those, and we are reconciling that with the single-thread design target. Planned, and clearly not shipped yet, is the device-profile idea. Because every question like "how many cores exist?" passes through one control point, we could make the answers selectable — say two cores and an iPhone model string — and optionally restrict the process to match. Today the core count and memory come from the real machine, and the model and version strings are already fixed stand-ins rather than real hardware. The profile is a small, contained change, but it does not exist yet.

## Try it, and where to read more

- Build it, build the test programs, run the full suite
- Then point it at any Apple ARM64 command-line program
- Licensed GPL-3.0-or-later, with provenance recorded in the attribution file
- Architecture and design notes live in the documentation folder

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
bash tests/fixtures/build_fixtures.sh
bash tests/run_tests.sh
build/machgate path/to/program
```

> **Speaker notes:** If you want to try it, these commands build the project, build the test programs, and run the full suite on any ARM64 Linux machine. Then point it at any Apple ARM64 command-line program you happen to have. The project is licensed under GPL-3.0-or-later, and the attribution file records where every piece of knowledge came from, because the implementation is a clean-room rewrite built on public format and ABI references. The architecture and design notes live in the documentation folder. Thank you. I am happy to take questions, especially about the parts that deliberately do not work yet.
