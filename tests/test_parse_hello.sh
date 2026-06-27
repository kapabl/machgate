#!/bin/bash
# Test: machgate runs hello_write, captures stdout
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/hello_write ] || bash tests/fixtures/build_fixtures.sh
output=$("$BUILD_DIR/machgate" tests/fixtures/hello_write 2>/dev/null)
[ "$output" = "hello" ]
