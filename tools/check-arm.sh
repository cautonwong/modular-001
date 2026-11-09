#!/usr/bin/env bash
# Compile domain and application for Cortex-M0 to keep them free of host libc.
# application/upgrade is skipped until its legacy nidavellir.h dependency is
# replaced; the legacy baseline still compiles it.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
flags=(-std=c11 -mcpu=cortex-m0 -mthumb -ffreestanding -Wall -Wextra -Werror -I"$root")
skipped=("$root/application/upgrade")

is_skipped() {
    local path="$1"
    for dir in "${skipped[@]}"; do
        [[ "$path" == "$dir"/* ]] && return 0
    done
    return 1
}

status=0
while IFS= read -r -d '' src; do
    is_skipped "$src" && continue
    arm-none-eabi-gcc "${flags[@]}" -c "$src" -o /dev/null || status=1
done < <(find "$root/domain" "$root/application" -name '*.c' -print0 2>/dev/null)

while IFS= read -r -d '' header; do
    is_skipped "$header" && continue
    arm-none-eabi-gcc "${flags[@]}" -x c -fsyntax-only "$header" || status=1
done < <(find "$root/domain" "$root/application" -name '*.h' -print0 2>/dev/null)

echo "skip: application/upgrade (legacy nidavellir.h dependency)"
exit "$status"
