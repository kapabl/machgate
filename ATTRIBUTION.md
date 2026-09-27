# Attribution

MachGate began as a substantially modified GPLv3-or-later fork of an earlier
standalone ARM64 Linux Mach-O loader. That upstream project provided the
original loader foundation and was itself based on loader components from
Darling's `mldr` work.

The current loader and runtime core have since been replaced with MachGate-owned
implementations derived from Apple open-source material, LLVM/LLD behavior and
documentation, public ABI documentation, and behavioral tests. This file keeps
the project's technical lineage visible without treating the earlier codebase
as the implementation specification for the rewrite.

## Inherited Foundation

- Mach-O parsing and ARM64 fat-binary selection
- Segment mapping and guest stack setup
- Commpage setup
- Chained fixup and import resolution foundation
- Mach-O dylib loading foundation
- Function trampoline and nearby branch-island machinery
- ARM instruction patching support

## MachGate Additions

- Darwin-to-Linux syscall gateway for guest `svc #0x80` instructions
- Translated Darwin syscall table and Darwin errno/status handling
- `libSystem.B.dylib` shim for imported libc, libSystem, pthread, time, signal,
  file, process, VM, and runtime calls
- Host VM interposition for runtime paths that bypass guest syscall/import
  translation
- Mach-O guest re-exec support for process-wrapper workloads
- Apple/Darwin C++ ABI adapters and libc++ compatibility work
- TLV bootstrap, initializer ordering work, compact-unwind to DWARF EH-frame
  registration, and runtime diagnostics
- ARM feature compatibility patches for RCPC and LSE atomics on older ARM64
  hosts
- External Mach-O CLI corpus testing against real public macOS ARM64 binaries
- Docker/QEMU runner scripts, release packaging, and GitHub workflow automation
- Project documentation, loop-engineering reports, and compatibility status
  tracking

## Naming

The user-facing project name is MachGate. Executables, packages, releases,
Docker scripts, documentation, and diagnostics should use MachGate naming.

Historical upstream names should not appear in the active source tree. Original
copyright and license notices remain governed by their respective files. New
files should use MachGate naming and GPLv3-or-later licensing.

## License

MachGate is distributed under the GNU General Public License version 3 or later.
See [LICENSE](LICENSE).

Release tarballs may also include an Apple-ABI-compatible build of LLVM
`libc++` and `libc++abi` for C++ Mach-O compatibility. Those libraries are
distributed under the Apache License v2.0 with LLVM exceptions; see
`share/doc/machgate/LICENSE.libcxx.txt` and
`share/doc/machgate/LICENSE.libcxxabi.txt` in the release package.

When bundled libc++ depends on GCC `libatomic`, release tarballs also include
`libatomic.so.1`; see `share/doc/machgate/LICENSE.libatomic.txt` when present.
