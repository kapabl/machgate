# MachGate presentation decks

Three audience-specific decks, each a single markdown file written for
[Gamma.app](https://gamma.app) slide generation. Every deck carries a speaker
notes block on every slide, and all three include the same cost argument: our
engine tests are command-line only and need no Apple hardware.

| Deck | File | Audience | Slides |
| --- | --- | --- | --- |
| 1 | `1-engineers.md` | Developers and contributors | 18 |
| 2 | `2-technical-management.md` | Engineering managers, directors, senior directors | 11 + title card |
| 3 | `3-general-conference.md` | Conference or external mixed audience | 11 |

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
   - `device-simulation.svg` — device-profile choke point
4. Speaker notes are markdown blockquotes. Gamma imports them as presenter
   notes; keep the `> **Speaker notes:**` marker so they stay identifiable.

## Fact sources

Claims in the decks are taken from `README.md`, `docs/DOMAIN_MODEL.md`,
`docs/SYSCALL_PATCHING.md`, and `docs/DARWIN_SYSCALL_TABLE_STATUS.md`. The
device-profile slides describe a capability at the sysctl choke point, not
shipped behavior: `src/shim/sysctl_compat.c` already returns fixed identity
strings, while CPU count and memory come from `src/shim/host_facts.c`.
