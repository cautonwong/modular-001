#!/usr/bin/env bash
set -euo pipefail

RUNNER="$1"
DIR="$(mktemp -d)"
trap 'rm -rf "$DIR"' EXIT

first="$("$RUNNER" --backup-files "$DIR" --seconds 130 --no-final-save)"
echo "$first" | grep -q '^profiles=22 '
echo "$first" | grep -q 'pq=2 '
echo "$first" | grep -q 'event_log=1 '

second="$("$RUNNER" --backup-files "$DIR" --seconds 5)"
echo "$second" | grep -q '^profiles=22 '
# The two PQ records were written before the abrupt exit and must be recovered
# from the record ring even though the final metadata flush was skipped.
echo "$second" | grep -q 'pq=2 '
echo "$second" | grep -q 'event_log=1 '
