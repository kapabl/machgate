#!/bin/bash
# Test: Darwin ARM64 SYS_open and SYS_lseek via svc #0x80
# Apple references:
# - apple-oss-distributions/xnu/bsd/kern/syscalls.master: SYS_open is 5, SYS_lseek is 199
# - apple-oss-distributions/xnu/bsd/sys/fcntl.h: Darwin O_RDONLY is 0
set -e
cd "$(dirname "$0")/.."
MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"
[ -f tests/fixtures/darwin_open_lseek ] || bash tests/fixtures/build_fixtures.sh
output="$("$BUILD_DIR/machgate" tests/fixtures/darwin_open_lseek 2>/dev/null)"
[ "$output" = "seek-ok" ]
