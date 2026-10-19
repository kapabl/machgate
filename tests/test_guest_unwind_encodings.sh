#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
REGISTER_FIXTURE="tests/fixtures/dynamic_guest_unwind_registers"
EH_FRAME_FIXTURE="tests/fixtures/dynamic_guest_unwind_eh_frame"
LLVM_OBJDUMP="${LLVM_OBJDUMP:-$(command -v llvm-objdump || command -v llvm-objdump-18 || true)}"

[ -n "$LLVM_OBJDUMP" ] || { echo "llvm-objdump not found" >&2; exit 1; }
[ -f "$REGISTER_FIXTURE" ] && [ -f "$EH_FRAME_FIXTURE" ] || \
	bash tests/fixtures/build_fixtures.sh

register_unwind_info="$($LLVM_OBJDUMP --macho --unwind-info "$REGISTER_FIXTURE")"
grep -q '0x04000101' <<<"$register_unwind_info"
grep -q '0x02002101' <<<"$register_unwind_info"
grep -q '0x00000000' <<<"$register_unwind_info"

eh_frame_sections="$($LLVM_OBJDUMP --macho --section-headers "$EH_FRAME_FIXTURE")"
grep -q '__eh_frame' <<<"$eh_frame_sections"
if grep -q '__unwind_info' <<<"$eh_frame_sections"; then
	echo "eh-frame-only fixture unexpectedly contains compact unwind metadata" >&2
	exit 1
fi

for fixture in "$REGISTER_FIXTURE" "$EH_FRAME_FIXTURE"; do
	MACHGATE_SYSTEM_SHIM_LIB="$BUILD_DIR/libsystem_shim.so" \
		timeout -k 2 10 "$BUILD_DIR/machgate" "$fixture"
done
