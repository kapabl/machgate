# Packer SIGCHLD Fix

Last refreshed: 2026-06-24.

## Summary

`packer --version` is fixed under the external functional gate.

The accepted fix is in `libsystem_shim`: when a Mach-O guest registers a Darwin
`SIGCHLD` handler through `sigaction(20, ...)`, MachGate reports success to the
guest but keeps the host Linux `SIGCHLD` disposition at `SIG_DFL` by default.
Blocking `wait4` still reports child status to the guest.

## Root Cause

Packer enters HashiCorp `panicwrap` before printing the version. That path
self-reexecs, waits for the wrapped child, copies output, and decides the parent
exit status from the child result.

Before this fix:

- The wrapped child printed `Packer v1.15.4`.
- The wrapped child exited `0`.
- Shim `wait4` observed and wrote Darwin status `0`.
- The outer panicwrap parent still exited `2`.

Loop Q rejected wait-status corruption with byte tracing:

- `linux_status=0`
- `darwin_status=0`
- wait status bytes before and after the shim write were `00000000`

The real failure was signal ABI mismatch:

- Darwin arm64 `SIGCHLD` is signal `20`.
- Linux arm64 `SIGCHLD` is signal `17`.
- Darwin Go's signal table treats signal `17` as `SIGSTOP`, not `SIGCHLD`.
- Delivering Linux `SIGCHLD=17` into Darwin Go signal machinery put panicwrap
  on the failure path after a successful child wait.

## Rejected Approach

A C dispatcher experiment translated Linux `SIGCHLD=17` to Darwin `SIGCHLD=20`
and called the saved guest handler. It logged the expected delivery, then
crashed with `SIGSEGV`.

That approach is rejected because a normal C signal wrapper does not preserve
Darwin Go's `sigtramp` frame/ABI.

## Accepted Policy

By default, MachGate does not install the guest handler as the host Linux
`SIGCHLD` handler. It still lets the guest believe `sigaction(SIGCHLD, ...)`
succeeded.

This policy is intentionally narrow:

- It applies to Darwin `SIGCHLD` only.
- It preserves blocking `wait4`, which is the path Packer uses for child status.
- It avoids injecting Linux signal number `17` into Darwin Go code expecting
  Darwin signal number `20`.
- `MACHGATE_ENABLE_HOST_SIGCHLD_HANDLER=1` can opt back into host SIGCHLD
  handler installation for diagnostics.

## Validation

Accepted Packer evidence:

- `tests/external/logs/loop-q4-packer-sigchld-production-attempt{1..5}/`:
  `packer --version` passed `5 / 5`.
- `tests/external/logs/loop-q6-packer-post-unitfix-attempt{1..5}/`:
  Packer reconfirmed `5 / 5` after the raw Darwin `vfork` unit regression fix.
- `tests/external/logs/full-loop-2026-06-24-Q-functional120/`:
  integrated 57-binary external corpus passed `57 / 57`.
- `tests/external/logs/full-loop-2026-06-24-R-functional120/`:
  fresh integrated 57-binary external corpus rerun passed `57 / 57`.

Regression evidence:

- ARM64 unit suite passed `28 / 28`; latest rerun after Loop R also passed
  `28 / 28`.
- Guard bucket `loop-q5-sigchld-guards` passed `10 / 10`.

## Caveat

This is not complete asynchronous Darwin signal emulation. It is a production
policy for the current single-guest-thread CLI target where child status is
consumed through blocking `wait4`.
