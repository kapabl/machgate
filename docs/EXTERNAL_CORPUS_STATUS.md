# External ARM64 Mach-O corpus status

## Regression gate

The authoritative command-line compatibility gate is
`tests/external/extended_arm64_macho_cli_manifest.txt`. It contains 57 unique,
checksum-pinned public ARM64 Mach-O probes.

The smaller `tests/external/arm64_macho_cli_manifest.txt` file contains 19
smoke probes. It is useful for a short feedback loop, but it is not a
replacement for the extended gate.

The 57-row gate is run with:

- an ARM64 Linux build of MachGate;
- an explicit ARM64 Apple-ABI libc++ and adjacent libc++abi provider;
- a 120-second timeout per probe; and
- exact native-provider keys, including both `libiconv` variants and
  `libresolv.9`.

The generic `libicucore` key maps to the compatibility library. The exact
versioned `libicucore.A` key is skipped because its imported ICU surface is not
used by the supported version probes and is outside the retained compatibility
surface.

CoreFoundation, IOKit, and CoreServices are explicit Partial providers.
Supported command-line symbols resolve from the compatibility library.
Unresolved ordinary data imports from a Partial provider bind null, while
unresolved function imports in lazy-pointer sections receive deterministic
fail-on-call bindings. For a strict mapped provider, a missing strong import
remains an eager error unless the Mach-O indirect symbol table proves that the
same symbol has a real `S_SYMBOL_STUBS` entry. That proof permits a fail-on-call
binding for a function whose pointer is encoded as a chained GOT fixup; a
data-only chained import remains a load error. This does not add GUI, device,
or event-loop implementations.

Flat and weak lookup searches guest definitions and explicitly selected
dependency providers before the ambient host namespace. This ordering is
required when Linux exports a same-named function with a different ABI
contract, such as thread-specific-data setup used by the Darwin Go runtime.

The exact versioned Objective-C provider key, `libobjc.A`, is `Skip` because
the Objective-C runtime is outside the supported surface and the corpus's
version paths do not exercise it. The versionless `libobjc` key remains `Stub`
for probes whose unsupported function calls must fail on call. Exact provider
decisions take precedence over versionless fallback.

The runner rejects a libc++ or libc++abi provider whose ELF machine is not
AArch64. It writes a per-probe `results.tsv` file and records each result as a
pass, crash, non-crash failure, or harness failure. Failed probes also receive
host-strace and QEMU-strace diagnostic attempts.

## Bitcoin Core unit-test runner gate

`tests/external/bitcoin_core_test_manifest.txt` is a focused manifest of seven
checksum-pinned official Bitcoin Core macOS ARM64 release tarballs, versions
25.2 through 31.0. Each row points at the release's `test_bitcoin` Boost.Test
runner and invokes it with `--help`, which proves the official test binary
loads, links, and exits successfully under MachGate. It complements, and does
not substitute for, the 57-row gate.

The rows are the archived artifact of record from the 2026-09-27 seven-version
run. Two evidence directories record passing results, both 7 out of 7:

- `/home/kapablanka/repos/machgate-bitcoin-unwind-7-results-20260927-AzQWxE`
- `/home/kapablanka/repos/machgate-bitcoin-unwind-hardened-7-results-20260927-3C3GYm`

The manifest runs with the same harness by pointing `MACHGATE_EXTERNAL_MANIFEST`
at the file and supplying the ARM64 libc++ provider. Full unit-suite pass
results are engine evidence recorded with the team's test runs, not harness
output.

## Historical completion evidence

MachGate is a cleanup of the earlier WIP repository, where historical gate
iterations and completion evidence remain recorded.

## Current baseline

An immutable build snapshot was run against all 57 rows with the
verified preserved artifact cache and an AArch64 libc++ provider.

Result:

| Classification | Count |
| --- | ---: |
| Pass | 57 |
| Crash | 0 |
| Non-crash failure | 0 |
| Harness failure | 0 |
| Total | 57 |

The same source snapshot passed the ARM64 core suite: 40 out of 40 tests, with zero
failures.

This baseline used these immutable inputs:

| Input | SHA-256 |
| --- | --- |
| Source-snapshot inventory | `32d79cdcd7b365030001a0670afa6223819ab16f0d8661114a792753f51dbeba` |
| MachGate executable | `06d68466dfe60015f02ab7986047ef61369af7f6bd16d37fc2eb012ae43e737f` |
| Compatibility library | `a20510d293bbc19449f0ee75d185ce3fc8df2169523004ec9df6697b119237d2` |
| ARM64 libc++ provider | `5cb4cf45cd02fb2a46b2c72093be720c0c0e165aceb981059fb9b3390a4761ba` |
| Extended manifest | `9143dde0fc94cc912c61bbd6f335f5fe517e2b9fb81e8e179aa91cc3c4b210d8` |
| External runner | `ae74fa31594945c3792f01779535a8a7ebd8dcb26b4cbde93a8e64a1a2d34582` |
| Result table | `824e61dce6652a06be330f92b19f2b8608d135dab4d57d8373a7cdcb42efe3a7` |
| Core-suite log | `e584b9102f7b879a793229308bd84871caaa15026850bb981cbe0d21b4bb99c2` |
| External console log | `b8b4b7988147391af1a3aab3aaddcb76b0e4c43e648bd07c5b73175227db3fec` |

The baseline ran against the archived manifest listed above. The current
`tests/external/extended_arm64_macho_cli_manifest.txt` differs from that
archived copy in one row: the `node` probe URL is pinned to the versioned
release path instead of the rolling `dist/latest` path, with the probe's
checksum, binary path, and arguments unchanged. Its current SHA-256 is
`d83a131e8f285b426f3936eca9dde23939f3d14342b97118113de101f5ce3bd1`.

The evidence directory is
`/home/kapablanka/repos/machgate-final2-57-results-20260927-Mqjn0q`.
Its `external/logs/results.tsv` file contains every probe result and its
stdout/stderr paths. The runner creates host-strace and QEMU-strace artifacts
only for failures, so this all-pass run produced no failure traces.

This baseline does not add GUI, game, HID, FSEvents, Objective-C runtime, or
run-loop implementations; unavailable optional function imports remain governed
by the documented fail-on-call policy.

## Reproduction

From the repository root, the complete Docker-based run is:

```sh
./scripts/run-full-corpus.sh
```

Set `MACHGATE_LIBCXX_DIR` when the ARM64 libc++ provider is not in the default
local provider directory.

Run from an ARM64 Linux environment after building MachGate and the pinned
Apple-ABI libc++ provider:

```sh
MACHGATE_ROOT=/work \
BUILD_DIR=/work/build-arm64 \
MACHGATE_RUN_EXTERNAL=1 \
MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/extended_arm64_macho_cli_manifest.txt \
MACHGATE_EXTERNAL_CACHE=/path/to/verified-public-cache \
MACHGATE_EXTERNAL_WORK=/tmp/machgate-external-57/work \
MACHGATE_EXTERNAL_LOGS=/tmp/machgate-external-57/logs \
MACHGATE_EXTERNAL_TIMEOUT=120 \
MACHGATE_EXTERNAL_REQUIRE_LIBCXX=1 \
MACHGATE_EXTERNAL_LIBCXX=/work/build-libcxx/lib/libc++.so.1 \
bash tests/test_external_macho_cli.sh
```

The cache is an optimization only. The harness can download missing public
artifacts, but it always verifies the manifest SHA-256 before execution.

External-corpus results measure compatibility behavior.
