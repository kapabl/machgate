#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

TEST_BINARY="$(mktemp)"
trap 'rm -f "$TEST_BINARY"' EXIT

"${CC:-cc}" -D_GNU_SOURCE -std=gnu11 -Wall -Wextra -Isrc \
    src/native_structor_adapter.c tests/native_structor_classifier_test.c \
    -o "$TEST_BINARY"
"$TEST_BINARY"
