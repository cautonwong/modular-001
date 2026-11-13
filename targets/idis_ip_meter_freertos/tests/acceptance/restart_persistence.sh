#!/usr/bin/env bash
set -euo pipefail

RUNNER="$1"
DIR="$(mktemp -d)"
trap 'rm -rf "$DIR"' EXIT

first="$("$RUNNER" --backup-files "$DIR" --seconds 60)"
second="$("$RUNNER" --backup-files "$DIR" --seconds 5)"

echo "$first" | grep -q '^profiles=22 '
echo "$first" | grep -q 'alarm=0x00000002 '
echo "$second" | grep -q '^profiles=22 '
echo "$second" | grep -q 'alarm=0x00000002 '
echo "$second" | grep -q 'max_demand=5000,1000,2000,2000 rate2=5000'
echo "$second" | grep -q 'billing=2 '

energy_first="$(echo "$first" | sed -n 's/^energy_raw=\([0-9-]*\).*/\1/p')"
energy_second="$(echo "$second" | sed -n 's/^energy_raw=\([0-9-]*\).*/\1/p')"
[ -n "$energy_first" ] && [ -n "$energy_second" ]
[ "$energy_second" -ge "$energy_first" ]
