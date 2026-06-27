#!/bin/bash
# Test: does cmake build succeed?
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
cmake --build "$BUILD_DIR" 2>&1
