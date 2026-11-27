#!/usr/bin/env bash
# Run the formal checks that the host has tools for.
#
# --strict makes a missing tool a failure; the GitHub workflow uses it after
# installing CBMC, Frama-C, Java/TLA+ and Lean. Local runs skip what is not
# installed. Frama-C 33 runs only the Eva plugin. WP is not part of this gate:
# the bundle ships the why3 runtime and prover detection data but no prover,
# and with a system z3 or Alt-Ergo the store contracts still time out or come
# back unknown.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STRICT=0
[ "${1:-}" = "--strict" ] && STRICT=1
FAILED=0
MISSING=0

# One lock per worktree, shared with ci_matrix.py and kept outside build/ so
# wiping the build directory does not release a running job.
exec 9>>"$ROOT/.ci.lock"
if command -v flock >/dev/null 2>&1; then
    if ! flock -n 9; then
        echo "formal: another ci.sh or check_formal.sh is running for $ROOT" >&2
        exit 1
    fi
fi
printf '%s\n' "$$" >&9

# Property and statement totals per harness. A check call added or removed,
# or production code reaching new statements, changes these; the mismatch
# message says which pin to update. The Frama-C version is pinned in CI so the
# statement accounting cannot drift.
BYTE_ASSERTIONS=11
BYTE_PRECONDITIONS=10
BYTE_STATEMENTS=130
BYTE_TOTAL_STATEMENTS=130
SECTOR_ASSERTIONS=3
SECTOR_PRECONDITIONS=16
SECTOR_STATEMENTS=312
SECTOR_TOTAL_STATEMENTS=317

# elan installs Lean under ~/.elan/bin; CI already has it on PATH.
if [ -d "${HOME}/.elan/bin" ]; then
    PATH="${HOME}/.elan/bin:${PATH}"
    export PATH
fi

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "formal: missing $1"
        MISSING=1
        return 1
    fi
}

# cbmc_check <name> <expected property count> <cbmc args...>
#
# CBMC exits 0 only when no property fails, so the count is the size of the
# proof. Pinning it makes a dropped check call or harness branch fail instead
# of silently shrinking what was verified.
cbmc_check() {
    local name="$1" want="$2"
    shift 2
    local log count
    log="$(mktemp)"
    if ! cbmc "$@" >"$log" 2>&1; then
        cat "$log"
        echo "formal: $name: cbmc failed"
        rm -f "$log"
        return 1
    fi
    count="$(sed -n 's/^\*\* [0-9][0-9]* of \([0-9][0-9]*\) failed.*/\1/p' "$log" | tail -n 1)"
    if [ "$count" != "$want" ]; then
        tail -n 5 "$log"
        echo "formal: $name: property count is ${count:-missing}, expected $want; update the expected count in tools/check_formal.sh with the harness change"
        rm -f "$log"
        return 1
    fi
    tail -n 1 "$log"
    rm -f "$log"
}

echo "== cbmc: A-XDR length decoder =="
if need cbmc; then
    cbmc_check "A-XDR length decoder" 502 \
        "$ROOT/formal/cbmc/axdr_length.c" "$ROOT/components/protocol/dlms/src/nd_axdr.c" \
        -I "$ROOT" -I "$ROOT/components/protocol/dlms/include" \
        --unwind 1 --bounds-check --pointer-check --div-by-zero-check \
        --signed-overflow-check --conversion-check || FAILED=1
fi

echo "== cbmc: A-XDR fixed-size skipper =="
if need cbmc; then
    cbmc_check "A-XDR fixed-size skipper" 493 \
        "$ROOT/formal/cbmc/axdr_skip.c" "$ROOT/components/protocol/dlms/src/nd_axdr.c" \
        -I "$ROOT" -I "$ROOT/components/protocol/dlms/include" \
        --unwind 1 --bounds-check --pointer-check --div-by-zero-check \
        --signed-overflow-check --conversion-check || FAILED=1
fi

echo "== cbmc: 698 A-XDR bounded length decoder =="
if need cbmc; then
    # The remaining count is symbolic; recursive structured values are covered
    # by tests/unit/test_fr_axdr_codec.c because CBMC 5.12 cannot encode that
    # recursion at a useful size.
    cbmc_check "698 A-XDR bounded length decoder" 324 \
        "$ROOT/formal/cbmc/axdr_698_len.c" \
        "$ROOT/application/adapters/dl698/fr_axdr_codec.c" \
        -I "$ROOT" \
        --unwind 3 --unwinding-assertions --bounds-check --pointer-check \
        --div-by-zero-check --signed-overflow-check --conversion-check || FAILED=1
fi

echo "== cbmc: pulse base record and journal header parsers =="
if need cbmc; then
    cbmc_check "pulse header parsers" 1770 \
        "$ROOT/formal/cbmc/pulse_headers.c" -I "$ROOT" \
        --nondet-static --unwind 16 --bounds-check --pointer-check \
        --div-by-zero-check --signed-overflow-check --conversion-check \
        || FAILED=1
