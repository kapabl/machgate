# Grok CLI Usage

This repository can use the local Grok Build CLI as an extra analysis agent.

## Verified Version

The local CLI was verified with:

```sh
grok --version
```

Known working version:

```text
grok 0.2.60 (474c2bbfc) [stable]
```

## Working One-Shot Command

Use `-p` / `--single` for a prompt that exits after completion:

```sh
grok --cwd /home/kapablanka/repos/machgate \
  --no-alt-screen \
  --disable-web-search \
  --no-subagents \
  --max-turns 1 \
  -p 'Say exactly: grok-ok' \
  --output-format plain
```

Expected smoke-test output:

```text
grok-ok
```

## Useful Flags

| Flag | Purpose |
| --- | --- |
| `--cwd /home/kapablanka/repos/machgate` | Run against this repo. |
| `--no-alt-screen` | Keep output in the current terminal instead of opening the TUI screen. |
| `-p` / `--single` | Send one prompt and exit. |
| `--prompt-file <path>` | Read the prompt from a file. Useful for long tasks. |
| `--output-format plain` | Print plain text output. |
| `--output-format json` | Print machine-readable JSON output. |
| `--output-format streaming-json` | Print incremental JSON events. |
| `--max-turns N` | Bound the amount of agent work. |
| `--disable-web-search` | Keep the task local-only. |
| `--no-subagents` | Prevent Grok from spawning its own subagents. |
| `--always-approve` | Allow tool execution without interactive approval. Use only for trusted prompts. |
| `--no-auto-update` | Skip update checks; useful for scripted runs. |
| `--session-id <id>` | Give the run a named session. |
| `--resume <id>` or `--continue` | Resume a previous Grok session. |

## Read-Only Analysis Template

Use this style when asking Grok to analyze failures without touching files:

```sh
grok --cwd /home/kapablanka/repos/machgate \
  --no-alt-screen \
  --disable-web-search \
  --no-subagents \
  --max-turns 8 \
  --output-format plain \
  -p 'Read-only task. Do not modify files. Analyze the MachGate Go Mach-O crash pattern. Read AGENTS.md, docs/EXTERNAL_MACHO_FIX_LOOP_2.md, docs/external-fix-workers/go-startup.md, docs/external-fix-workers/go-lcmain-abi.md, and relevant logs. Produce: likely root cause, evidence, exact source files/functions to inspect or patch, and 2-3 concrete experiments.'
```

## Prompt-File Template

For longer tasks, write the prompt to `/tmp/grok-task.txt`:

```sh
cat > /tmp/grok-task.txt <<'EOF'
Read-only task. Do not modify files.

Analyze the current external Mach-O failure class.
Return:
1. likely root cause
2. evidence with file/log references
3. exact source files/functions to inspect or patch
4. 2-3 concrete experiments
EOF

grok --cwd /home/kapablanka/repos/machgate \
  --no-alt-screen \
  --disable-web-search \
  --no-subagents \
  --max-turns 8 \
  --prompt-file /tmp/grok-task.txt \
  --output-format plain
```

## Notes

- `grok agent headless` opened the Grok Build pager in this environment and was not useful for batch stdout.
- Direct `-p` and `--prompt-file` both work after login.
- If Grok prompts for login or project trust, complete that interactively first, then rerun the noninteractive command.
- For source-editing tasks, constrain file ownership explicitly and require a visible report file under `docs/external-fix-workers/`.
