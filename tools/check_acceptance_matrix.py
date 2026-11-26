#!/usr/bin/env python3
"""Check the firmware acceptance matrix against registered acceptance cases."""
from __future__ import annotations

import argparse
import collections
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "docs" / "firmware-acceptance-matrix.md"
ACCEPTANCE = ROOT / "targets" / "stategrid_meter" / "tests" / "acceptance"
EXEMPTIONS = ROOT / "tools" / "acceptance_matrix_exemptions.txt"

ROW_RE = re.compile(r"^\|\s*(ACC-[A-Z0-9-]+)\s*\|(.*)\|\s*$")
DOMAIN_RE = re.compile(r"^### 6\.\d+\s+(.+?)\s*$")
CASE_RE = re.compile(r"@case\(\s*[\"'](ACC-[A-Z0-9-]+)[\"']")
STATUS_RE = re.compile(
    r"^状态：v2\.0，(\d+) 行业务能力；(\d+) 行已覆盖，(\d+) 行排除，"
    r"(\d+) 行待补。\s*$",
    re.MULTILINE,
)
TOTAL_RE = re.compile(r"^\|\s*合计\s*\|(.*)\|\s*$")
SIGN_RE = re.compile(r"(全量“所有业务”签收要求 )(\d+)( 行全部到)")
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


def cases(directory: Path = ACCEPTANCE) -> set[str]:
    found: set[str] = set()
    for path in sorted(directory.glob("test_*.py")):
        text = path.read_text(encoding="utf-8")
        code = "\n".join(
            line for line in text.splitlines()
            if not line.lstrip().startswith("#"))
        for name in CASE_RE.findall(code):
            if name in found:
                raise SystemExit(f"duplicate acceptance case {name}")
            found.add(name)
    return found


def exemptions(path: Path = EXEMPTIONS) -> dict[str, str]:
    found: dict[str, str] = {}
    if not path.is_file():
        return found
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        parts = line.split(None, 1)
        if len(parts) != 2 or not parts[1].strip():
            raise SystemExit(f"exemption line {number} has no reason")
        if parts[0] in found:
            raise SystemExit(f"duplicate exemption {parts[0]}")
        found[parts[0]] = parts[1].strip()
    return found


def reverse_errors(
    matrix: list[dict[str, str]],
    registered: set[str],
    exempt: dict[str, str],
) -> list[str]:
    by_id = {row["id"]: row for row in matrix}
    errors: list[str] = []
    for case in sorted(registered):
        if case in by_id:
            continue
        if case.endswith("-BLOCKED"):
            base = case[: -len("-BLOCKED")]
            row = by_id.get(base)
            if row is None:
                errors.append(
                    f"{case} is a blocked probe but {base} has no matrix row")
            elif row["status"] != "X":
                errors.append(
                    f"{case} pins {base} but that row is {row['status']}, not X")
            continue
        if case not in exempt:
            errors.append(
                f"{case} is registered but has no matrix row and no exemption")
    for case in sorted(exempt):
        if case not in registered:
            errors.append(f"exemption {case} is stale: no registered case")
            continue
        mapped = case in by_id
        if case.endswith("-BLOCKED"):
            base = by_id.get(case[: -len("-BLOCKED")])
            mapped = mapped or (base is not None and base["status"] == "X")
        if mapped:
            errors.append(f"exemption {case} is stale: the case is mapped now")
    return errors


def summary_counts(matrix: list[dict[str, str]]) -> dict[str, int]:
    total = collections.Counter(row["status"] for row in matrix)
    return {
        "rows": len(matrix),
        "A": total["A"],
        "B": total["B"],
        "C": total["C"],
        "D": total["D"],
        "E": total["E"],
        "X": total["X"],
        "pending": len(matrix) - total["A"] - total["X"],
    }


