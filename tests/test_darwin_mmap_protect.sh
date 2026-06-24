#!/bin/bash
# Test: Darwin ARM64 SYS_mmap, SYS_mprotect, and SYS_munmap via svc #0x80
# Apple references:
# - apple-oss-distributions/xnu/bsd/kern/syscalls.master: SYS_mmap is 197, SYS_mprotect is 74, SYS_munmap is 73
# - apple-oss-distributions/xnu/bsd/sys/mman.h: Darwin PROT_* and MAP_* values
set -e
cd "$(dirname "$0")/.."
MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"
[ -f tests/fixtures/darwin_mmap_protect ] || bash tests/fixtures/build_fixtures.sh
output="$("$BUILD_DIR/machgate" tests/fixtures/darwin_mmap_protect 2>/dev/null)"
[ "$output" = "MMAP" ]
