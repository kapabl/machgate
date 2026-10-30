#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

TEST_BINARY="$(mktemp)"
trap 'rm -f "$TEST_BINARY"' EXIT

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Isrc \
    src/domain/guest_process.c tests/guest_process_domain_test.c \
    -o "$TEST_BINARY"
"$TEST_BINARY"
