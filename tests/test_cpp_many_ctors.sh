#!/bin/bash
set -e

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"

[ -f tests/fixtures/cpp_many_ctors ] || bash tests/fixtures/build_fixtures.sh

"$BUILD_DIR/machgate" tests/fixtures/cpp_many_ctors 2>/dev/null

if [ "${MACHGATE_TEST_CPP_PUBLIC_REGISTRARS:-0}" = "1" ]; then
    MACHGATE_BUILD_CPP_PUBLIC_REGISTRARS=1 bash tests/fixtures/build_fixtures.sh
fi

if [ -f tests/fixtures/cpp_public_test_registrars ]; then
    "$BUILD_DIR/machgate" tests/fixtures/cpp_public_test_registrars 2>/dev/null
fi
