#!/bin/bash
# Test: Darwin's read-only thread pointer uses isolated single-thread storage
# rather than exposing the host runtime's live thread-control block.
# References: Arm A-profile system-register encodings and Darwin ARM64 TLS ABI.
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/darwin_tpidrro ] || bash tests/fixtures/build_fixtures.sh

status=0
"$BUILD_DIR/machgate" tests/fixtures/darwin_tpidrro 2>/dev/null || status=$?
[ "$status" -eq 0 ]
