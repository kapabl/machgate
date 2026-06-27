#!/bin/bash
# Test: machgate handles LC_MAIN entry point correctly
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/with_lc_main ] || bash tests/fixtures/build_fixtures.sh
status=0
"$BUILD_DIR/machgate" tests/fixtures/with_lc_main 2>/dev/null || status=$?
[ $status -eq 7 ]
