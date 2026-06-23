# Worker J: Canonical Merge And Regression

## Scope

Owned files:

- `tests/external/arm64_macho_cli_manifest.txt`
- `docs/EXTERNAL_MACHO_CLI_PROBES.md`
- `docs/external-fix-workers/canonical-merge.md`

Source files were not edited.

## Status

Worker J stopped after attempt 1. The canonical manifest has 19 rows, not 50, because the available evidence only identified 9 additional passing rows. The manifest needs 31 more reproducible passing rows to reach the 50-row target.

## Attempt 1

Merged known-passing rows only:

- `zoxide`
- `sd`
- `git-cliff`
- `sccache`
- `cargo-binstall`
- `hexyl`
- `pastel`
- `atuin`
- `ninja`

Rows intentionally not merged:

- Worker 1 failures: `procs`, `nu`
- Worker 2 failures: `fzf`, `gh`, `lazygit`, `glow`, `gum`, `goreleaser`, `chezmoi`, `duf`, `shfmt`, `fx`
- Worker 3 failures: `kubectl`, `helm`, `k9s`, `kind`, `minikube`, `tilt`, `argocd`, `flux`, `kubeseal`, `stern`
- Worker 4 failures: `terraform`, `packer`, `vault`, `consul`, `nomad`, `boundary`, `tofu`, `terragrunt`, `pulumi`
- Worker 5 failures: `cmake`, `sqlite3`, `duckdb`, `node`, `bun`, `protoc`, `nvim`

Manifest validation:

```sh
awk -F'|' 'BEGIN{count=0} /^[[:space:]]*#/ || NF < 5 {next} {count++; names[$1]++; if (names[$1] > 1) print "duplicate " $1} END{print count " canonical data rows"}' tests/external/arm64_macho_cli_manifest.txt
```

Result:

```text
19 canonical data rows
```

Normal regression command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail
cmake --build build-arm64 --parallel
bash tests/fixtures/build_fixtures.sh
BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh'
```

Result:

```text
27/27 passed, 0 failed
```

Canonical external command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/arm64_macho_cli_manifest.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/canonical-merge-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/canonical-merge-attempt1 MACHGATE_EXTERNAL_TIMEOUT=30 bash tests/test_external_macho_cli.sh'
```

Result:

```text
18/19 external Mach-O CLI probes passed, 1 failed
```

Passing canonical rows:

- `ripgrep`
- `fd`
- `hyperfine`
- `bat`
- `jq`
- `starship`
- `delta`
- `bottom`
- `just`
- `zoxide`
- `sd`
- `git-cliff`
- `sccache`
- `cargo-binstall`
- `hexyl`
- `pastel`
- `atuin`
- `ninja`

Failing canonical row:

- `yq`: status 139, no stdout, all binds resolved, reaches `_main`, then segfaults. Logs:
  - `tests/external/logs/canonical-merge-attempt1/yq.err`
  - `tests/external/logs/canonical-merge-attempt1/yq.qemu-strace`

## Exit

No attempts 2 through 5 were run. There are no additional known-passing worker rows to merge, and manifest/report-only changes cannot make known failing external rows reproducible passes.

Current canonical count:

- 19 rows

Shortfall:

- 31 additional reproducible passing rows needed to reach 50

Changed files:

- `tests/external/arm64_macho_cli_manifest.txt`
- `docs/EXTERNAL_MACHO_CLI_PROBES.md`
- `docs/external-fix-workers/canonical-merge.md`
