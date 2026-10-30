#!/bin/bash
# Test: the LC_MAIN launch context presents contiguous argv, environment, and
# apple vectors and mirrors argc/argv on the initial stack.
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/darwin_launch_context ] || \
    bash tests/fixtures/build_fixtures.sh

status=0
"$BUILD_DIR/machgate" tests/fixtures/darwin_launch_context first second \
    2>/dev/null || status=$?
[ "$status" -eq 0 ]
