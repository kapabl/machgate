# Local Agent and Toolchain Status

This file records local agent runs, toolchain changes, and non-Docker verification state so work can resume after a crash or restart.

## Current Focus

Current external status is tracked in `docs/EXTERNAL_MACHO_LIVE_STATUS.md`.

As of the 2026-06-23 refresh:

- Canonical external manifest is `19/19` passing.
- Go TLS/signal targets `yq`, `fzf`, `gum`, and `shfmt` are passing.
- Full visible unique external corpus is `51/57` passing with `MACHGATE_EXTERNAL_MAP_LIBCXX=1`.
- Full visible unique external corpus is `45/57` passing without libc++ opt-in.
- Current full-corpus non-passing set is `nu`, `tilt`, `terraform`, `packer`, `nomad`, and `bun`.
- Loop E Codex workers are complete; Grok report is collected; Claude hit the local session limit.
- Active external report directory: `/tmp/machgate-agent-reports/loop-e/`.
- Latest full run logs are in `tests/external/logs/full-loop-2026-06-23-K-loop-e-final/`.
- Latest full run work dir is `tests/external/work/full-loop-2026-06-23-K-loop-e-final/`.
- Latest promoted passes are `cmake` and `nvim`; `protoc` remains passing.

The stale `yq` startup hypothesis below is retained as historical diagnosis only. It is no longer the active failure.

Current hypothesis:

- The crash is in Go Darwin `runtime.tlsinit`.
- The faulting instruction is `0x3980001b`, decoded as `ldrsb w27, [x0]`.
- At the fault, `x0=NULL`.
- Go Darwin startup reads `TPIDRRO_EL0` with instruction `0xd53bd060`.
- On Linux under MachGate, `TPIDRRO_EL0` is not the usable TLS base for this process.
- The narrow patch being tested rewrites `0xd53bd060` to `0xd53bd040`, changing `MRS TPIDRRO_EL0` to `MRS TPIDR_EL0`.

Changed file currently under test:

- `src/machismo.c`

## Evidence

Existing logs:

- `tests/external/logs/worker-f-attempt4/yq.err`
- `tests/external/logs/worker-f-attempt4/fzf.err`
- `tests/external/logs/worker-f-attempt4/gum.err`
- `tests/external/logs/worker-f-attempt4/shfmt.err`

Common observed state:

- LC_MAIN vectors are valid and contiguous.
- `argv`, `envp`, and `applep` derived from the Darwin stack match the direct pointers.
- `apple0` is `executable_path=...`.
- All four crash at the same Go runtime pattern with `x0=NULL`.

Relevant Go runtime source on this host:

- `/usr/lib/golang/src/runtime/rt0_darwin_arm64.s`
- `/usr/lib/golang/src/runtime/asm_arm64.s`
- `/usr/lib/golang/src/runtime/tls_arm64.h`
- `/usr/lib/golang/src/runtime/sys_darwin_arm64.go`

Important source facts:

- Darwin ARM64 Go uses `MRS TPIDRRO_EL0` for TLS reads.
- Linux ARM64 Go uses `MRS TPIDR_EL0`.
- Darwin `runtime.tlsinit` creates a pthread key, writes magic `0xc476c475c47957`, then scans the TLS base for that magic.

## Grok Run

Working Grok command shape:

```sh
timeout 1200s grok \
  --cwd /home/kapablanka/repos/machgate \
  --no-alt-screen \
  --always-approve \
  --no-subagents \
  --max-turns 100 \
  --output-format streaming-json \
  -p '<prompt>'
```

Prompt used:

```text
You are working in /home/kapablanka/repos/machgate on MachGate.

Task: Diagnose the Go-family crash in yq/fzf/gum/shfmt. All binaries reach LC_MAIN with valid argv/envp/apple vectors, then fault at instruction 0x3980001b with x0=NULL. Local analysis suggests this may be Go Darwin runtime TLS startup: runtime.tlsinit reads TPIDRRO_EL0, gets a NULL/invalid TLS base under Linux, then scans for pthread TLS magic and faults. A tentative MachGate patch rewrites instruction 0xd53bd060 (MRS TPIDRRO_EL0) to 0xd53bd040 (MRS TPIDR_EL0) in executable Mach-O code.

Focus on:
- What Go runtime function lives at or near yq PC/file offset 0x10006a6d0 / 0x6a6d0
- What x0 should have been pointing to
- Whether the TPIDRRO_EL0 -> TPIDR_EL0 rewrite is the right narrow fix
- Any safer or more correct alternative patch

Constraints:
- Read-only analysis only: do not edit files, do not commit, do not push
- Do not use Docker
- Use only local tools: llvm-objdump, llvm-nm, go tool objdump/nm, strings, existing logs/docs/source

Return format:
- Exact disassembly around yq file offset 0x6a6d0 plus nearest symbols if recoverable
- Likely Go runtime function and source path
- One narrow high-confidence hypothesis
- One exact next experiment or verification command that does not use Docker
```

Grok output log:

- `/tmp/grok-yq-diagnosis-20260623-001130.jsonl`

Grok result:

