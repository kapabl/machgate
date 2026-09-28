#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

manifest=tests/external/extended_arm64_macho_cli_manifest.txt
runner=tests/test_external_macho_cli.sh

[ -f "$manifest" ]
bash -n "$runner"

awk -F'|' '
    /^[[:space:]]*#/ || /^[[:space:]]*$/ { next }
    NF != 5 { invalid_fields++ }
    $3 !~ /^[[:xdigit:]]{64}$/ { invalid_hashes++ }
    { rows++; names[$1]++ }
    END {
        duplicates=0
        for (name in names) {
            if (names[name] != 1) {
                duplicates++
            }
        }
        if (rows != 57 || invalid_fields || invalid_hashes || duplicates) {
            printf "rows=%d invalid_fields=%d invalid_hashes=%d duplicates=%d\n",
                rows, invalid_fields, invalid_hashes, duplicates > "/dev/stderr"
            exit 1
        }
    }
' "$manifest"

grep -q 'TIMEOUT="${MACHGATE_EXTERNAL_TIMEOUT:-120}"' "$runner"
grep -q 'MANIFEST="${MACHGATE_EXTERNAL_MANIFEST:-$MACHGATE_ROOT/tests/external/extended_arm64_macho_cli_manifest.txt}"' "$runner"
grep -q '^libiconv = libc\.so\.6$' "$runner"
grep -q '^libiconv\.2 = libc\.so\.6$' "$runner"
grep -q '^libresolv = .*libsystem_shim\.so$' "$runner"
grep -q '^libresolv\.9 = .*libsystem_shim\.so$' "$runner"
grep -q '^AppKit = SKIP$' "$runner"
grep -q '^CoreFoundation = PARTIAL:.*libsystem_shim\.so$' "$runner"
grep -q '^IOKit = PARTIAL:.*libsystem_shim\.so$' "$runner"
grep -q '^libobjc\.A = SKIP$' "$runner"
grep -q '^libicucore = .*libsystem_shim\.so$' "$runner"
grep -q '^libicucore\.A = SKIP$' "$runner"
grep -q '^CoreServices = PARTIAL:.*libsystem_shim\.so$' "$runner"
grep -Fq 'Machine:[[:space:]]+AArch64' "$runner"

echo "external corpus gate: 57 unique pinned probes"
