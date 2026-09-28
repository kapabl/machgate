#!/bin/bash
set -e

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"

[ -f tests/fixtures/darwin_init_offsets ] || \
    bash tests/fixtures/build_fixtures.sh

"$BUILD_DIR/machgate" tests/fixtures/darwin_init_offsets 2>/dev/null
