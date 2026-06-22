#!/bin/bash
# Test: Darwin ARM64 SYS_write via svc #0x80 writes to stdout
# Apple references:
# - apple-oss-distributions/xnu/bsd/kern/syscalls.master: SYS_write is 4
# - apple-oss-distributions/xnu/libsyscall/custom/SYS.h: ARM64 wrappers use x16 and svc #0x80
set -e
cd "$(dirname "$0")/.."
MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"
[ -f tests/fixtures/darwin_write_stdout ] || bash tests/fixtures/build_fixtures.sh
output="$("$BUILD_DIR/machismo" tests/fixtures/darwin_write_stdout 2>/dev/null)"
[ "$output" = "darwin write" ]
