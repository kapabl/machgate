# MachGate

## Apple command-line programs, running on plain Linux — no Mac required

- MachGate runs Apple command-line programs directly on an ARM64 Linux machine
- Modern Macs and most Linux machines speak the same chip language
- So the program runs natively, at full speed
- No emulator, no virtual machine, no second operating system
- We translate only where the program talks to the operating system

> **Speaker notes:** **What if an Apple program could run on Linux without a Mac?** [PAUSE] That is what MachGate does for arm64 command-line software. No emulator. No virtual machine. No second operating system. The program's instructions run on the Linux processor; MachGate translates the requests it makes to the operating system. [PAUSE] That's the idea I'll show you today.

## Why a Mac program won't run on a Linux machine

- A program is just instructions, and the chip language is the same
- But programs constantly stop and ask the operating system for help
- Open this file, read these bytes, give me memory
- Apple programs ask Apple's kernel, using Apple's vocabulary
- Linux expects different wording and hands back answers in a different shape

> **Speaker notes:** **The surprising part is that the processor isn't the problem.** Both machines can understand the same instructions. The problem begins when the program asks for a file, memory, or another operating-system service. Apple and Linux expect different requests and return different answers. Even changing libraries is not enough for programs that make these calls directly. [PAUSE] We need to translate at the point where the request happens.

## We don't need Macs to test our engine

- Our engine tests are plain command-line programs, so no macOS is needed
- Running them on Apple hardware pays for capability they never use
- Macs are expensive to buy, and renting them from cloud providers is expensive too
- Apple machines are also harder to scale, so they can become the bottleneck
- General-purpose machines are cheaper to operate and run many jobs at once
- Same coverage, no Apple hardware in the loop, more testing for the same outlay

> **Speaker notes:** **Here is why this matters beyond an interesting technical trick.** Our engine tests are command-line programs. They do not need a Mac desktop, a display, or Apple's graphical frameworks. But Macs cost more to buy or rent, and scaling them is difficult. [PAUSE] If we can run those tests on ordinary Linux machines, we can use more computing capacity and get feedback faster without paying for features those tests never touch.

## The idea: rewrite every OS request before the program starts

- MachGate reads the program file and finds every OS request
- Each one is replaced with a small jump into MachGate's own code
- That code answers using the Linux equivalent, then resumes the program
- Nothing else is emulated; only the OS boundary is translated
- Unsupported requests get the true "unsupported" answer, never fake success

*Figure: `docs/diagrams/syscall-patching.svg`*

> **Speaker notes:** **The key move happens before the program begins.** MachGate finds the instructions that ask Apple's operating system for help and replaces each one with a small jump into its own code. It translates the request, gets the Linux result, converts the answer, and resumes the program. [PAUSE] We leave the computation alone and translate only the operating-system boundary. If a request cannot be supported, we say so explicitly.

## How it works, end to end

- Validate the file, choose the right part of it
- Map it into memory, checking every range before use
- Rewrite every OS request, prepare the program's memory
- Set up arguments, environment, and its starting location
- Hand over control; answers keep flowing through MachGate

*Figure: `docs/diagrams/guest-loading.svg`*

> **Speaker notes:** **The entire startup follows five steps.** Validate the file. Select the arm64 code. Map its memory safely. Redirect its OS requests. Then start execution with the right arguments and environment. [PAUSE] Preparation is complete before the first guest instruction runs. That's what makes the model predictable rather than a collection of runtime workarounds.

## What runs today

- Command-line tools and statically linked runtimes
- The Apple-chip portion of universal (multi-chip) program files
- Origin doesn't matter: iOS and macOS command-line files share one format
- A corpus of known macOS command-line programs runs under MachGate
- Bitcoin's unit tests pass, and more than 99% of our engine's macOS and iOS test cases pass

