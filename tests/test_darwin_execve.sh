#!/bin/bash
# Test: Darwin SYS_execve re-execs the MachGate loader for Mach-O guests.
set -e
cd "$(dirname "$0")/.."

MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"

[ -f tests/fixtures/darwin_execve_exit42 ] || bash tests/fixtures/build_fixtures.sh

status=0
(cd tests/fixtures && "$BUILD_DIR/machgate" darwin_execve_exit42 2>/dev/null) || status=$?
[ "$status" -eq 42 ]
