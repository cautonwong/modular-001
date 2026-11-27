#!/usr/bin/env python3
"""Check the three product budgets and record their measured values.

`docs/design/timing-budget.md` is the record. This check verifies that the
table names the three budgets and fills every field, runs the simulator
measurement, and prints the numbers so the CI log carries the current values.
It sets no performance threshold: a budget is a number to watch, not a gate.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUDGET_DOC = Path("docs/design/timing-budget.md")
BUDGET_IDS = ("PERF", "POWER", "FAILSAFE")
FIELDS = ("目标", "测量方法", "基线", "当前值", "HIL 到期日")
ISO_DATE = re.compile(r"\d{4}-\d{2}-\d{2}")
STATS_PREFIX = "[sim] stats "
MEASURE_CYCLES = 100
MEASURE_SCENARIO = "at 60 power off\nat 80 power on\n"
# Values that the recorded [sim] stats line must reproduce. The stack number is
# host and libc dependent, so it is printed but not compared.
RECORDED = ("stage-max", "wake-latency-passes", "power-off-entry-passes",
            "watchdog-max-gap-passes")
NUMERIC = ("cycles", "stages", "stage-max", "watchdog-kicks",
           "watchdog-max-gap-passes", "stack-high-water-bytes")


def split_row(line: str) -> list[str] | None:
    stripped = line.strip()
    if not stripped.startswith("|"):
        return None
    return [cell.strip() for cell in stripped.strip("|").split("|")]


def budget_table(text: str) -> tuple[list[str] | None, list[list[str]]]:
    lines = text.splitlines()
    for index, line in enumerate(lines):
        header = split_row(line)
        if not header or header[0] != "ID":
            continue
        rows = []
        for later in lines[index + 1:]:
            cells = split_row(later)
            if cells is None:
                break
            rows.append(cells)
        return header, rows
    return None, []


def doc_errors(text: str) -> list[str]:
    errors: list[str] = []
    header, rows = budget_table(text)
    if header is None:
        return [f"{BUDGET_DOC}: no budget table with an ID header"]
    missing = [field for field in FIELDS if field not in header]
    if missing:
        return [f"{BUDGET_DOC}: table lacks {', '.join(missing)}"]
    column = {field: header.index(field) for field in FIELDS}
    seen: dict[str, list[str]] = {}
    for cells in rows:
        if not cells or cells[0] not in BUDGET_IDS:
            continue
        if len(cells) != len(header):
            errors.append(f"{BUDGET_DOC}: {cells[0]} row has {len(cells)} "
                          f"cells, header has {len(header)}")
            continue
        seen[cells[0]] = cells
        for field in FIELDS:
            if not cells[column[field]]:
                errors.append(f"{BUDGET_DOC}: {cells[0]} has an empty {field}")
        current = cells[column["当前值"]]
        if re.search(r"verified|已验证", current, re.IGNORECASE):
            errors.append(f"{BUDGET_DOC}: {cells[0]} claims a HIL verification; "
                          "no HIL evidence exists")
        due = cells[column["HIL 到期日"]]
        if not ISO_DATE.search(due) and "TBD" not in due.upper():
            errors.append(f"{BUDGET_DOC}: {cells[0]} HIL due date is neither "
                          "YYYY-MM-DD nor TBD")
    for budget in BUDGET_IDS:
        if budget not in seen:
            errors.append(f"{BUDGET_DOC}: missing budget {budget}")
    if STATS_PREFIX not in text:
        errors.append(f"{BUDGET_DOC}: no recorded {STATS_PREFIX.strip()} line")
    if "tools/check_budgets.py" not in text:
        errors.append(f"{BUDGET_DOC}: measurement command "
                      "tools/check_budgets.py is not named")
    return errors


def parse_stats(lines: list[str]) -> dict[str, str] | None:
    for line in lines:
        if not line.startswith(STATS_PREFIX):
            continue
        values: dict[str, str] = {}
        for part in line[len(STATS_PREFIX):].split():
            key, sep, value = part.partition("=")
            if sep:
                values[key] = value
        return values
    return None


def stats_errors(values: dict[str, str]) -> list[str]:
    errors: list[str] = []
    for key in NUMERIC:
        if key not in values:
            errors.append(f"[sim] stats lacks {key}")
        elif not values[key].isdigit():
            errors.append(f"[sim] stats {key} is not a number: {values[key]}")
    for key in ("wake-latency-passes", "power-off-entry-passes"):
        if values.get(key) == "none":
            errors.append(f"[sim] stats {key} was not measured; the scenario "
                          "must include power off and power on")
        elif key in values and not values[key].isdigit():
            errors.append(f"[sim] stats {key} is not a number: {values[key]}")
    if values.get("cycles") != str(MEASURE_CYCLES):
        errors.append(f"[sim] stats cycles={values.get('cycles')}, expected "
                      f"{MEASURE_CYCLES}")
    if values.get("stage-max") == "0":
        errors.append("[sim] stats stage-max=0; the live pass never ran")
    if values.get("stack-high-water-bytes") == "0":
        errors.append("[sim] stats stack-high-water-bytes=0; use the host build, "
                      "not the ASan build")
    return errors


def recorded_errors(text: str, values: dict[str, str]) -> list[str]:
    recorded = parse_stats(text.splitlines())
    if recorded is None:
        return [f"{BUDGET_DOC}: no recorded [sim] stats line"]
    errors: list[str] = []
    for key in RECORDED:
        if recorded.get(key) != values.get(key):
            errors.append(f"{BUDGET_DOC}: recorded {key}={recorded.get(key)} "
                          f"but the measurement prints {values.get(key)}")
    return errors


def measure(root: Path, sim: Path) -> dict[str, str]:
    with tempfile.TemporaryDirectory(prefix="budgets-") as tmp:
        work = Path(tmp)
        scenario = work / "scenario.txt"
        scenario.write_text(MEASURE_SCENARIO, encoding="utf-8")
        trace = work / "trace.txt"
        command = [
            str(sim),
            "--cycles", str(MEASURE_CYCLES),
            "--eeprom", str(work / "eeprom.bin"),
            "--flash", str(work / "flash.bin"),
            "--scenario", str(scenario),
            "--start-time", "2021-04-30 12:00:00",
            "--trace", str(trace),
        ]
        started = time.monotonic()
        completed = subprocess.run(command, cwd=root, capture_output=True,
                                   text=True, timeout=120, check=False)
        wall_ms = (time.monotonic() - started) * 1000
        lines = completed.stdout.splitlines() + completed.stderr.splitlines()
        if trace.is_file():
            lines += trace.read_text(encoding="utf-8", errors="replace").splitlines()
        if completed.returncode != 0:
            tail = "\n".join(completed.stderr.splitlines()[-5:])
            raise SystemExit(f"budgets: firmware_sim exited "
                             f"{completed.returncode}\n{tail}")
        values = parse_stats(lines)
        if values is None:
            raise SystemExit("budgets: firmware_sim printed no [sim] stats line")
        values["host-wall-ms"] = f"{wall_ms:.0f}"
        return values


VALID_RECORD = """# timing budget

