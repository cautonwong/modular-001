#!/usr/bin/env bash
# Freeze the legacy persisted offsets that the refactor must not change.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
binary="$(mktemp /tmp/legacy-probe-XXXXXX)"
trap 'rm -f "${binary}"' EXIT

gcc -std=gnu11 -DWIN32 -I "${root}" \
    -I "${root}/targets/stategrid_meter" \
    -I "${root}/targets/stategrid_meter/app" \
    "${root}/tools/legacy_probe.c" -o "${binary}"

output="$("${binary}")"
# Here-strings, not `printf | grep -q`: with pipefail an early grep exit can
# SIGPIPE the writer under load and turn a passing check into exit 141.
grep -E '^sizeof\(struct mbase_control_block_t\) = 1368$' <<< "${output}" >/dev/null
grep -E '^persistent_section_start +849$' <<< "${output}" >/dev/null
grep -E '^active_p +852$' <<< "${output}" >/dev/null
grep -E '^active_n +904$' <<< "${output}" >/dev/null

echo "persistence offsets unchanged (size 1368, persistent 849, active_p 852)"
