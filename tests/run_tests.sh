#!/bin/bash
set -uo pipefail

cd "$(dirname "$0")/.."
export MACHGATE_ROOT="$(pwd)"
case "${BUILD_DIR:-}" in
    "") export BUILD_DIR="$MACHGATE_ROOT/build" ;;
    /*) export BUILD_DIR ;;
    *) export BUILD_DIR="$MACHGATE_ROOT/$BUILD_DIR" ;;
esac

test_files=(tests/test_*.sh)
passed_count=0
failed_count=0

for test_file in "${test_files[@]}"; do
    test_name="$(basename "$test_file" .sh)"
    if test_output="$(bash "$test_file" 2>&1)"; then
        printf 'PASS: %s\n' "$test_name"
        passed_count=$((passed_count + 1))
    else
        printf 'FAIL: %s\n' "$test_name"
        printf '%s\n' "$test_output" | sed -n '1,5s/^/  /p'
        failed_count=$((failed_count + 1))
    fi
done

printf '%s\n' '---'
printf '%d/%d passed, %d failed\n' \
    "$passed_count" "${#test_files[@]}" "$failed_count"
((failed_count == 0))
