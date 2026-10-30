#!/usr/bin/env bash
# Full-snapshot C++ architecture and DDD review via the local Codex CLI.
# Runs under the ChatGPT/Codex subscription of the logged-in user
# (codex login) — no API key required. Usage:
#
#     scripts/ai-review.sh
#
# The report lands in ai-review/report.md.
set -euo pipefail

cd "$(dirname "$0")/.."

command -v codex >/dev/null 2>&1 || {
    echo "codex CLI not found. Install it with: npm install -g @openai/codex" >&2
    exit 1
}

mkdir -p ai-review

exec codex exec --sandbox workspace-write \
    "Perform a full-snapshot C++ architecture and Domain-Driven Design review of this repository (this is the whole review, not a diff review). Focus on: layer boundaries under src/ (domain, application, adapters, ports), bounded-context leaks, aggregate and invariant risks, business logic outside the domain layer, ubiquitous-language consistency with docs/DOMAIN_MODEL.md, and maintainability. Cite concrete files and lines as evidence for every observation, rank findings by importance, and keep the report under 100 lines of markdown. Write the report to ai-review/report.md (the directory already exists)."
