#!/usr/bin/env bash
# Compile domain and application for Cortex-M0 to keep them free of host libc.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
flags=(-std=c11 -mcpu=cortex-m0 -mthumb -ffreestanding -Wall -Wextra -Werror -I"$root")

status=0
while IFS= read -r -d '' src; do
    arm-none-eabi-gcc "${flags[@]}" -c "$src" -o /dev/null || status=1
done < <(find "$root/domain" "$root/application" -name '*.c' -print0 2>/dev/null)

while IFS= read -r -d '' header; do
    arm-none-eabi-gcc "${flags[@]}" -x c -fsyntax-only "$header" || status=1
done < <(find "$root/domain" "$root/application" -name '*.h' -print0 2>/dev/null)

exit "$status"
