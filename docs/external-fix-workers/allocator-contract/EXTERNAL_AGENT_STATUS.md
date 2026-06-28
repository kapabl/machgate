# External Agent Status

## Grok

Status: completed.

Useful findings:

- native mapped `libc++.so.1` and `libc++abi.so.1` can allocate outside the
  MachGate ledger
- direct and malloc-zone allocator exports were incomplete
- flat resolver lookup could fall back to host `RTLD_DEFAULT`
- `_libc_default_*` coverage is narrower than Darwin libmalloc's full surface

Prompt: `grok-prompt.txt`
Report: `grok-report.txt`

## Agy

Status: completed.

Useful findings:

- missing C++ destroying-delete variants remain a future audit item
- `malloc_zone_from_ptr`, `malloc_get_all_zones`, and unregister/query APIs were
  absent
- native `libc++/libc++abi` allocation ownership remains the highest residual
  risk

Prompt: `agy-prompt.txt`
Report: `agy-report.txt`

## Claude

Status: blocked by authentication in this run.

Prompt: `claude-prompt.txt`
Observed result: `401 Invalid authentication credentials`.
