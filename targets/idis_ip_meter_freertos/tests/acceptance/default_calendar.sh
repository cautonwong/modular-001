#!/usr/bin/env bash
set -euo pipefail

RUNNER="$1"
DIR="$(mktemp -d)"
trap 'rm -rf "$DIR"' EXIT

out="$("$RUNNER" --backup-files "$DIR" --seconds 5 \
    --start-time 2026-01-01T00:00:00)"
echo "$out" | grep -q '^profiles=22 '
echo "$out" | grep -q 'tariff2=0'
tariff1="$(echo "$out" | sed -n 's/^energy_raw=[0-9-]* tariff1=\([0-9-]*\).*/\1/p')"
[ -n "$tariff1" ] && [ "$tariff1" -gt 0 ]
