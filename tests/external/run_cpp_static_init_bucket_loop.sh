#!/bin/bash
set -uo pipefail

usage()
{
    echo "usage: $0 BOOST_NOTHING_TO_TEST_ABORT|BOOST_AUTO_START_DBG|BOOST_AUTO_START_DBG_ABORT|TIMEOUT_AFTER_MAIN" >&2
    echo "env: MACHGATE_CPP_LOOP_ATTEMPTS=1..5 MACHGATE_CPP_LOOP_TIMEOUT=45 MACHGATE_CPP_LOOP_REQUIRE_ALL_ATTEMPTS=1" >&2
}

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
bucket="${1:-}"
docker_image="${MACHGATE_CPP_LOOP_DOCKER_IMAGE:-machgate-arm64-toolchain}"
docker_platform="${MACHGATE_CPP_LOOP_DOCKER_PLATFORM:-linux/arm64}"
use_docker="${MACHGATE_CPP_LOOP_DOCKER:-1}"
timeout_seconds="${MACHGATE_CPP_LOOP_TIMEOUT:-45}"
max_attempts="${MACHGATE_CPP_LOOP_ATTEMPTS:-5}"
require_all_attempts="${MACHGATE_CPP_LOOP_REQUIRE_ALL_ATTEMPTS:-0}"
skip_build="${MACHGATE_CPP_LOOP_SKIP_BUILD:-0}"
build_each_attempt="${MACHGATE_CPP_LOOP_BUILD_EACH_ATTEMPT:-0}"

case "$bucket" in
    BOOST_NOTHING_TO_TEST_ABORT|boost-nothing-to-test-abort|boost-nothing)
        bucket_name="boost-nothing-to-test-abort"
        manifest_rel="tests/external/cpp_static_init_boost_nothing_to_test_abort_manifest.txt"
        ;;
    BOOST_AUTO_START_DBG|boost-auto-start-dbg|boost)
        bucket_name="boost-auto-start-dbg"
        manifest_rel="tests/external/cpp_static_init_boost_auto_start_dbg_manifest.txt"
        ;;
    BOOST_AUTO_START_DBG_ABORT|boost-auto-start-dbg-abort|boost-abort)
        bucket_name="boost-auto-start-dbg-abort"
        manifest_rel="tests/external/cpp_static_init_boost_auto_start_dbg_abort_manifest.txt"
        ;;
    TIMEOUT_AFTER_MAIN|timeout-after-main|timeout)
        bucket_name="timeout-after-main"
        manifest_rel="tests/external/cpp_static_init_timeout_after_main_manifest.txt"
        ;;
    *)
        usage
        exit 2
        ;;
esac

case "$max_attempts" in
    ''|*[!0-9]*)
        echo "MACHGATE_CPP_LOOP_ATTEMPTS must be an integer from 1 to 5" >&2
        exit 2
        ;;
esac

if [ "$max_attempts" -lt 1 ]; then
    echo "MACHGATE_CPP_LOOP_ATTEMPTS must be at least 1" >&2
    exit 2
fi

if [ "$max_attempts" -gt 5 ]; then
    echo "MACHGATE_CPP_LOOP_ATTEMPTS capped at 5" >&2
    max_attempts=5
fi

manifest_host="$repo_root/$manifest_rel"
if [ ! -f "$manifest_host" ]; then
    echo "manifest not found: $manifest_host" >&2
    exit 1
fi

summary_dir="$repo_root/tests/external/logs"
summary_file="$summary_dir/cpp-static-init-$bucket_name-loop-summary.tsv"
mkdir -p "$summary_dir"
printf "bucket\tattempt\tstatus\tlogs\twork\n" > "$summary_file"

run_build()
{
    if [ "$skip_build" = "1" ]; then
        return 0
    fi

    if [ "$use_docker" = "1" ]; then
        docker run --rm --platform "$docker_platform" \
            -v "$repo_root:/work" \
            -w /work "$docker_image" \
            cmake --build build-arm64 --parallel
        return $?
    fi

    (cd "$repo_root" && cmake --build "${BUILD_DIR:-build-arm64}" --parallel)
}