> **Speaker notes:** **This is working on real code today.** MachGate can run compatible command-line tools, static runtimes, and the arm64 part of universal binaries. Some of those files come from macOS, others from iOS; what matters is the services they need. [PAUSE] The evidence is concrete: a corpus of known macOS command-line programs runs this way, Bitcoin's unit tests pass, and more than 99% of our engine's macOS and iOS test cases run and pass on Linux. [PAUSE] That's the result that moved this from possibility to useful infrastructure.

## What we deliberately don't support

- No graphical apps, no Apple user-interface frameworks, no GPU
- No Apple inter-process messaging; those requests fail with a clear error
- Threads are created by the host system, not by a MachGate thread scheduler
- Unsupported requests always return the true "unsupported" answer
- Failures are therefore diagnosable, not mysterious

> **Speaker notes:** **A clear boundary is a strength, not a weakness.** MachGate is not trying to run the whole Apple desktop. We leave graphical interfaces, GPU work, and Apple-specific inter-process messaging out of scope. Threads and child processes use the Linux host. [PAUSE] Unsupported requests fail explicitly. That keeps the results meaningful and tells us where compatibility ends.

## Why not the obvious alternatives?

- Rosetta translates Intel instructions into Apple-chip instructions — Apple's own tool, and Apple is winding it down
- QEMU emulates a whole chip in software — works anywhere, but every instruction pays a translation cost
- A virtual machine boots a second operating system — heavy, and a macOS one still needs Apple hardware
- MachGate translates the kernel conversation itself, once, before the program starts
- The program runs at full chip speed, and MachGate is just a normal Linux process

> **Speaker notes:** **Why not use the tools that already exist?** Rosetta translates Intel code on Macs. QEMU can emulate a processor. Virtual machines boot another operating system. [PAUSE] MachGate takes a different route: execute arm64 code natively and translate the direct kernel requests before the program starts. One boundary, designed for command-line workloads.

## Evidence, not demos

- 48 behavior test suites exercise the engine end to end
- 137 test-fixture files cover loading, memory, and OS requests
- Outside checks: 57 pinned public Apple command-line programs, plus Bitcoin's unit tests
- 669 of 669 OS requests classified from Apple's published table
- 455 (68%) genuinely translated; 214 (32%) return the true "unsupported" answer

> **Speaker notes:** **The evidence goes beyond one successful demo.** We have 48 behavior test suites, 137 fixture files, and 57 pinned public programs used as external probes, plus Bitcoin's unit tests passing under MachGate. We've classified all 669 syscall entries in the source table: 455 translated and 214 explicitly unsupported. [PAUSE] And the important result is still the real workload: more than 99% of our engine's macOS and iOS test cases pass.

## Where things stand

- Loading, OS request translation, memory handling, and program replacement all work
- Threads and child processes come from the host system
- Already possible: cap how many cores the program may use, and it sees a smaller machine
- A named profile could also report a different model and memory, from the same control point
- Model and version strings are fixed values, so every run reports the same identity

*Figure: `docs/diagrams/device-simulation.svg`*

> **Speaker notes:** **The foundation is already working.** Loading, system-call translation, memory handling, and process replacement are implemented, with host-backed threading and child processes. We can already influence affinity-based CPU answers by restricting available cores. A named profile that also selects device identity and memory is a logical extension, not something I'm claiming is delivered. [PAUSE] The direction is broader test coverage with increasingly controllable execution conditions.

## Try it, and where to read more

- Build it, build the test programs, run the full suite
- Then point it at any Apple ARM64 command-line program
- Architecture and design notes live in the documentation folder

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
bash tests/fixtures/build_fixtures.sh
bash tests/run_tests.sh
build/machgate path/to/program
```

> **Speaker notes:** **Use it, and extend it carefully.** These commands build MachGate, create the fixtures, run the tests, and launch a compatible Apple arm64 command-line program on Linux. The design and the supported boundary are documented with the source. [LONG PAUSE] The larger idea is simple: we should not tie command-line computation to expensive platform hardware when the processor can already run it natively.
