# MachGate

MachGate runs ARM64 Mach-O command-line executables in ARM64 Linux containers.
Its central feature is a Darwin-to-Linux syscall gateway for guest code that
issues `svc #0x80` directly, including statically linked runtimes that do not
call imported libSystem functions.

The current scope is intentionally narrow:

- ARM64 Mach-O executables on ARM64 Linux
- thin executables and ARM64 slices from fat binaries
- command-line workloads
- a single guest thread
- direct Darwin syscall translation

MachGate does not target x86-64 guests, graphical applications, Apple
frameworks, Mach IPC, or general macOS application compatibility.

## Clean-room rewrite boundary

MachGate's current core is a clean-room rewrite derived from Apple open-source
format and ABI references, LLVM/LLD behavior and documentation, public ABI
documentation, and MachGate's behavioral tests. The syscall translators and
native compatibility shim are MachGate-owned functionality and remain part of
the rewrite.

The sibling reference checkout is used only to classify provenance and decide
which legacy files can be deleted. It is not an implementation or behavioral
specification for replacement code. This describes the project's engineering
boundary; it is not a legal certification. Required attribution remains in
[ATTRIBUTION.md](ATTRIBUTION.md).

## How it works

MachGate maps the executable's segments at their requested virtual addresses,
selects an `LC_MAIN` or `LC_UNIXTHREAD` entry point, and creates a guarded guest
stack. Before entering guest code it scans executable instruction sections for
Darwin `svc #0x80` instructions and replaces each one with a direct ARM64
branch to a nearby generated island.

The island preserves guest registers and flags, calls the syscall dispatcher,
then resumes at the instruction after the original `svc`. The dispatcher
translates syscall numbers, arguments, structures, flags, results, and errno
values between Darwin and Linux conventions. Unsupported calls return Darwin
`ENOSYS`.

MachGate also includes focused compatibility components:

- classic and chained Mach-O import binding to an explicitly selected native
  provider
- a native libSystem shim for imported command-line runtime functions
- guest C++ allocator hook discovery
- an optional prefix-based native trampoline mechanism
- a VM interposer for host mappings that must participate in guest VM tracking

See [syscall patching](docs/SYSCALL_PATCHING.md) for the gateway design and
[the syscall table](docs/DARWIN_SYSCALL_TABLE_STATUS.md) for dispatcher coverage.

## Build

MachGate must be built for an ARM64 Linux target.

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
```

The build produces:

- `build/machgate`
- `build/libsystem_shim.so`
- `build/libmachgate_vm_interpose.so`

Run a guest with:

```sh
build/machgate path/to/program [arguments...]
```

Use `-v` for loader diagnostics.

## Test

The fixture builder needs an LLVM toolchain capable of producing ARM64 Mach-O
objects and executables.

```sh
bash tests/fixtures/build_fixtures.sh
bash tests/run_tests.sh
```

Tests cover thin and fat loading, entry-point handling, static initializers,
syscall ranges, VM operations, process re-execution, the shim surface, allocator
hooks, classic and chained libSystem imports, and native trampolines.

## Optional runtime settings

`MACHGATE_SYSTEM_SHIM_LIB` names the native shim used for dynamic libSystem and
guest allocator bridge tests.

`MACHGATE_DYLIB_MAP` names a `key = native-library` map for other Mach-O
dependencies. Keys use the dependency basename without the `.dylib` suffix.

`MACHGATE_TRAMPOLINE_LIB` and `MACHGATE_TRAMPOLINE_PREFIX` enable explicit
prefix-based redirection of guest definitions to symbols in a native shared
library.

`MACHGATE_TRACE_CXX_ALLOCATORS` prints the number of guest C++ allocation hooks
connected to the shim.

Additional narrowly scoped trace variables live next to the subsystem that
implements them. They are diagnostics, not a stable public interface.

## Repository layout

```text
src/machgate.c                 startup and guest entry
src/loader.c                   thin/fat validation and segment mapping
src/trampoline.c               optional native symbol redirection
src/guest_symbols.c            guest symbol lookup
src/guest_allocator_bridge.c   guest-to-shim allocator hooks
src/syscall/                   Darwin syscall patching and translation
src/shim/                      native libSystem compatibility surface
tests/                         fixture and behavior tests
docs/                          current architecture and maintenance notes
```

## License and attribution

MachGate is distributed under GPL-3.0-or-later. See [LICENSE](LICENSE) and
[ATTRIBUTION.md](ATTRIBUTION.md) for the required legal notices and provenance.
