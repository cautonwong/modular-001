#!/usr/bin/env bash
set -euo pipefail

RUNNER="$1"
DIR="$(mktemp -d)"
trap 'rm -rf "$DIR"' EXIT

out="$("$RUNNER" --backup-files "$DIR" --seconds 10 \
    --start-time 2026-02-28T23:59:55)"
echo "$out" | grep -q '^profiles=22 '
# The default billing single action fires at 2026-03-01 00:00:00; the host's
# end-of-run billing capture then adds the second record.
echo "$out" | grep -q 'billing=2 '
echo "$out" | grep -q 'ec2=1 '