| ID | 目标 | 测量方法 | 基线 | 当前值 | HIL 到期日 |
| --- | --- | --- | --- | --- | --- |
| PERF | one pass | `tools/check_budgets.py` | e43b56f9 | 14 stages per pass | 2026-12-31 |
| POWER | one pass | `tools/check_budgets.py` | e43b56f9 | wake 1 pass | 2026-12-31 |
| FAILSAFE | TBD | `tools/check_budgets.py` | e43b56f9 | 0 passes | 2026-12-31 |

[sim] stats cycles=100 stage-max=14 wake-latency-passes=1 power-off-entry-passes=0 watchdog-max-gap-passes=1
"""


def self_check() -> list[str]:
    errors: list[str] = []
    if doc_errors(VALID_RECORD):
        errors.append("self-check: the valid record was rejected")
    variants = {
        "missing budget": VALID_RECORD.replace(
            "| FAILSAFE | TBD | `tools/check_budgets.py` | e43b56f9 | "
            "0 passes | 2026-12-31 |\n", ""),
        "empty field": VALID_RECORD.replace(
            "| wake 1 pass | 2026-12-31 |", "| wake 1 pass |  |"),
        "verified claim": VALID_RECORD.replace(
            "| 0 passes |", "| verified |"),
        "no stats line": VALID_RECORD.replace("[sim] stats", "stats"),
        "no command": VALID_RECORD.replace("tools/check_budgets.py", "sim"),
    }
    for name, text in variants.items():
        if not doc_errors(text):
            errors.append(f"self-check: accepted the {name} negative")
    good = {"cycles": "100", "stages": "1400", "stage-max": "14",
            "wake-latency-passes": "1", "power-off-entry-passes": "0",
            "watchdog-kicks": "100", "watchdog-max-gap-passes": "1",
            "stack-high-water-bytes": "2568"}
    if stats_errors(good):
        errors.append("self-check: the valid stats were rejected")
    bad = dict(good, **{"stack-high-water-bytes": "0"})
    if not stats_errors(bad):
        errors.append("self-check: accepted a zero stack high-water")
    if not recorded_errors(VALID_RECORD, dict(good, **{"stage-max": "15"})):
        errors.append("self-check: accepted a stale recorded value")
    if recorded_errors(VALID_RECORD, good):
        errors.append("self-check: rejected a matching recorded value")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path,
                        help="repository root; defaults to this script's parent's parent")
    parser.add_argument("--sim", type=Path,
                        help="firmware_sim path; relative paths resolve against root")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    root = (args.root or ROOT).resolve()

    errors = self_check()
    if errors:
        print("\n".join(errors))
        print("budget self-check: FAILED")
        return 1
    if args.self_check:
        print("budget self-check ok")
        return 0

    text = (root / BUDGET_DOC).read_text(encoding="utf-8")
    errors = doc_errors(text)
    if errors:
        print("\n".join(errors))
        print("budgets: FAILED")
        return 1

    if args.sim is None:
        print("budgets: --sim is required for the measurement", file=sys.stderr)
        return 2
    sim = args.sim if args.sim.is_absolute() else root / args.sim
    if not sim.is_file():
        print(f"budgets: {sim} is missing; build firmware_sim first",
              file=sys.stderr)
        return 2

    values = measure(root, sim)
    errors = stats_errors(values) + recorded_errors(text, values)
    if errors:
        print("\n".join(errors))
        print("budgets: FAILED")
        return 1

    print(f"budget: performance cycles={values['cycles']} "
          f"stages={values['stages']} stage-max={values['stage-max']} "
          f"host-wall-ms={values['host-wall-ms']}")
    print(f"budget: power wake-latency-passes={values['wake-latency-passes']} "
          "sleep-current=HIL-pending")
    print(f"budget: failsafe power-off-entry-passes="
          f"{values['power-off-entry-passes']} "
          f"watchdog-max-gap-passes={values['watchdog-max-gap-passes']} "
          "save-window=HIL-pending")
    print(f"budget: stack stack-high-water-bytes="
          f"{values['stack-high-water-bytes']}")
    print("budgets ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
