#!/bin/bash
# Test: machgate extracts arm64 slice from fat (universal) binary
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/fat_binary ] || bash tests/fixtures/build_fixtures.sh

# The fat binary has x86_64 (exits 99) and arm64 (exits 42).
# On aarch64, machgate should select the arm64 slice and exit 42.
status=0
"$BUILD_DIR/machgate" tests/fixtures/fat_binary 2>/dev/null || status=$?
[ $status -eq 42 ]