- Identified the fault as `runtime.tlsinit`.
- Confirmed all four binaries have three `d53bd060` instructions.
- Confirmed the `TPIDRRO_EL0 -> TPIDR_EL0` rewrite is the right narrow first fix.
- Warned that the next possible failure is `runtime.tlsinit` aborting later if Linux pthread TLS layout does not expose the magic in the Darwin-expected scan window.

## Low-Token Agent Prompting

For Grok and Claude, prefer short "caveman" prompts for debugging loops. The goal is to reduce model turns, tool churn, and final-answer verbosity.

Pattern:

```text
Repo: /home/kapablanka/repos/machgate.
Task: diagnose yq crash.
Facts: LC_MAIN ok. Crash pc=0x10006a6d0 insn=0x3980001b x0=NULL. Suspect Go tlsinit / TPIDRRO.
Do: read logs/code, prove/refute, give patch.
No: docker, commit, long report.
Reply short.
```

For read-only analysis:

```text
Read only. No edits.
Find function at yq 0x10006a6d0.
Tell: function, why x0 NULL, next patch.
Reply short.
```

For writable fix attempts:

```text
Fix one thing only.
Patch narrow.
Run smallest test.
Report files changed + result.
Reply short.
```

## Claude Run

Claude Code is installed:

```sh
claude --version
```

Observed version:

```text
2.1.186 (Claude Code)
```

Yolo/full-permission mode is blocked when invoked as root:

```text
--dangerously-skip-permissions cannot be used with root/sudo privileges for security reasons
```

A `claude-agent` user was created with:

- home directory
- repo ACL write access
- `docker` group
- `wheel` group
- passwordless sudo
- `/home/claude-agent/.ssh`

But Claude under that user is not logged in:

```text
Not logged in · Please run /login
```

Current usable Claude path is root without yolo, or `claude-agent` after a login is completed for that user.

## Local Build Attempts

Host architecture:

```text
x86_64
```

Local x86 build:

```sh
cmake --build build --parallel
```

Result:

- Build succeeds.
- This does not validate ARM64 Mach-O execution.
- Running `bash tests/run_tests.sh` with `BUILD_DIR=build` is invalid for ARM64 guest execution on this x86_64 host and produces many fixture segfaults/bus errors.

Existing ARM64 build directory:

- `build-arm64`

Problem:

- It was configured inside `/work/build-arm64`.
- Running it from `/home/kapablanka/repos/machgate/build-arm64` fails CMake regeneration because `/work` does not exist in the host namespace.

Failure:

```text
CMake Error: The source directory "/work" does not exist.
```

## Installed Cross Packages

Installed on this CentOS host:

```sh
dnf -y install gcc-aarch64-linux-gnu gcc-c++-aarch64-linux-gnu binutils-aarch64-linux-gnu
```

Installed packages:

- `binutils-aarch64-linux-gnu-2.38-3.el9.x86_64`
- `gcc-aarch64-linux-gnu-12.1.1-1.el9.x86_64`
- `gcc-c++-aarch64-linux-gnu-12.1.1-1.el9.x86_64`
- `cross-binutils-common-2.38-3.el9.noarch`
- `cross-gcc-common-12.1.1-1.el9.noarch`
- `isl-0.16.1-15.el9.x86_64`

Current blocker:

```text
/usr/bin/aarch64-linux-gnu-ld: cannot find crt1.o: No such file or directory
/usr/bin/aarch64-linux-gnu-ld: cannot find crti.o: No such file or directory
/usr/bin/aarch64-linux-gnu-ld: cannot find -lc: No such file or directory
/usr/bin/aarch64-linux-gnu-ld: cannot find crtn.o: No such file or directory
```

Meaning:

- The cross compiler is installed.
- The ARM64 glibc/sysroot is not installed.
- `cmake -S . -B build-aarch64-host -G Ninja -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++` currently fails at compiler ABI detection.

## QEMU State

`binfmt_misc` has an ARM64 registration:

```text
/proc/sys/fs/binfmt_misc/aarch64
enabled
interpreter /usr/bin/qemu-aarch64
```

But this host did not have `/usr/bin/qemu-aarch64` in `PATH` or on disk during the check.

CentOS package search did not find a package literally named `qemu-user`.

Next action:

1. Install or locate a package that provides `/usr/bin/qemu-aarch64`.
2. Install an ARM64 glibc/sysroot package or provide a sysroot path.
3. Configure `build-aarch64-host`.
4. Run `build-aarch64-host/machismo tests/external/work/yq --version` through binfmt/QEMU.

## Current Patch Under Test

Patch intent in `src/machismo.c`:

- In executable instruction sections, rewrite:

```text
0xd53bd060  MRS TPIDRRO_EL0
```

to:

```text
0xd53bd040  MRS TPIDR_EL0
```

- Apply to the main Mach-O.
- Apply to loaded Mach-O dylibs.
- Log the rewrite count.

Expected pass signal for `yq`:

- Loader logs `rewrote 3 Darwin TPIDRRO reads to Linux TPIDR reads`.
- The crash no longer occurs at `pc=0x10006a6d0`.
- Best case: `yq --version` prints version output.
- If the next failure is `runtime.tlsinit` abort at `0x10006a714`, the rewrite helped but Linux pthread TLS layout still needs emulation.
