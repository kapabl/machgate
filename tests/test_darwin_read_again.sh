#!/bin/bash
# Test: base-path syscall failures report Darwin errno values.
# The Darwin ABI translation contract requires a translated Syscall Outcome:
# the guest must observe Darwin EAGAIN (35). A raw Linux EAGAIN (11) would
# surface as Darwin EDEADLK because the two ABIs assign different meanings.
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/darwin_read_again ] || bash tests/fixtures/build_fixtures.sh

status=0
python3 - "$BUILD_DIR/machgate" tests/fixtures/darwin_read_again <<'EOF' || status=$?
import os
import sys

machgate, fixture = sys.argv[1], sys.argv[2]
read_fd, write_fd = os.pipe()
os.set_blocking(read_fd, False)
os.dup2(read_fd, 0)
os.close(read_fd)
os.set_inheritable(write_fd, True)
os.execv(machgate, [machgate, fixture])
EOF
[ "$status" -eq 42 ] || {
    echo "guest exit $status, expected 42" >&2
    exit 1
}
