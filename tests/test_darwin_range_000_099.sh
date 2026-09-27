#!/bin/bash
# Test: representative Darwin ARM64 syscalls from range 0-99 via svc #0x80
# Apple references:
# - apple-oss-distributions/xnu/bsd/kern/syscalls.master: syscall numbers 0-99
# - apple-oss-distributions/xnu/libsyscall/custom/SYS.h: ARM64 wrappers use x16 and svc #0x80
set -e
cd "$(dirname "$0")/.."

MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
LLVM_MC="${LLVM_MC:-$(command -v llvm-mc || command -v llvm-mc-18 || true)}"
LD64_LLD="${LD64_LLD:-$(command -v ld64.lld || command -v ld64.lld-18 || true)}"

[ -n "$LLVM_MC" ] || { echo "llvm-mc not found" >&2; exit 1; }
[ -n "$LD64_LLD" ] || { echo "ld64.lld not found" >&2; exit 1; }
trap 'rm -f tests/fixtures/range000099.link tests/fixtures/darwin_range_000_099.o' EXIT

fixture="tests/fixtures/darwin_range_000_099"
rm -f tests/fixtures/range000099.link tests/fixtures/darwin_range_000_099.o "$fixture"
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o tests/fixtures/darwin_range_000_099.o \
    tests/fixtures/darwin_range_000_099.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o "$fixture" -e _main tests/fixtures/darwin_range_000_099.o
rm -f tests/fixtures/darwin_range_000_099.o

output="$("$BUILD_DIR/machgate" "$fixture" 2>/dev/null)"
[ "$output" = "range000099" ]
