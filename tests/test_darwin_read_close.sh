#!/bin/bash
# Test: Darwin ARM64 SYS_read and SYS_close via svc #0x80
# Apple references:
# - apple-oss-distributions/xnu/bsd/kern/syscalls.master: SYS_read is 3, SYS_close is 6
# - apple-oss-distributions/xnu/libsyscall/custom/SYS.h: ARM64 wrappers use x16 and svc #0x80
set -e
cd "$(dirname "$0")/.."
MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"
[ -f tests/fixtures/darwin_read_close ] || bash tests/fixtures/build_fixtures.sh
output="$(printf 'READCLOS' | "$BUILD_DIR/machismo" tests/fixtures/darwin_read_close 2>/dev/null)"
[ "$output" = "READCLOS" ]
