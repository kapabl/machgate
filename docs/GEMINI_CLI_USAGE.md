# Gemini CLI Usage

This repository can use Google's Gemini CLI as an extra analysis agent, similar
to Grok and Claude, when authentication is available.

## Invocation

The command is not installed globally as `gemini` in this environment. Use the
npm entrypoint:

```sh
npx -y @google/gemini-cli --help
```

Headless mode uses `-p` / `--prompt`:

```sh
timeout 1200s npx -y @google/gemini-cli \
  --skip-trust \
  --approval-mode yolo \
  --output-format stream-json \
  -p 'You are working in /home/kapablanka/repos/machgate. Read AGENTS.md and docs/LOOP_ENGINEERING.md. Analyze the assigned failure bucket and write a concise report.'
```

For read-only analysis, use plan approval mode:

```sh
timeout 1200s npx -y @google/gemini-cli \
  --skip-trust \
  --approval-mode plan \
  --output-format text \
  -p 'Read-only task. Analyze the current MachGate failure bucket. Do not edit files. Return root cause, evidence, and next experiments.'
```

## Useful Flags

| Flag | Purpose |
| --- | --- |
| `-p` / `--prompt` | Noninteractive headless prompt. |
| `--skip-trust` | Trust the current workspace for this run. |
| `--approval-mode plan` | Read-only mode. |
| `--approval-mode auto_edit` | Auto-approve edits only. |
| `--approval-mode yolo` | Auto-approve all tool calls. Use only for trusted prompts. |
| `--yolo` | Deprecated alias for yolo approval mode. |
| `--output-format text` | Plain output. |
| `--output-format json` | Single JSON result. |
| `--output-format stream-json` | JSONL event stream, best for long-running background jobs. |
| `--model pro` | Pro model alias for harder diagnosis. |
| `--model flash` | Faster model alias for broad triage. |
| `--session-id <uuid>` | Start a named session. |
| `--resume latest` | Resume the latest session. |
| `--include-directories <path>` | Add more source/context directories to the workspace. |

## Authentication

The bundled docs list three supported authentication paths:

- OAuth login with an eligible Gemini Code Assist account.
- `GEMINI_API_KEY` from AI Studio.
- Vertex AI with `GOOGLE_API_KEY` and `GOOGLE_GENAI_USE_VERTEXAI=true`.

Current smoke test:

```sh
timeout 120s npx -y @google/gemini-cli \
  --skip-trust \
  --approval-mode yolo \
  --output-format text \
  -p 'Say exactly: gemini-ok'
```

Current result in this environment:

```text
Error authenticating: IneligibleTierError: This client is no longer supported for Gemini Code Assist for individuals.
```

That means Gemini is wired but blocked until a supported auth method is present.
After auth is fixed, rerun the smoke test and then launch loop prompts from
`docs/LOOP_ENGINEERING.md`.

## Loop Prompt Template

```text
You are working in /home/kapablanka/repos/machgate.
Read AGENTS.md, docs/LOOP_ENGINEERING.md, and docs/CATCH2_STATIC_INIT_EXTERNAL_LOOP.md.
You are not alone; do not revert or overwrite other workers' changes.

Bucket: <bucket>
Binaries: <exact list>
Evidence:
- <log paths>
- <manifest path>
- <source context path if any>

Loop engineering:
- Try up to 5 concrete hypotheses.
- For each attempt, collect evidence, make one minimal change or experiment,
  run the smallest verification, and record the result.
- Stop early if fixed.
- If 5 attempts fail, mark harder-later with exact evidence.
- If the symptom changes, document the new bucket and stop this loop.

Allowed writes:
- docs/CATCH2_STATIC_INIT_EXTERNAL_LOOP.md
- docs/EXTERNAL_MACHO_LIVE_STATUS.md
- tests/external/catch2_search/static_init_scan.tsv
- source files only if a narrow fix is proven

Verification:
<command>

Final response:
- attempts made
- files changed
- commands run
- pass/fail evidence
- next hypothesis if not fixed
```
