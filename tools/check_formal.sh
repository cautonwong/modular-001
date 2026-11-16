#!/usr/bin/env bash
# Run the formal checks that the host has tools for.
#
# --strict makes a missing tool a failure; the GitHub workflow uses it after
# installing CBMC, Java/TLA+ and Lean. Local runs skip what is not installed.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STRICT=0
[ "${1:-}" = "--strict" ] && STRICT=1
FAILED=0
MISSING=0

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "formal: missing $1"
        MISSING=1
        return 1
    fi
}

echo "== cbmc: A-XDR length decoder =="
if need cbmc; then
    cbmc "$ROOT/formal/cbmc/axdr_length.c" "$ROOT/components/protocol/dlms/src/nd_axdr.c" \
        -I "$ROOT" -I "$ROOT/components/protocol/dlms/include" \
        --unwind 1 --bounds-check --pointer-check --div-by-zero-check \
        --signed-overflow-check --conversion-check || FAILED=1
fi

echo "== cbmc: A-XDR fixed-size skipper =="
if need cbmc; then
    cbmc "$ROOT/formal/cbmc/axdr_skip.c" "$ROOT/components/protocol/dlms/src/nd_axdr.c" \
        -I "$ROOT" -I "$ROOT/components/protocol/dlms/include" \
        --unwind 1 --bounds-check --pointer-check --div-by-zero-check \
        --signed-overflow-check --conversion-check || FAILED=1
fi

echo "== tla+: block transfer state machine =="
TLA_JAR="${TLA2TOOLS:-$ROOT/formal/tla/tla2tools.jar}"
if need java && [ -f "$TLA_JAR" ]; then
    java -cp "$TLA_JAR" tlc2.TLC \
        -config "$ROOT/formal/tla/BlockTransfer.cfg" \
        "$ROOT/formal/tla/BlockTransfer.tla" || FAILED=1
else
    [ -f "$TLA_JAR" ] || echo "formal: missing TLA+ tools at $TLA_JAR"
    MISSING=1
fi

echo "== lean: FIFO capacity arithmetic =="
if need lean && need lake; then
    (cd "$ROOT/formal/lean" && lake build) || FAILED=1
fi

if [ "$FAILED" -ne 0 ]; then
    echo "formal: FAILED"
    exit 1
fi
if [ "$MISSING" -ne 0 ] && [ "$STRICT" -eq 1 ]; then
    echo "formal: missing tools in strict mode"
    exit 1
fi
echo "formal: ok"