fi

echo "== cbmc: profile record slot reader =="
if need cbmc; then
    # The seed loop fills 128 bytes per slot; --unwind 129 is the first bound
    # whose unwinding assertion holds. --unwinding-assertions makes that
    # bound a checked fact; at 128 the seed_memory loop fails it.
    cbmc_check "profile record slot reader" 871 \
        "$ROOT/formal/cbmc/profile_slot.c" "$ROOT/domain/common/store.c" \
        -I "$ROOT" \
        --nondet-static --unwind 129 --unwinding-assertions \
        --bounds-check --pointer-check \
        --div-by-zero-check --signed-overflow-check --conversion-check \
        || FAILED=1
fi

echo "== cbmc: memory-backed byte store =="
if need cbmc; then
    # An 8-byte store; --unwind 9 is the first bound whose memcpy and
    # memcmp unwinding assertions hold.
    cbmc_check "memory-backed byte store" 242 \
        "$ROOT/formal/cbmc/byte_store.c" "$ROOT/domain/common/store.c" \
        -I "$ROOT" \
        --nondet-static --unwind 9 --unwinding-assertions \
        --bounds-check --pointer-check \
        --div-by-zero-check --signed-overflow-check --conversion-check \
        || FAILED=1
fi

echo "== cbmc: sector store window and read-modify-write =="
if need cbmc; then
    # Two 4-byte sectors in an 8-byte device; --unwind 9 covers the 8-byte
    # memcpy, the read-modify-write chunk loops and the erase loop.
    cbmc_check "sector store window and read-modify-write" 1128 \
        "$ROOT/formal/cbmc/sector_store.c" "$ROOT/domain/common/store.c" \
        -I "$ROOT" \
        --nondet-static --unwind 9 --unwinding-assertions \
        --bounds-check --pointer-check \
        --div-by-zero-check --signed-overflow-check --conversion-check \
        || FAILED=1
fi

# Eva proves the bounded stores by abstract interpretation. Frama-C exits 0
# even when it reports alarms, so the summary is the verdict: an anchored zero
# alarm count (the unanchored form also matched "10 alarms"), no [eva:alarm]
# line (precondition alarms are separate from the summary count), 100%
# function coverage, no unknown or invalid logical property, and the pinned
# statement and property totals. Pinning the totals makes a deleted check call
# fail instead of shrinking the proof silently.
FRAMA_C_BIN="${FRAMA_C:-frama-c}"

summary_value() { # <log> <property key> -> the number before "total"
    awk -v key="$2" '$1 == key && $NF == "total" { print $(NF - 1); exit }' "$1"
}

statement_counts() { # <log> -> "<reached> <total>"
    sed -n 's/.*In these functions, \([0-9][0-9]*\) statements reached (out of \([0-9][0-9]*\)):.*/\1 \2/p' "$1" | tail -n 1
}

# eva_accepts <log> <assertions> <preconditions> <statements> <total statements>
eva_accepts() {
    local log="$1"
    if grep -q '^\[eva:alarm\]' "$log"; then
        return 1
    fi
    if ! grep -Eq '^[[:space:]]*0 alarms generated by the analysis' "$log"; then
        return 1
    fi
    if ! grep -Eq '[0-9]+ functions analyzed \(out of [0-9]+\): 100% coverage' "$log"; then
        return 1
    fi
    if grep -Eq '(Assertions|Preconditions).*[1-9][0-9]* (unknown|invalid)' "$log"; then
        return 1
    fi
    if [ "$(summary_value "$log" Assertions)" != "$2" ]; then
        return 1
    fi
    if [ "$(summary_value "$log" Preconditions)" != "$3" ]; then
        return 1
    fi
    if [ "$(statement_counts "$log")" != "$4 $5" ]; then
        return 1
    fi
    return 0
}

eva() {
    local name="$1" harness="$2" source="$3"
    local want_assertions="$4" want_preconditions="$5"
    local want_statements="$6" want_total="$7"
    local log
    log="$(mktemp)"
    if ! "$FRAMA_C_BIN" -eva -eva-slevel 512 -eva-min-loop-unroll 8 \
        -cpp-extra-args="-I $ROOT" "$harness" "$source" >"$log" 2>&1; then
        cat "$log"
        echo "formal: $name: frama-c failed"
        rm -f "$log"
        return 1
    fi
    if ! eva_accepts "$log" "$want_assertions" "$want_preconditions" \
        "$want_statements" "$want_total"; then
        sed -n '/ANALYSIS SUMMARY/,$p' "$log"
        if [ "$(summary_value "$log" Assertions)" != "$want_assertions" ] ||
            [ "$(summary_value "$log" Preconditions)" != "$want_preconditions" ] ||
            [ "$(statement_counts "$log")" != "$want_statements $want_total" ]; then
            echo "formal: $name: summary totals are $(summary_value "$log" Assertions)/$(summary_value "$log" Preconditions) properties and $(statement_counts "$log") statements; expected $want_assertions/$want_preconditions and $want_statements $want_total; update the expected counts in tools/check_formal.sh with the harness change"
        else
            echo "formal: $name: Eva summary is not clean"
        fi
        rm -f "$log"
        return 1
    fi
    sed -n '/ANALYSIS SUMMARY/,$p' "$log"
    rm -f "$log"
}

