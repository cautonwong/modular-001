#!/usr/bin/env python3
"""Check that every P0/P1 X row of the acceptance matrix has a risk record."""
from __future__ import annotations

import argparse
import datetime
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_acceptance_matrix import rows as matrix_rows

ROOT = Path(__file__).resolve().parents[1]
REGISTER = ROOT / "docs" / "risk-register.md"

RISK_PRIORITIES = {"P0", "P1"}
DECISIONS = {"fix", "accept", "defer"}
HEADER = ["ID", "优先级", "Owner", "原因", "决定", "到期日", "批准人", "证据"]
KEYS = ["id", "priority", "owner", "reason", "decision", "due", "approver",
        "evidence"]
ROW_RE = re.compile(r"^\|\s*(ACC-[A-Z0-9-]+)\s*\|(.*)\|\s*$")
SEPARATOR_RE = re.compile(r"^:?-{2,}:?$")
DATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")


def parse_register(path: Path = REGISTER) -> list[dict[str, str]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    header_at = None
    for index, line in enumerate(lines):
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if cells == HEADER:
            header_at = index
            break
    if header_at is None:
        raise SystemExit(f"{path} has no table header: {' | '.join(HEADER)}")
    records: list[dict[str, str]] = []
    for line in lines[header_at + 1:]:
        if not line.startswith("|"):
            break
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if all(SEPARATOR_RE.match(cell) for cell in cells):
            continue
        match = ROW_RE.match(line)
        if not match:
            raise SystemExit(f"{path} has an unparseable table row: {line}")
        values = [cell.strip() for cell in match.group(2).split("|")]
        if len(values) != len(HEADER) - 1:
            raise SystemExit(
                f"{path} row {match.group(1)} has {len(values) + 1} cells, "
                f"expected {len(HEADER)}")
        records.append(dict(zip(KEYS, [match.group(1)] + values)))
    if not records:
        raise SystemExit(f"{path} has no risk rows")
    return records


def register_errors(
    records: list[dict[str, str]],
    matrix: list[dict[str, str]],
    today: datetime.date | None = None,
) -> list[str]:
    errors: list[str] = []
    today = today or datetime.date.today()
    by_id: dict[str, dict[str, str]] = {}
    for record in records:
        row_id = record["id"]
        if row_id in by_id:
            errors.append(f"{row_id}: duplicate register row")
            continue
        by_id[row_id] = record
        if record["priority"] not in RISK_PRIORITIES:
            errors.append(
                f"{row_id}: priority {record['priority']!r} is not P0 or P1")
        if record["decision"] not in DECISIONS:
            errors.append(
                f"{row_id}: decision {record['decision']!r} is not fix, "
                "accept, or defer")
        due = record["due"]
        try:
            if not DATE_RE.match(due):
                raise ValueError
            due_date = datetime.date.fromisoformat(due)
        except ValueError:
            errors.append(f"{row_id}: due date {due!r} is not YYYY-MM-DD")
        else:
            if record["decision"] in {"fix", "defer"} and due_date < today:
                errors.append(
                    f"{row_id}: {record['decision']} is overdue since {due}")
        if record["decision"] == "accept" and record["approver"] in {"", "-"}:
            errors.append(f"{row_id}: accept needs an approver")
        evidence = Path(record["evidence"])
        if (evidence.is_absolute() or ".." in evidence.parts
                or not (ROOT / evidence).is_file()):
            errors.append(
                f"{row_id}: evidence {record['evidence']!r} is not a "
                "repo-relative file")

    risk_rows = {
        row["id"]: row for row in matrix
        if row["status"] == "X" and row["priority"] in RISK_PRIORITIES
    }
    for row_id in sorted(risk_rows):
        if row_id not in by_id:
            errors.append(f"{row_id}: P0/P1 X row has no register entry")
    matrix_by_id = {row["id"]: row for row in matrix}
    for row_id in sorted(by_id):
        if row_id in risk_rows:
            continue
        row = matrix_by_id.get(row_id)
        if row is None:
            errors.append(f"{row_id}: register row has no matrix row")
        elif row["status"] == "A":
            errors.append(
                f"{row_id}: register row is stale, the matrix row moved to A")
        else:
            errors.append(
                f"{row_id}: register row is not a P0/P1 X row "
                f"(matrix is {row['status']}/{row['priority']})")
    return errors


def self_check() -> list[str]:
    matrix = matrix_rows()
    risk = [row for row in matrix
            if row["status"] == "X" and row["priority"] in RISK_PRIORITIES]
    if not risk:
        return ["self-check: the matrix has no P0/P1 X rows"]
    base = [
        {"id": row["id"], "priority": row["priority"], "owner": "fixture",
         "reason": "fixture", "decision": "defer", "due": "2999-12-31",
         "approver": "-", "evidence": "docs/firmware-acceptance-matrix.md"}
        for row in risk
    ]
    accepted = next(row for row in matrix if row["status"] == "A")
    stale = dict(base[0], id=accepted["id"], priority=accepted["priority"])

    def mutate(index: int, **changes: str) -> list[dict[str, str]]:
        records = [dict(record) for record in base]
        records[index].update(changes)
        return records

    fixtures = [
        ("clean register", base, None),
        ("missing row", base[1:], base[0]["id"]),
        ("duplicate row", base + [dict(base[0])], "duplicate register row"),
        ("overdue defer", mutate(0, due="2000-01-01"), "overdue"),
        ("accept without approver",
         mutate(0, decision="accept", approver="-"), "accept needs an approver"),
        ("accept with approver",
         mutate(0, decision="accept", approver="reviewer"), None),
        ("register row moved to A", base + [stale], "moved to A"),
        ("evidence outside the repo",
         mutate(0, evidence="/tmp/evidence.log"), "repo-relative file"),
        ("invalid due date", mutate(0, due="2026-13-01"), "YYYY-MM-DD"),
        ("unknown decision", mutate(0, decision="maybe"), "decision"),
    ]
    errors: list[str] = []
    for name, records, expected in fixtures:
        found = register_errors(records, matrix)
        if expected is None and found:
            errors.append(f"self-check: {name} was rejected: {found}")
        elif expected is not None and not any(
                expected in message for message in found):
            errors.append(f"self-check: {name} was not rejected")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--self-check", action="store_true",
        help="run the register guard against in-memory fixtures")
    args = parser.parse_args()

    matrix = matrix_rows()
    records = parse_register()
    risk_count = sum(
        1 for row in matrix
        if row["status"] == "X" and row["priority"] in RISK_PRIORITIES)
    errors = register_errors(records, matrix)
    self_errors: list[str] = []
    if args.self_check:
        self_errors = self_check()
        errors.extend(self_errors)

    print(f"risk rows={len(records)} matrix P0/P1 X={risk_count}")
    if args.self_check and not self_errors:
        print("risk register self-check ok (clean, missing, duplicate, "
              "overdue, accept, stale A, evidence, date and decision "
              "fixtures rejected or accepted as intended)")
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
