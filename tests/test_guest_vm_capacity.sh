#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

TEST_BINARY="$(mktemp)"
trap 'rm -f "$TEST_BINARY"' EXIT

"${CC:-cc}" -D_GNU_SOURCE -std=gnu11 -Wall -Wextra -Isrc \
    src/syscall/guest_vm_track.c tests/guest_vm_capacity_test.c \
    -pthread -o "$TEST_BINARY"
"$TEST_BINARY"

