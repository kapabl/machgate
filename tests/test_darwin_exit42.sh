#!/bin/bash
# Test: Darwin ARM64 SYS_exit via svc #0x80 exits with status 42
# Apple references:
# - apple-oss-distributions/xnu/bsd/kern/syscalls.master: SYS_exit is 1
# - apple-oss-distributions/xnu/libsyscall/custom/SYS.h: ARM64 wrappers use x16 and svc #0x80
set -e
cd "$(dirname "$0")/.."
MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"
[ -f tests/fixtures/darwin_exit42 ] || bash tests/fixtures/build_fixtures.sh
status=0
"$BUILD_DIR/machgate" tests/fixtures/darwin_exit42 2>/dev/null || status=$?
[ $status -eq 42 ]