run_attempt()
{
    local attempt="$1"
    local label="cpp-static-init-$bucket_name-attempt$attempt"
    local host_log_dir="$repo_root/tests/external/logs/$label"
    local host_work_dir="$repo_root/tests/external/work/$label"
    local status

    rm -rf "$host_log_dir" "$host_work_dir"
    mkdir -p "$host_log_dir" "$host_work_dir"

    echo "== $bucket_name attempt $attempt/$max_attempts =="
    echo "logs: $host_log_dir"
    echo "work: $host_work_dir"

    if [ "$build_each_attempt" = "1" ]; then
        run_build || return $?
    fi

    if [ "$use_docker" = "1" ]; then
        docker run --rm --platform "$docker_platform" \
            -v "$repo_root:/work" \
            -w /work \
            -e MACHGATE_RUN_EXTERNAL=1 \
            -e MACHGATE_EXTERNAL_VERBOSE=1 \
            -e MACHGATE_TRACE_SIGNALS=1 \
            -e MACHGATE_TRACE_LCMAIN=1 \
            -e MACHGATE_EXTERNAL_MAP_LIBCXX=1 \
            -e BUILD_DIR=/work/build-arm64 \
            -e MACHGATE_EXTERNAL_MANIFEST="/work/$manifest_rel" \
            -e MACHGATE_EXTERNAL_LOGS="/work/tests/external/logs/$label" \
            -e MACHGATE_EXTERNAL_WORK="/work/tests/external/work/$label" \
            -e MACHGATE_EXTERNAL_TIMEOUT="$timeout_seconds" \
            "$docker_image" \
            bash tests/test_external_macho_cli.sh 2>&1 | tee "$host_log_dir/console.log"
        status=${PIPESTATUS[0]}
    else
        (cd "$repo_root" && \
            MACHGATE_RUN_EXTERNAL=1 \
            MACHGATE_EXTERNAL_VERBOSE=1 \
            MACHGATE_TRACE_SIGNALS=1 \
            MACHGATE_TRACE_LCMAIN=1 \
            MACHGATE_EXTERNAL_MAP_LIBCXX=1 \
            BUILD_DIR="${BUILD_DIR:-$repo_root/build-arm64}" \
            MACHGATE_EXTERNAL_MANIFEST="$manifest_host" \
            MACHGATE_EXTERNAL_LOGS="$host_log_dir" \
            MACHGATE_EXTERNAL_WORK="$host_work_dir" \
            MACHGATE_EXTERNAL_TIMEOUT="$timeout_seconds" \
            bash tests/test_external_macho_cli.sh) 2>&1 | tee "$host_log_dir/console.log"
        status=${PIPESTATUS[0]}
    fi

    if [ "$status" -eq 0 ]; then
        printf "%s\t%s\tPASS\t%s\t%s\n" "$bucket_name" "$attempt" "$host_log_dir" "$host_work_dir" >> "$summary_file"
    else
        printf "%s\t%s\tFAIL:%s\t%s\t%s\n" "$bucket_name" "$attempt" "$status" "$host_log_dir" "$host_work_dir" >> "$summary_file"
    fi

    return "$status"
}

if [ "$build_each_attempt" != "1" ]; then
    run_build || exit $?
fi

pass_count=0
fail_count=0
attempt=1

while [ "$attempt" -le "$max_attempts" ]; do
    if run_attempt "$attempt"; then
        pass_count=$((pass_count + 1))
        if [ "$require_all_attempts" != "1" ]; then
            echo "PASS: $bucket_name attempt $attempt"
            echo "summary: $summary_file"
            exit 0
        fi
    else
        fail_count=$((fail_count + 1))
    fi

    attempt=$((attempt + 1))
done

echo "---"
echo "$bucket_name attempts: pass=$pass_count fail=$fail_count"
echo "summary: $summary_file"

if [ "$fail_count" -eq 0 ]; then
    exit 0
fi

exit 1
