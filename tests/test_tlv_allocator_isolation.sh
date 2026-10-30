#!/bin/bash
# Test: TLV bookkeeping uses host allocator functions without interposition.
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
probe_directory="$(mktemp -d)"
trap 'rm -rf "$probe_directory"' EXIT

cc -Wall -Wextra -Werror -o "$probe_directory/tlv_allocator_isolation_probe" \
    tests/fixtures/tlv_allocator_isolation_probe.c -ldl

"$probe_directory/tlv_allocator_isolation_probe" \
    "$BUILD_DIR/libsystem_shim.so"
