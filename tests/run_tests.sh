#!/bin/bash
# Test runner for machgate and wrapgen
cd "$(dirname "$0")/.."

export MACHGATE_ROOT="$(pwd)"
export BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"

PASS=0
FAIL=0
TOTAL=0

for test in tests/test_*.sh; do
    TOTAL=$((TOTAL+1))
    name=$(basename "$test" .sh)
    output=$(bash "$test" 2>&1)
    if [ $? -eq 0 ]; then
        echo "PASS: $name"
        PASS=$((PASS+1))
    else
        echo "FAIL: $name"
        echo "  $output" | head -5
        FAIL=$((FAIL+1))
    fi
done

echo "---"
echo "$PASS/$TOTAL passed, $FAIL failed"
[ $FAIL -eq 0 ]
