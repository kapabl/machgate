#!/bin/bash
# Test: Darwin ARM64 SYS_fstat via svc #0x80 writes Darwin struct stat
# Apple references:
# - apple-oss-distributions/xnu/bsd/kern/syscalls.master: SYS_fstat is 189
# - apple-oss-distributions/xnu/bsd/sys/stat.h: Darwin LP64 struct stat layout
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/darwin_fstat ] || bash tests/fixtures/build_fixtures.sh
cd tests/fixtures
"$BUILD_DIR/machgate" darwin_fstat 2>/dev/null
