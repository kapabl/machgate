# MachGate contributor instructions

## Project target

MachGate runs ARM64 Mach-O command-line executables inside ARM64 Linux
containers. Its defining feature is Darwin-to-Linux translation for guest code
that issues Darwin syscalls directly with `svc #0x80`.

Supported scope:

- ARM64 Mach-O guests
- ARM64 Linux hosts
- command-line executables
- a single guest thread
- thin files and ARM64 slices from fat files

Out of scope:

- graphical applications and game-specific integrations
- AppKit, Metal, CoreGraphics, WindowServer, and GUI event loops
- x86-64 guests
- Mach IPC
- general macOS application compatibility

Do not add GUI or game compatibility shims to the core.

## Clean-room rewrite boundary

Develop the replacement on a dedicated branch. Do not write rewrite commits to
`master`.

For this project, clean-room rewrite means replacement implementation is derived
from Apple open-source format and ABI references, LLVM/LLD behavior and
documentation, public ABI documentation, and MachGate behavioral tests. This is
an engineering boundary, not a legal certification.

Use the sibling reference checkout only to classify provenance and decide what
legacy material can be deleted. Do not use it as an implementation or behavior
specification, and do not copy implementation text from it into replacement
files.

The syscall gateway, syscall range dispatchers, guest VM tracking, re-execution
support, native compatibility shim, and their tests are MachGate-owned
functionality. Preserve them unless a behavior test proves a narrower
replacement is correct. Remove legacy or historical subsystems only after
confirming that current MachGate behavior does not depend on them.

Do not remove or rewrite `LICENSE` or `ATTRIBUTION.md` as part of technical
cleanup. Legal notices and Git history are separate from ordinary product and
architecture documentation.

## Core syscall design

Darwin ARM64 syscall wrappers use `x16` for the syscall number, `x0` onward for
arguments, and `svc #0x80` for entry. Darwin reports failure by setting carry
and returning a Darwin errno value.

Before guest execution, MachGate scans executable instruction sections and
replaces each `svc #0x80` with a direct branch to a nearby island. The island
must preserve guest registers and condition flags, dispatch on `x16`, restore
state, and resume after the replaced instruction. Use a direct branch rather
than branch-with-link so the guest link register is preserved.

Unknown syscall numbers must report the guest PC and argument registers when
the applicable diagnostic mode is enabled, then return Darwin `ENOSYS`.

Prefer raw Linux syscalls in translation backends. Translate flags, structures,
return values, and errno values explicitly; do not assume the Darwin and Linux
ABIs agree.

## Development rules

- Make narrowly scoped changes.
- Add or update a behavioral test before changing behavior.
- Keep existing syscall-range coverage.
- Do not restore deleted game, graphics, or general-purpose compatibility code.
- Avoid workload-specific special cases.
- Preserve unrelated user changes and untracked artifacts.
- Do not claim a cleanup or test result without current verification evidence.
- Use `rg`, `git diff`, and direct file reads for discovery. Do not use the
  disabled codebase index service.

## Build and test

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
bash tests/fixtures/build_fixtures.sh
bash tests/run_tests.sh
```

Use an ARM64 Linux environment. For debugging a failed compilation, rerun the
build with `cmake --build build --verbose -j1`.

## Code style

- Use descriptive, intention-revealing names.
- Use single-letter variables only for conventional loop indices.
- Keep functions short and prefer early returns.
- Name return-value variables `result`.
- Avoid inline comments; express intent through structure and naming.
- Preserve local formatting.