def summary_errors(text: str, matrix: list[dict[str, str]]) -> list[str]:
    counts = summary_counts(matrix)
    lines = text.splitlines()
    errors: list[str] = []

    status = [line for line in lines if line.startswith("状态：")]
    if len(status) != 1:
        errors.append(
            f"line 4 status summary: found {len(status)} status lines, expected 1"
        )
    else:
        match = STATUS_RE.match(status[0])
        if match is None:
            errors.append("line 4 status summary: line does not match the expected format")
        else:
            got = tuple(int(value) for value in match.groups())
            want = (counts["rows"], counts["A"], counts["X"], counts["pending"])
            if got != want:
                errors.append(
                    f"line 4 status summary: rows/A/X/pending={got}, expected {want}"
                )

    total_lines = [line for line in lines if line.startswith("| 合计 |")]
    if len(total_lines) != 1:
        errors.append(
            f"section 5 total row: found {len(total_lines)} total rows, expected 1"
        )
    else:
        match = TOTAL_RE.match(total_lines[0])
        if match is None:
            errors.append("section 5 total row: row does not match the expected format")
        else:
            cells = [cell.strip() for cell in match.group(1).split("|")]
            if len(cells) != 8 or cells[7] != "":
                errors.append(
                    f"section 5 total row: has {len(cells)} cells, expected 8"
                )
            else:
                try:
                    got = tuple(int(cell) for cell in cells[:7])
                except ValueError:
                    errors.append("section 5 total row: counts are not integers")
                else:
                    want = (
                        counts["rows"], counts["A"], counts["B"],
                        counts["C"], counts["D"], counts["E"], counts["X"],
                    )
                    if got != want:
                        errors.append(
                            f"section 5 total row: rows/A/B/C/D/E/X={got}, "
                            f"expected {want}"
                        )

    signs = SIGN_RE.findall(text)
    if len(signs) != 1:
        errors.append(
            f"section 8 sign-off: found {len(signs)} sign-off counts, expected 1"
        )
    elif int(signs[0][1]) != counts["rows"]:
        errors.append(
            f"section 8 sign-off: rows={signs[0][1]}, expected {counts['rows']}"
        )

    return errors


def update_counts(matrix: list[dict[str, str]]) -> None:
    counts = summary_counts(matrix)
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
    for domain, domain_counts in by_domain.items():
        rows.append(
            f"| {domain} | {sum(domain_counts.values())} | {domain_counts['A']} | "
            f"{domain_counts['B']} | {domain_counts['C']} | {domain_counts['D']} | "
            f"{domain_counts['E']} | {domain_counts['X']} | {priorities.get(domain, 'P0')} |"
        )
    rows.append(
        f"| 合计 | {counts['rows']} | {counts['A']} | {counts['B']} | "
        f"{counts['C']} | {counts['D']} | {counts['E']} | {counts['X']} | |"
    )
    lines[start:end + 1] = rows
    for index, line in enumerate(lines):
        if line.startswith("状态："):
            lines[index] = (
                f"状态：v2.0，{counts['rows']} 行业务能力；{counts['A']} 行已覆盖，"
                f"{counts['X']} 行排除，{counts['pending']} 行待补。"
            )
        if "全量“所有业务”签收要求" in line:
            lines[index] = SIGN_RE.sub(rf"\g<1>{counts['rows']}\g<3>", line)
    MATRIX.write_text("\n".join(lines) + "\n", encoding="utf-8")


