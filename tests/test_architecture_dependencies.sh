#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

application_includes="$(
    rg -n '^#include "' src/application --glob '*.[ch]' |
        rg -v '#include "(application|domain|ports)/' || true
)"

if [ -n "$application_includes" ]; then
    printf '%s\n' "application layer depends on outward implementation headers:" >&2
    printf '%s\n' "$application_includes" >&2
    exit 1
fi

domain_includes="$(
    rg -n '^#include "' src/domain --glob '*.[ch]' |
        rg -v '#include "domain/' || true
)"

if [ -n "$domain_includes" ]; then
    printf '%s\n' "domain layer depends on outward implementation headers:" >&2
    printf '%s\n' "$domain_includes" >&2
    exit 1
fi

if rg -n '^struct machgate_guest_image[[:space:]]*\{' \
    src/domain/guest_image.h >/dev/null; then
    printf '%s\n' "Guest Image representation is exposed by the domain header" >&2
    exit 1
fi

outward_layers='(adapters|ports|syscall|shim)'
adapters_application_includes="$(
    rg -n '^#include "application/' src/adapters src/syscall src/shim src/ports \
        --glob '*.[ch]' || true
)"
if [ -n "$adapters_application_includes" ]; then
    printf '%s\n' "outward implementation layers depend on the application layer:" >&2
    printf '%s\n' "$adapters_application_includes" >&2
    exit 1
fi

ports_includes="$(
    rg -n '^#include "' src/ports --glob '*.[ch]' |
        rg -v '#include "(domain|ports)/' || true
)"
if [ -n "$ports_includes" ]; then
    printf '%s\n' "ports depend on anything beyond the domain and their own interfaces:" >&2
    printf '%s\n' "$ports_includes" >&2
    exit 1
fi

application_link_line="$(
    sed -n 's/.*target_link_libraries(machgate_application\(.*\)).*/\1/p' CMakeLists.txt
)"
case "$application_link_line" in
    *system_shim*|*vm_interpose*|*machgate\ *)
        printf '%s\n' "application target links an outward implementation target:" \
            "$application_link_line" >&2
        exit 1
        ;;
esac
case "$application_link_line" in
    *machgate_domain*) ;;
    *)
        printf '%s\n' "application target does not link the domain target:" \
            "$application_link_line" >&2
        exit 1
        ;;
esac

