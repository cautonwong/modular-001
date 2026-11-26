#!/usr/bin/env bash
# Executable Frama-C negatives: mutate a temporary source copy, run the same
# Eva command as the positive gate, and require the exact alarm count and
# location. The unmutated control must report zero alarms. A mutation that
# does not change the file, a wrong source or a missing alarm fails.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
frama="${FRAMA_C:-frama-c}"
cases="${FORMAL_NEGATIVE_CASES:-$root/formal/frama-c/negative/cases.txt}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
failed=0

run_eva() {
    if ! "$frama" -eva -eva-slevel 512 -eva-min-loop-unroll 8 \
        -cpp-extra-args="-I $root" "$1" "$2" >"$3" 2>&1; then
        echo "negative: frama-c failed for $1 $2"
        cat "$3"
        return 1
    fi
}

while IFS='@' read -r name harness source want_count want_location mutation; do
    case "$name" in ''|'#'*) continue ;; esac
    dir="$tmp/$name"
    mkdir -p "$dir"
    harness_copy="$dir/$(basename "$harness")"
    source_copy="$dir/$(basename "$source")"
    if ! cp "$root/$harness" "$harness_copy" \
        || ! cp "$root/$source" "$source_copy"; then
        echo "negative: $name: missing harness or source"
        failed=1
        continue
    fi
    before="$(sha256sum "$source_copy" | cut -d' ' -f1)"
    sed -i "$mutation" "$source_copy"
    after="$(sha256sum "$source_copy" | cut -d' ' -f1)"
    if [ "$before" = "$after" ]; then
        echo "negative: $name: mutation did not change the source"
        failed=1
        continue
    fi
    if ! run_eva "$harness_copy" "$source_copy" "$dir/mutated.log"; then
        failed=1
        continue
    fi
    alarms="$(grep -c '^\[eva:alarm\]' "$dir/mutated.log" || true)"
    if [ "$alarms" != "$want_count" ]; then
        echo "negative: $name: expected $want_count alarm(s), got $alarms"
        failed=1
        continue
    fi
    alarm_context="$(grep -A2 '^\[eva:alarm\]' "$dir/mutated.log" || true)"
    if ! grep -qF "$want_location" <<< "$alarm_context"; then
        echo "negative: $name: no alarm at '$want_location'"
        failed=1
        continue
    fi
    if ! run_eva "$root/$harness" "$root/$source" "$dir/control.log"; then
        failed=1
        continue
    fi
    alarms="$(grep -c '^\[eva:alarm\]' "$dir/control.log" || true)"
    if [ "$alarms" != "0" ]; then
        echo "negative: $name: control copy produced $alarms alarm(s)"
        failed=1
        continue
    fi
    echo "negative ok: $name ($want_count alarm at $want_location; control 0)"
done < "$cases"

if [ "$failed" -ne 0 ]; then
    echo "frama-c negatives: FAILED"
    exit 1
fi
echo "frama-c negatives ok"
