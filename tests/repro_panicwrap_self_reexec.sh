#!/bin/bash
# Focused Loop N reproduction for Packer panicwrap self-reexec shape.
set -e
cd "$(dirname "$0")/.."

MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"
export BUILD_DIR

if [ ! -f tests/fixtures/darwin_panicwrap_self_reexec ] || \
    [ tests/fixtures/darwin_panicwrap_self_reexec.s -nt tests/fixtures/darwin_panicwrap_self_reexec ]; then
    bash tests/fixtures/build_fixtures.sh
fi

stdout_file="$(mktemp)"
stderr_file="$(mktemp)"
trap 'rm -f "$stdout_file" "$stderr_file"' EXIT

status=0
timeout "${REPRO_TIMEOUT:-10}" \
    bash -c 'cd tests/fixtures && "$BUILD_DIR/machgate" darwin_panicwrap_self_reexec' \
    >"$stdout_file" 2>"$stderr_file" || status=$?

if [ "$status" -ne 0 ]; then
    echo "darwin_panicwrap_self_reexec exited $status" >&2
    echo "stdout:" >&2
    sed -n '1,20p' "$stdout_file" >&2
    echo "stderr:" >&2
    sed -n '1,20p' "$stderr_file" >&2
    exit 1
fi

stdout="$(cat "$stdout_file")"

if [ "$stdout" != "" ]; then
    echo "darwin_panicwrap_self_reexec leaked stdout:" >&2
    sed -n '1,20p' "$stdout_file" >&2
    exit 1
fi