# The fixture is an otherwise clean byte-store summary; only the anchored
# alarm count can reject it, so its totals are fixed here rather than shared
# with the live harness pins. Check the fixture itself first: a missing or
# edited fixture would otherwise pass the self-check for the wrong reason.
FIXTURE="$ROOT/formal/frama-c/negative/alarm_count_regression.txt"
echo "== frama-c: Eva summary validator self-check =="
if ! grep -Eq '^[[:space:]]*10 alarms generated by the analysis' "$FIXTURE"; then
    echo "formal: the fixture is not a 10-alarm summary"
    FAILED=1
elif grep -q '^\[eva:alarm\]' "$FIXTURE"; then
    echo "formal: the fixture carries an [eva:alarm] line, so it does not test the anchored count"
    FAILED=1
elif eva_accepts "$FIXTURE" 11 10 130 130; then
    echo "formal: validator accepted a 10-alarm summary; the zero alarm count must be anchored"
    FAILED=1
fi

echo "== frama-c: byte store Eva =="
if need "$FRAMA_C_BIN"; then
    eva "byte store" "$ROOT/formal/frama-c/byte_store_eva.c" \
        "$ROOT/infra/memory/byte_store.c" \
        "$BYTE_ASSERTIONS" "$BYTE_PRECONDITIONS" "$BYTE_STATEMENTS" \
        "$BYTE_TOTAL_STATEMENTS" || FAILED=1
fi

echo "== frama-c: sector store Eva =="
if need "$FRAMA_C_BIN"; then
    eva "sector store" "$ROOT/formal/frama-c/sector_store_eva.c" \
        "$ROOT/infra/memory/sector_store.c" \
        "$SECTOR_ASSERTIONS" "$SECTOR_PRECONDITIONS" "$SECTOR_STATEMENTS" \
        "$SECTOR_TOTAL_STATEMENTS" || FAILED=1
fi

echo "== frama-c: negative mutation cases =="
if need "$FRAMA_C_BIN"; then
    bash "$ROOT/tools/check_formal_negatives.sh" || FAILED=1
fi

# TLA2TOOLS names the jar; CI downloads it to the runner temp. A local run
# may also leave it in formal/tla/ or /tmp (the jar is never committed).
TLA_JAR=""
for candidate in "${TLA2TOOLS:-}" "$ROOT/formal/tla/tla2tools.jar" /tmp/tla2tools.jar; do
    if [ -n "$candidate" ] && [ -f "$candidate" ]; then
        TLA_JAR="$candidate"
        break
    fi
done

if need java && [ -n "$TLA_JAR" ]; then
    echo "== tla+: block transfer state machine =="
    java -XX:+UseParallelGC -cp "$TLA_JAR" tlc2.TLC -cleanup \
        -config "$ROOT/formal/tla/BlockTransfer.cfg" \
        "$ROOT/formal/tla/BlockTransfer.tla" || FAILED=1
    echo "== tla+: energy pulse journal =="
    java -XX:+UseParallelGC -cp "$TLA_JAR" tlc2.TLC -workers auto -cleanup \
        -config "$ROOT/formal/tla/EnergyPulse.cfg" \
        "$ROOT/formal/tla/EnergyPulse.tla" || FAILED=1
    echo "== tla+: profile GET two-pass =="
    java -XX:+UseParallelGC -cp "$TLA_JAR" tlc2.TLC -cleanup \
        -config "$ROOT/formal/tla/ProfileGet.cfg" \
        "$ROOT/formal/tla/ProfileGet.tla" || FAILED=1
    echo "== tla+: energy snapshot dual slot =="
    java -XX:+UseParallelGC -cp "$TLA_JAR" tlc2.TLC -cleanup \
        -config "$ROOT/formal/tla/Snapshot.cfg" \
        "$ROOT/formal/tla/Snapshot.tla" || FAILED=1
else
    [ -n "$TLA_JAR" ] || echo "formal: missing TLA+ tools (TLA2TOOLS, formal/tla/tla2tools.jar or /tmp/tla2tools.jar)"
    MISSING=1
fi

echo "== lean: FIFO, pulse, profile and energy snapshot arithmetic =="
if need lean && need lake; then
    (cd "$ROOT/formal/lean" && lake build) || FAILED=1
    python3 "$ROOT/tools/check_lean_manifest.py" || FAILED=1
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
