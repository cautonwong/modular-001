#!/usr/bin/env python3
"""Check the firmware acceptance matrix against registered acceptance cases."""
from __future__ import annotations

import argparse
import collections
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "docs" / "firmware-acceptance-matrix.md"
ACCEPTANCE = ROOT / "targets" / "stategrid_meter" / "tests" / "acceptance"

ROW_RE = re.compile(r"^\|\s*(ACC-[A-Z0-9-]+)\s*\|(.*)\|\s*$")
DOMAIN_RE = re.compile(r"^### 6\.\d+\s+(.+?)\s*$")
CASE_RE = re.compile(r"@case\(\s*[\"'](ACC-[A-Z0-9-]+)[\"']")
VALID = {"A", "B", "C", "D", "E", "X"}


def rows() -> list[dict[str, str]]:
    domain = "?"
    found: list[dict[str, str]] = []
    seen: set[str] = set()
    for line in MATRIX.read_text(encoding="utf-8").splitlines():
        match = DOMAIN_RE.match(line)
        if match:
            domain = match.group(1)
            continue
        match = ROW_RE.match(line)
        if not match:
            continue
        cells = [cell.strip() for cell in match.group(2).split("|")]
        if len(cells) < 6:
            raise SystemExit(f"matrix row {match.group(1)} has {len(cells)} cells")
        row = {
            "id": match.group(1),
            "domain": domain,
            "capability": cells[0],
            "trigger": cells[1],
            "criterion": cells[2],
            "status": cells[3],
            "priority": cells[4],
            "blocker": cells[5],
        }
        if row["id"] in seen:
            raise SystemExit(f"duplicate matrix row {row['id']}")
        if row["status"] not in VALID:
            raise SystemExit(f"matrix row {row['id']} has unknown status {row['status']!r}")
        seen.add(row["id"])
        found.append(row)
    return found


def cases() -> set[str]:
    found: set[str] = set()
    for path in sorted(ACCEPTANCE.glob("test_*.py")):
        for name in CASE_RE.findall(path.read_text(encoding="utf-8")):
            if name in found:
                raise SystemExit(f"duplicate acceptance case {name}")
            found.add(name)
    return found


def update_counts(matrix: list[dict[str, str]]) -> None:
    lines = MATRIX.read_text(encoding="utf-8").splitlines()
    header = next(i for i, line in enumerate(lines) if line.startswith("| 域 |"))
    start = header + 2
    end = next(i for i, line in enumerate(lines) if line.startswith("| 合计 |"))
    priorities = {}
    for line in lines[start:end]:
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        if len(cells) == 9:
            priorities[cells[0]] = cells[8]
    by_domain: dict[str, collections.Counter[str]] = collections.defaultdict(collections.Counter)
    for row in matrix:
        by_domain[row["domain"]][row["status"]] += 1
    rows = []
    for domain, counts in by_domain.items():
        rows.append(
            f"| {domain} | {sum(counts.values())} | {counts['A']} | "
            f"{counts['B']} | {counts['C']} | {counts['D']} | "
            f"{counts['E']} | {counts['X']} | {priorities.get(domain, 'P0')} |"
        )
    total = collections.Counter(row["status"] for row in matrix)
    rows.append(
        f"| 合计 | {len(matrix)} | {total['A']} | {total['B']} | "
        f"{total['C']} | {total['D']} | {total['E']} | {total['X']} | |"
    )
    lines[start:end + 1] = rows
    MATRIX.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--strict", action="store_true", help="require every row to be A or X")
    parser.add_argument("--quiet", action="store_true", help="only print failures")
    parser.add_argument(
        "--update",
        action="store_true",
        help="rewrite the section 5 count table from row statuses",
    )
    args = parser.parse_args()

    matrix = rows()
    registered = cases()
    by_status = collections.Counter(row["status"] for row in matrix)
    by_domain: dict[str, collections.Counter[str]] = collections.defaultdict(collections.Counter)
    for row in matrix:
        by_domain[row["domain"]][row["status"]] += 1

    errors: list[str] = []
    for row in matrix:
        if row["status"] == "A" and row["id"] not in registered:
            errors.append(f"{row['id']} is A but has no @case")
        if row["status"] == "X" and (not row["blocker"] or row["blocker"] in {"无", "-"}):
            errors.append(f"{row['id']} is X but has no exclusion reason")
        if args.strict and row["status"] not in {"A", "X"}:
            errors.append(f"{row['id']} is still {row['status']}")

    if args.update:
        update_counts(matrix)

    if not args.quiet:
        print(f"rows={len(matrix)} cases={len(registered)}")
        print(" ".join(f"{key}={by_status[key]}" for key in sorted(by_status)))
        for domain, counts in by_domain.items():
            print(f"{domain}: " + " ".join(f"{key}={counts[key]}" for key in sorted(counts)))

    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
