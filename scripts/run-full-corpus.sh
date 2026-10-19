#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "$0")/.." && pwd)"
provider_directory="${MACHGATE_LIBCXX_DIR:-/home/kapablanka/machgate-libcxx-20.1.8-arm64-20260927/build-libcxx/lib}"
container_image="${MACHGATE_ARM64_IMAGE:-machgate-arm64-toolchain:latest}"
probe_timeout="${MACHGATE_EXTERNAL_TIMEOUT:-120}"

[ -f "$provider_directory/libc++.so.1" ] || {
    echo "libc++ provider not found: $provider_directory/libc++.so.1" >&2
    exit 1
}

[ -f "$provider_directory/libc++abi.so.1" ] || {
    echo "libc++abi provider not found: $provider_directory/libc++abi.so.1" >&2
    exit 1
}

docker run --rm --init --platform linux/arm64 \
    --mount type=bind,src="$repository_root",dst=/work \
    --mount type=bind,src="$provider_directory",dst=/provider,readonly \
    --workdir /work \
    --env MACHGATE_EXTERNAL_TIMEOUT="$probe_timeout" \
    "$container_image" \
    /bin/bash -lc '
        set -euo pipefail
        cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug
        cmake --build build-arm64 --parallel
        export MACHGATE_ROOT=/work
        export BUILD_DIR=/work/build-arm64
        export MACHGATE_RUN_EXTERNAL=1
        export MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/extended_arm64_macho_cli_manifest.txt
        export MACHGATE_EXTERNAL_CACHE=/work/tests/external/cache
        export MACHGATE_EXTERNAL_WORK=/work/tests/external/work
        export MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs
        export MACHGATE_EXTERNAL_REQUIRE_LIBCXX=1
        export MACHGATE_EXTERNAL_LIBCXX=/provider/libc++.so.1
        bash tests/test_external_macho_cli.sh
    '
