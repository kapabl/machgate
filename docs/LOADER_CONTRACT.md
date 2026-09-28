# Loader contract

The loader is deliberately limited to the executable image needed by the
current ARM64 command-line target. It is not a general dynamic loader.

This loader is part of the MachGate clean-room rewrite. Its implementation
contract comes from Apple open-source Mach-O definitions, LLVM/LLD behavior and
documentation, public ARM64 and Mach-O ABI documentation, and MachGate tests.
The sibling reference checkout is provenance and deletion evidence only, not an
implementation or behavior specification. This is an engineering boundary, not
a legal certification.

## Accepted input

The input must be an ARM64 `MH_EXECUTE` Mach-O image, either as a thin file or
as the ARM64 slice of a 32-bit- or 64-bit-table fat file. File ranges, command
sizes, segment ranges, and address arithmetic must be validated before mapping.

The executable must provide an entry point through `LC_MAIN` or a supported
ARM64 `LC_UNIXTHREAD` command.

## Mapping

Each non-empty segment with a nonzero initial protection is mapped at its
declared guest virtual address with `MAP_FIXED_NOREPLACE`. File bytes are copied
into the mapping; anonymous memory supplies zero-fill. Final Mach-O protections
are applied only after the image is complete.

The loader records the mapped address span and makes nearby executable pages
available to the syscall and trampoline patchers. Every direct branch to an
island must remain within the ARM64 branch displacement limit.

## Pre-entry sequence

Before the entry point runs, MachGate must:

1. validate and map the executable;
2. bind supported classic or chained Mach-O pointer imports to explicitly
   selected native providers;
3. apply explicitly configured native trampolines;
4. patch Darwin syscall instructions;
5. configure the initialized, alignment-padding, and zero-fill extent of the
   guest TLV storage image;
6. allocate a guarded, 16-byte-aligned guest stack;
7. expose guest process arguments and executable-path state;
8. connect guest C++ allocation operators to the shim when both surfaces are
   available;
9. run pointer-based `S_MOD_INIT_FUNC_POINTERS` and offset-based
   `S_INIT_FUNC_OFFSETS` entries on the guest stack;
10. enter the guest executable's directory; and
11. transfer control to the selected entry point.

No initializer or guest entry point may run before syscall sites are patched.

## Current exclusions

The focused loader does not recursively load Mach-O dynamic libraries and does
not claim full dyld behavior. Classic bind/lazy-bind streams and ARM64 chained
pointer formats may target a native libSystem provider selected with
`MACHGATE_SYSTEM_SHIM_LIB`. Function redirection is explicit through
`MACHGATE_TRAMPOLINE_LIB` and `MACHGATE_TRAMPOLINE_PREFIX`.

New load-command or dynamic-link behavior requires a fixture that fails without
the feature and passes with the smallest implementation of the required
contract.
