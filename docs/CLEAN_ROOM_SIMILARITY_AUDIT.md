# Clean-room source similarity audit

## Status

The current MachGate working tree has no byte-identical audited files and no
exact executable-logic, test, or build-automation block of eight or more lines
in common with the sibling reference checkout. The 139 current lines covered
by exact blocks of that size are public Mach-O, dyld chained-fixup, or Darwin
`stat` ABI layouts. The retained `src/shim/libsystem_shim.c` and every extracted
Compatibility Runtime module have zero exact blocks of eight or more lines.

This satisfies the audit's narrow exact-overlap remediation target for the
current working tree. It does not prove independent authorship, exclude
semantic similarity, certify a legal clean-room process, or establish that the
tree is behaviorally complete.

## Compared material

- MachGate HEAD recorded by the report:
  `d1bdcbbf493dfd9fb9c0680d36fcd11c50cd7d4f`
- Reference HEAD:
  `75d38cafa916d77659b3d7ccfabaaaa113a1832d`
- Audited inputs: 173 current files and 90 reference files
- Reference role: provenance comparison only, not an implementation source

The tool reads Git-indexed files plus unignored, untracked files. It includes
C, C++, headers, assembly, shell, Python, linker/version scripts, `.yml` files,
CMake modules, and `CMakeLists.txt`; it excludes `extern/`. This covers the
project's implementation, build, test, workflow, and helper-script surfaces
without treating the vendored LLVM checkout as MachGate source.

The report records repository HEADs, but the MachGate input also contains
working-tree changes. The result therefore describes the files at the time of
the run, not an immutable MachGate revision. It must be rerun after the work is
committed to attach the result to a fixed snapshot.

## Method

`tools/source_similarity.py` performs three independent measurements:

1. byte-identical files;
2. exact blocks of at least eight lines after removing trailing whitespace;
3. exact 25-token windows using a language-neutral lexical tokenizer.

The current result is reproducible with:

```sh
REFERENCE_CHECKOUT=/path/to/sibling-reference
python3 tools/source_similarity.py \
  . \
  "$REFERENCE_CHECKOUT" \
  --output /tmp/machgate-source-similarity.json
```

## Repository-level result

| Measure | Matched | Current total | Coverage |
| --- | ---: | ---: | ---: |
| Byte-identical files | 0 files | 173 files | 0% |
| Lines in exact blocks of 8 or more lines | 139 lines | 43,425 lines | 0.320% |
| Nonblank lines in those blocks | 125 lines | 38,565 lines | 0.324% |
| Tokens covered by exact 25-token windows | 1,496 tokens | 214,731 tokens | 0.697% |

The exact-block records cover nine ranges. One line in `src/macho_defs.h` is
shared by two adjacent records, so the block lengths sum to 140 while their
union is the reported 139 lines.

## Remaining exact-block classification

| Current range | Lines | Public contract represented |
| --- | ---: | --- |
| `src/syscall/syscall_gate.c:178-203` | 26 | Darwin `stat` layout and size assertion |
| `src/syscall/syscall_range_100_199.c:194-219` | 26 | Darwin `stat` layout and size assertion |
| `src/syscall/syscall_range_200_399.c:411-431` | 21 | Darwin `stat64` layout |
| `src/syscall/syscall_range_400_plus.c:531-551` | 21 | Darwin `stat` layout |
| `src/macho_defs.h:103-114` | 12 | Mach-O `section_64` layout tail |
| `src/macho_defs.h:114-121` | 8 | Mach-O `entry_point_command` layout |
| `src/macho_defs.h:181-188` | 8 | Mach-O `linkedit_data_command` layout |
| `src/native_imports.c:54-62` | 9 | dyld chained-fixups header layout |
| `src/native_imports.c:64-72` | 9 | dyld chained starts-in-segment layout |

These are field declarations and two layout assertions, not executable
function bodies. There are no exact blocks of eight or more lines in loader
logic, syscall translation logic, shim behavior, tests, shell/Python helpers,
version scripts, workflows, or CMake files. In particular, the broad syscall
ranges are MachGate implementation; the reported overlap inside them is only
the guest `stat` data layout.

The public-contract provenance used for the independent implementation is
catalogued in `docs/APPLE_OPEN_SOURCE_REFERENCES.md`. Repeated guest structures
can still be centralized later to reduce duplication, but their presence does
not change this block classification.

## Completed Compatibility Runtime split

The former monolithic compatibility surface has been separated into focused
modules compiled by the current CMake target:

- assertion, basic runtime, time, thread-local, unwind, and guest-process
  compatibility;
- Darwin errno, host facts, sysctl, locale/string, and pthread synchronization;
- file ABI translation, file operations, and filesystem metadata;
- dispatch, Core Foundation, and security compatibility.

The remaining `src/shim/libsystem_shim.c` is still part of the runtime, but its
previous long exact implementation blocks are gone. Neither it nor any of the
22 extracted C modules has an exact match of eight or more lines in this run.

Low-risk inherited surfaces outside the command-line target were also removed
from the export set, with negative export checks covering HID/LaunchServices
placeholders, CF run-loop and FSEvents placeholders, Mach receive-source and
notification registration, Mach port/semaphore/thread-control placeholders,
ICU placeholders, and redundant Darwin `stat` wrappers. The narrow retained
Mach operations now tested by the shim suite return explicit unsupported or
invalid-operation results instead of false success. This is a bounded cleanup
statement, not a claim that every unsupported API has been removed.

## Interpretation and limitations

Exact text matching is useful evidence for finding residual copied text, but it
has important limits:

- public ABI structures and conventional declarations create legitimate exact
  matches;
- token-window coverage includes common identifiers and short idioms and is a
  discovery signal, not a copied-code percentage;
- renaming, reformatting, or restructuring can hide common origin from this
  method;
- independently written implementations can remain semantically similar when
  they implement the same public contract;
- this audit does not establish provenance by itself and is not a legal
  certification; and
- the working tree can change after the report is produced.

Behavioral validation is separate from source-similarity validation. Unit,
fixture, and external-corpus results must be reported independently and cannot
be inferred from these percentages.

For a release or durable provenance record, rerun the audit against the final
committed tree, retain the JSON report with both immutable revisions, review any
new long blocks manually, and keep the public-contract provenance catalogue
with the implementation.
