#!/bin/bash
# Test: a file-backed, nonmapped __DWARF segment does not participate in VM sizing.
# Reference: LLVM Mach-O links debug sections into a zero-vmsize, zero-protection
# segment whose file contents remain available to offline debug consumers.
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/darwin_zero_vmsize_debug ] || \
    bash tests/fixtures/build_fixtures.sh

status=0
"$BUILD_DIR/machgate" tests/fixtures/darwin_zero_vmsize_debug \
    2>/dev/null || status=$?
[ "$status" -eq 42 ]
