# Loop Engineering

Loop engineering is the repeatable debugging workflow used for MachGate
external-binary work. It turns a vague failing corpus into bounded fix loops
with evidence, stop conditions, and visible status.

## Core Rule

Each loop owns one failure bucket, not the whole project. A bucket is a group of
binaries that fail with the same visible symptom, trace pattern, or suspected
runtime gap.

Examples:

- `BOOST_AUTO_START_DBG`: Boost.Test exits with `Parameter auto_start_dbg has invalid characters in name.`
- `TIMEOUT_AFTER_MAIN`: static initialization completes, `_main` is called, and the binary times out.
- `SIGILL_PROBE`: a guest instruction raises `SIGILL` during startup, then the binary may continue or fail later.

## Loop Shape

1. Pick one bucket and list the exact binaries.
2. Collect current evidence: stdout, stderr, qemu-strace, strace, signal context,
   disassembly offsets, source references, and exact command lines.
3. Form one narrow hypothesis.
4. Apply one minimal change or experiment.
5. Run the smallest targeted verification for that bucket.
6. Record the result in the loop doc.
7. Repeat until fixed or the retry limit is reached.

## Stop Conditions

- Stop early when the bucket passes its targeted manifest.
- Stop after 5 failed fix attempts for the same bucket and mark it
  `harder-later` with exact evidence.
- Stop if the symptom changes into a new bucket; document the old bucket as
  fixed or superseded and start a new loop.
- Stop if the only remaining failure is environmental or timeout-only and not a
  functional blocker; document the timeout threshold that passes.

## Required Prompt Fields

Every agent, Grok, Claude, Gemini, or AGY loop prompt should include:

- Working directory.
- Bucket name and exact binaries.
- Current evidence files.
- Allowed write scope.
- Retry limit.
- Verification command.
- Documentation file to update.
- Final response format.

## Prompt Template

```text
You are working in /home/kapablanka/repos/machgate.
You are not alone; do not revert or overwrite other workers' changes.

Bucket: <BUCKET_NAME>
Binaries: <binary list>
Evidence:
- <log paths>
- <doc path>

Loop engineering rules:
- Try up to 5 concrete hypotheses.
- For each attempt: state hypothesis, make one minimal change or experiment,
  run the smallest targeted verification, record result.
- Stop early if fixed.
- If 5 attempts fail, mark harder-later with exact evidence.
- If the symptom changes, document the new bucket and stop this loop.

Allowed writes:
- <paths>

Verification:
<command>

Update:
- <status doc path>

Final response:
- attempts made
- files changed
- commands run
- pass/fail evidence
- next hypothesis if not fixed
```

## Current C++ Test-Runner Loop

Source of truth: `docs/CATCH2_STATIC_INIT_EXTERNAL_LOOP.md`.

Current buckets:

| Bucket | Count | First target |
|---|---:|---|
| `BOOST_NOTHING_TO_TEST_ABORT` | 4 | `bitcoin-29.2-test` |
| `TIMEOUT_AFTER_MAIN` | 0 active, 8 fixed | `bitcoin-test` |
| `BOOST_AUTO_START_DBG` | 0 active, 3 superseded | `knots-test` |

Recommended order:

1. `BOOST_NOTHING_TO_TEST_ABORT`, because it is now the only focused public
   C++ test-runner blocker.
2. Regression-check `TIMEOUT_AFTER_MAIN`, because it was fixed by the libc++
   atomic-wait fallback plus `_Unwind_Resume` forwarding.
3. Regression-check the old `BOOST_AUTO_START_DBG` parser symptom, because it
   was fixed by Darwin ctype masks and is now superseded.

## External Agent CLI Notes

- Grok: see `docs/GROK_CLI_USAGE.md`.
- Gemini: see `docs/GEMINI_CLI_USAGE.md`. Use
  `npx -y @google/gemini-cli`; the plain `gemini` binary is not currently on
  `PATH` here. Current blocker is authentication, not the prompt shape.
- AGY / Antigravity: see `docs/AGY_CLI_USAGE.md`. Use `agy --print` for
  headless mode and `--dangerously-skip-permissions` for full auto-approve.
  Current blocker is login; `agy` prints an OAuth URL and waits for an auth
  code.
