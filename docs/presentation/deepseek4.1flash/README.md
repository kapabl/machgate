# MachGate presentation decks

Three audience-specific decks, each a single markdown file written for
[Gamma.app](https://gamma.app) slide generation. Every deck carries a speaker
notes block on every slide, and all three include the same general economic
argument: our engine tests are command-line only, so running them on Apple
hardware pays for capability the tests never use.

The economics slides stay general on purpose. They are qualitative — no cost
figures, no hourly rates, no placeholders — and make one argument: buying or
renting Apple hardware is expensive and it is harder to scale than
general-purpose machines, while our tests are command-line only. The technical
reasons the approach works belong to the other slides; keep them out of the cost
argument.

| Deck | File | Audience | Slides |
| --- | --- | --- | --- |
| 1 | `1-engineers-speaker-notes.md` | Developers and contributors | 21 |
| 2 | `2-technical-management-speaker-notes.md` | Engineering managers, directors, senior directors | 11 + title card |
| 3 | `3-general-conference-speaker-notes.md` | Conference or external mixed audience | 11 |

## Importing into Gamma.app

1. Open Gamma, choose **Import** → **Paste in text** or **Import a file**, and
   select one deck file.
2. Each `##` heading becomes one card. The `#` heading becomes the deck title.
3. Add the matching diagrams from `docs/diagrams/` to the slides marked with a
   `*Figure:*` line:
   - `guest-loading.svg` — file to Guest Image
   - `syscall-patching.svg` — one instruction becomes a branch
   - `syscall-dispatch.svg` — the runtime path through the dispatcher
   - `host-components.svg` — shim, interposer, and provider layers
   - `device-simulation.svg` — device-profile control point
4. Speaker notes are markdown blockquotes. Gamma imports them as presenter
   notes; keep the `> **Speaker notes:**` marker so they stay identifiable.

## Fact sources

Claims in the decks are taken from `README.md`, `docs/DOMAIN_MODEL.md`,
`docs/SYSCALL_PATCHING.md`, and `docs/DARWIN_SYSCALL_TABLE_STATUS.md`. The
device-identity slides describe current behavior: the parallelism answer counts
`SYS_sched_getaffinity`, so restricting the process to fewer cores already
changes the core count a guest sees, while `src/shim/sysctl_compat.c` returns
fixed model and version strings and `src/shim/host_facts.c` supplies memory and
page size. A single named profile covering cores, model, and memory together would
be a contained extension at that control point. The pass-rate claims — Bitcoin's
unit tests pass under MachGate, and more than 99% of the engine's macOS and iOS
test cases pass — come from the team's engine test runs rather than from
repository documents.
