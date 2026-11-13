#!/usr/bin/env bash
# Freeze the legacy persisted offsets that the refactor must not change.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
binary="$(mktemp /tmp/legacy-probe-XXXXXX)"
trap 'rm -f "${binary}"' EXIT

gcc -std=gnu11 -DWIN32 -I "${root}" -I "${root}/nidavellir/include" \
    -I "${root}/targets/stategrid_meter" \
    -I "${root}/targets/stategrid_meter/app" \
    "${root}/tools/legacy_probe.c" -o "${binary}"

output="$("${binary}")"
printf '%s\n' "${output}" | grep -Eq '^sizeof\(struct mbase_control_block_t\) = 1368$'
printf '%s\n' "${output}" | grep -Eq '^persistent_section_start +849$'
printf '%s\n' "${output}" | grep -Eq '^active_p +852$'
printf '%s\n' "${output}" | grep -Eq '^active_n +904$'

echo "persistence offsets unchanged (size 1368, persistent 849, active_p 852)"
