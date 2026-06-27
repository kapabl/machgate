#!/bin/bash
# Test: correct exit code for exit(0)
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/exit0 ] || bash tests/fixtures/build_fixtures.sh
"$BUILD_DIR/machgate" tests/fixtures/exit0 2>/dev/null
status=$?
[ $status -eq 0 ]
