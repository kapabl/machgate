#!/bin/bash
# Test: representative Darwin ARM64 syscalls 100-199 via svc #0x80
set -e
cd "$(dirname "$0")/.."

MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
FIXTURE_DIR="$MACHGATE_ROOT/tests/fixtures"
FIXTURE="$FIXTURE_DIR/darwin_range_100_199"
SOURCE="$FIXTURE_DIR/darwin_range_100_199.s"

if [ ! -f "$FIXTURE" ] || [ "$SOURCE" -nt "$FIXTURE" ]; then
    LLVM_MC="${LLVM_MC:-$(command -v llvm-mc || command -v llvm-mc-18 || true)}"
    LD64_LLD="${LD64_LLD:-$(command -v ld64.lld || command -v ld64.lld-18 || true)}"
    [ -n "$LLVM_MC" ] || { echo "llvm-mc not found" >&2; exit 1; }
    [ -n "$LD64_LLD" ] || { echo "ld64.lld not found" >&2; exit 1; }
    "$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
        -o "$FIXTURE_DIR/darwin_range_100_199.o" \
        "$SOURCE"
    "$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
        -o "$FIXTURE" -e _main "$FIXTURE_DIR/darwin_range_100_199.o"
    rm -f "$FIXTURE_DIR/darwin_range_100_199.o"
fi

rm -rf "$FIXTURE_DIR/range_100_199.tmp"
rm -f "$FIXTURE_DIR/range_100_199.file"
rm -f "$FIXTURE_DIR/range_100_199.sock"
cd "$FIXTURE_DIR"
output="$("$BUILD_DIR/machgate" darwin_range_100_199 2>/dev/null)"
[ "$output" = "range100_199" ]
rm -f "$FIXTURE_DIR/range_100_199.file"
rm -f "$FIXTURE_DIR/range_100_199.sock"
