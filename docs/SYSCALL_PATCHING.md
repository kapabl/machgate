# Darwin syscall patching

MachGate does not wait for a Darwin syscall to trap into Linux. Linux does not
understand the Darwin ARM64 syscall ABI, so every Darwin `svc #0x80` instruction
in guest executable code is patched before the guest runs.

## Original Darwin call path

Darwin ARM64 syscall wrappers pass the syscall number in `x16`, arguments in
`x0` through `x5`, and execute `svc #0x80`.

```text
Guest Mach-O __TEXT,__text

  address N-8:    mov/movz/... x16, #DARWIN_SYS_write
  address N-4:    argument setup in x0..x5
  address N:      svc #0x80              opcode 0xd4001001
  address N+4:    guest continuation

Darwin result contract:

  success: x0 = return value, carry flag clear
  failure: x0 = Darwin errno, carry flag set
```

That instruction cannot execute directly on Linux because the host kernel would
receive a Linux `svc` with the wrong syscall number register, wrong syscall
number table, and wrong error convention.

## Patched call path

MachGate scans executable Mach-O instruction sections and replaces each
four-byte `svc #0x80` instruction with a direct ARM64 branch to a nearby island.

```text
Before patching

  guest text at N
  +------------------------------+
  | svc #0x80                    |  0xd4001001
  +------------------------------+
  | instruction after syscall    |  N + 4
  +------------------------------+

After patching

  guest text at N                         nearby executable island
  +------------------------------+        +------------------------------+
  | b syscall_island             | -----> | save x0..x18, x30, NZCV     |
  +------------------------------+        | save resume PC = N + 4       |
  | instruction after syscall    | <----+ | x0 = &syscall_gate_state     |
  +------------------------------+      | | blr syscall_gate_dispatch    |
                                      | | restore x1..x18, x30, NZCV   |
                                      | | restore translated x0 result  |
                                      +- | br resume PC                  |
                                        +------------------------------+
```

The patch uses `b`, not `bl`, so the guest link register `x30` is not destroyed.
The island uses `blr` only for the internal call from generated code into the
MachGate C dispatcher.

## Why branch islands exist

ARM64 instructions are fixed width: one instruction is four bytes. A direct
`b` instruction can only reach a signed 26-bit immediate scaled by four bytes,
or roughly plus/minus 128 MiB from the patched instruction. MachGate therefore
allocates executable island pools near the guest code and emits one island per
patched syscall site.

```text
Mach-O code range                         MachGate island pool

  0x100000000                              0x102640000
  +------------------------------+         +------------------------------+
  | normal guest instructions    |         | island for svc site A        |
  | b island_for_syscall_A       | ------> | island for svc site B        |
  | normal guest instructions    |         | island for svc site C        |
  +------------------------------+         +------------------------------+

Rule: every patched `b` must be in ARM64 direct-branch range of its island.
```

## Dispatcher ABI boundary

The generated island builds a `struct syscall_gate_state` on the guest stack and
passes its address to `syscall_gate_dispatch`.

```text
guest register state at patched syscall

  x0..x5   Darwin syscall arguments
  x16      Darwin syscall number
  x30      guest link register
  NZCV     guest condition flags
  PC       original continuation address, N + 4

             saved into syscall_gate_state
                         |
                         v
  +---------------------------------------------------+
  | syscall_gate_dispatch(struct syscall_gate_state*) |
  +---------------------------------------------------+
        |
        | translate number, flags, structures, errno,
        | return value, and carry flag semantics
        v

  Linux backend uses Linux syscall ABI internally:

  x8 / SYS_* number, Linux flags, Linux structs, Linux errno
```

The C dispatcher treats `state->x[16]` as the Darwin syscall number. It either
implements the Darwin operation in MachGate, calls a raw Linux syscall with
translated arguments, or returns Darwin `ENOSYS` for unsupported Darwin syscalls.

## Return path

The dispatcher writes the Darwin-visible result back into the saved state. The
island restores the modified state and branches to the original continuation
address.

```text
Darwin syscall succeeds

  dispatcher: state->x[0] = result
              state->nzcv &= ~CARRY

  island:     restore registers and NZCV
              br N + 4

Darwin syscall fails

  dispatcher: state->x[0] = darwin_errno
              state->nzcv |= CARRY

  island:     restore registers and NZCV
              br N + 4
```

From the guest program's point of view, it executed a Darwin syscall wrapper and
continued at the next instruction with Darwin's result convention preserved.
From the host's point of view, no Darwin `svc` reached the Linux kernel.

## Instruction-boundary safety

MachGate only scans sections marked as ARM64 instruction sections by the Mach-O
metadata. ARM64 has fixed four-byte instruction width, so the scan walks those
sections in four-byte units and matches the exact `svc #0x80` opcode
`0xd4001001`. This avoids byte-pattern patching in data and keeps replacement
at real instruction boundaries.

After patching a section, MachGate restores the original Mach-O memory
protections and clears the instruction cache for both patched text and generated
islands.