def self_check_errors() -> list[str]:
    text = MATRIX.read_text(encoding="utf-8")
    matrix = rows()
    registered_cases = cases()
    counts = summary_counts(matrix)
    errors: list[str] = []

    for message in summary_errors(text, matrix):
        errors.append(f"self-check: clean matrix rejected: {message}")

    status_wrong = (
        f"状态：v2.0，{counts['rows'] + 1} 行业务能力；{counts['A']} 行已覆盖，"
        f"{counts['X']} 行排除，{counts['pending']} 行待补。"
    )
    total_wrong = (
        f"| 合计 | {counts['rows']} | {counts['A'] + 1} | {counts['B']} | "
        f"{counts['C']} | {counts['D']} | {counts['E']} | {counts['X']} | |"
    )
    total_current = (
        f"| 合计 | {counts['rows']} | {counts['A']} | {counts['B']} | "
        f"{counts['C']} | {counts['D']} | {counts['E']} | {counts['X']} | |"
    )
    variants = [
        ("status", STATUS_RE.sub(status_wrong, text, count=1),
         "line 4 status summary"),
        ("total", text.replace(total_current, total_wrong, 1),
         "section 5 total row"),
        ("sign-off", SIGN_RE.sub(rf"\g<1>{counts['rows'] + 1}\g<3>", text, count=1),
         "section 8 sign-off"),
    ]

    with tempfile.TemporaryDirectory() as directory:
        for name, variant, location in variants:
            if variant == text:
                errors.append(f"self-check: {name} mutation did not change the text")
                continue
            copy = Path(directory) / f"matrix-{name}.md"
            copy.write_text(variant, encoding="utf-8")
            found = summary_errors(copy.read_text(encoding="utf-8"), matrix)
            if not any(location in message for message in found):
                errors.append(
                    f"self-check: {name} mutation was not rejected at {location}"
                )

    with tempfile.TemporaryDirectory() as directory:
        acceptance = Path(directory) / "acceptance"
        acceptance.mkdir()
        (acceptance / "test_case.py").write_text(
            '@case(\n    "ACC-REAL",\n    "clause",\n)\n'
            '# @case("ACC-COMMENTED")\n', encoding="utf-8")
        got = cases(acceptance)
        if got != {"ACC-REAL"}:
            errors.append(
                f"self-check: commented case counted, got {sorted(got)}")

    mapped = next(row["id"] for row in matrix if row["status"] == "A")
    excluded = next(row["id"] for row in matrix if row["status"] == "X")
    fixtures: list[tuple[str, str, dict[str, str], str | None]] = [
        ("mapped case", f'@case("{mapped}")\n', {}, None),
        ("unmapped case", '@case("ACC-UNMAPPED")\n', {}, "ACC-UNMAPPED"),
        ("exempt case", '@case("ACC-UNMAPPED")\n',
         {"ACC-UNMAPPED": "fixture reason"}, None),
        ("blocked probe of an X row", f'@case("{excluded}-BLOCKED")\n', {}, None),
        ("blocked probe of an A row", f'@case("{mapped}-BLOCKED")\n', {},
         f"{mapped}-BLOCKED"),
        ("orphan blocked probe", '@case("ACC-ORPHAN-BLOCKED")\n', {},
         "ACC-ORPHAN-BLOCKED"),
    ]
    for name, text, exempt, expected in fixtures:
        with tempfile.TemporaryDirectory() as directory:
            acceptance = Path(directory) / "acceptance"
            acceptance.mkdir()
            (acceptance / "test_fixture.py").write_text(text, encoding="utf-8")
            found = reverse_errors(matrix, cases(acceptance), exempt)
        if expected is None and found:
            errors.append(f"self-check: {name} was rejected: {found}")
        elif expected is not None and not any(expected in message for message in found):
            errors.append(f"self-check: {name} was not rejected")

    stale = reverse_errors(matrix, registered_cases, {"ACC-STALE": "fixture reason"})
    if not any("ACC-STALE" in message and "stale" in message for message in stale):
        errors.append("self-check: stale exemption was not rejected")
    remapped = reverse_errors(matrix, registered_cases, {mapped: "fixture reason"})
    if not any(mapped in message and "stale" in message for message in remapped):
        errors.append("self-check: remapped exemption was not rejected")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--strict", action="store_true", help="require every row to be A or X")
    parser.add_argument("--quiet", action="store_true", help="only print failures")
    parser.add_argument(
        "--update",
        action="store_true",
        help="rewrite the status line, section 5 count table and section 8 sign-off",
    )
    parser.add_argument(
        "--self-check",
        action="store_true",
        help="run the summary count guard against mutated temporary copies",
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
    errors.extend(reverse_errors(matrix, registered, exemptions()))

    if args.update:
        update_counts(matrix)

    errors.extend(summary_errors(MATRIX.read_text(encoding="utf-8"), matrix))
    self_errors: list[str] = []
    if args.self_check:
        self_errors = self_check_errors()
        errors.extend(self_errors)

    if not args.quiet:
        print(f"rows={len(matrix)} cases={len(registered)}")
        print(" ".join(f"{key}={by_status[key]}" for key in sorted(by_status)))
        for domain, domain_counts in by_domain.items():
            print(f"{domain}: " + " ".join(f"{key}={domain_counts[key]}" for key in sorted(domain_counts)))
        if args.self_check and not self_errors:
            print("matrix guard self-check ok (3 summary mutations, 6 reverse "
                  "fixtures and 2 exemption fixtures rejected or accepted as intended)")

    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
