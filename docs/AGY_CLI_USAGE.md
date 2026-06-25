# AGY CLI Usage

This repository can use Google's Antigravity CLI, `agy`, as an additional
external analysis/fix agent when authentication is available.

## Verified Binary

```sh
command -v agy
agy --version
```

Current local result:

```text
/root/.local/bin/agy
1.0.12
```

## Headless One-Shot Mode

Use `--print` / `-p` for one prompt that exits:

```sh
timeout 1200s agy \
  --print-timeout 20m \
  --dangerously-skip-permissions \
  --print 'You are working in /home/kapablanka/repos/machgate. Read AGENTS.md and docs/LOOP_ENGINEERING.md. Execute the assigned failure-bucket loop and report evidence.'
```

Short alias:

```sh
agy -p 'Say exactly: agy-ok'
```

## Useful Flags

| Flag | Purpose |
| --- | --- |
| `--print` / `-p` | Run a single prompt noninteractively and print the response. |
| `--prompt` | Alias for `--print`. |
| `--print-timeout 20m` | Wait longer than the default `5m` for hard debugging loops. |
| `--dangerously-skip-permissions` | Auto-approve all tool permission requests. Use only for trusted prompts. |
| `--sandbox` | Run with terminal restrictions enabled. Usually not used for MachGate fix loops. |
| `--model <name>` | Select a model after login. |
| `--project <id>` | Use an existing Antigravity project. |
| `--new-project` | Create a new project for the session. |
| `--continue` / `-c` | Continue the most recent conversation. |
| `--conversation <id>` | Resume a previous conversation by ID. |
| `--add-dir <path>` | Add another directory to the workspace; repeatable. |
| `--log-file <path>` | Write CLI logs to a known file. |

## Authentication

Current smoke command:

```sh
timeout 120s agy \
  --print-timeout 60s \
  --print 'Say exactly: agy-ok'
```

Current result in this environment:

```text
Authentication required. Please visit the URL to log in:
  https://accounts.google.com/o/oauth2/auth?...
Waiting for authentication (timeout 30s)...
Or, paste the authorization code here and press Enter:
Error: authentication timed out.
```

Model listing also confirms login is required:

```text
Error: Please sign in to view available models. Launch the CLI without arguments to sign in.
```

To authenticate interactively:

```sh
agy
```

Then complete the browser login or paste the authorization code when prompted.
After login, rerun the smoke test:

```sh
agy --print-timeout 60s --print 'Say exactly: agy-ok'
```

The changelog for `1.0.11` also says Application Default Credentials are
supported with:

```sh
USE_ADC=1 agy
```

Use that only when Google Cloud ADC is already configured on the host.

Current ADC smoke test:

```sh
timeout 90s env USE_ADC=1 agy --print-timeout 60s --print 'Say exactly: agy-ok'
```

Current result:

```text
Error: authentication required. Run 'agy' to log in.
```

## Models And Plugins

List models after login:

```sh
agy models
```

Plugin commands:

```sh
agy plugin list
agy plugin import gemini
agy plugin import claude
```

Current local plugin state:

```text
No imported plugins.
```

## Loop Prompt Template

```text
You are working in /home/kapablanka/repos/machgate.
Read AGENTS.md, docs/LOOP_ENGINEERING.md, and docs/CATCH2_STATIC_INIT_EXTERNAL_LOOP.md.
You are not alone; do not revert or overwrite other workers' changes.

Bucket: BOOST_NOTHING_TO_TEST_ABORT
Binaries: bitcoin-29.2-test, bitcoin-26.2-test, knots-test, qtum-test
Evidence:
- tests/external/logs/cpp-static-init-boost-nothing-to-test-abort-attempt1/
- tests/external/cpp_static_init_boost_nothing_to_test_abort_manifest.txt
- docs/CATCH2_STATIC_INIT_EXTERNAL_LOOP.md

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
MACHGATE_CPP_LOOP_ATTEMPTS=1 MACHGATE_CPP_LOOP_TIMEOUT=45 bash tests/external/run_cpp_static_init_bucket_loop.sh BOOST_NOTHING_TO_TEST_ABORT

Final response:
- attempts made
- files changed
- commands run
- pass/fail evidence
- next hypothesis if not fixed
```
