#!/usr/bin/env python3
"""Ratchet conversion warnings in the production host libraries.

The build uses -Wconversion, -Wsign-conversion, -Wcast-qual,
-Wdouble-promotion and -Wfloat-conversion without -Werror. Warnings are
grouped by file and warning flag, not by line, so unrelated edits do not
churn the baseline.

Run with --update after fixing or accepting a change.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "conversion_baseline.txt"
WARNING = re.compile(
    r"^(?P<path>[^:\n]+):\d+:\d+: warning: .*?\[(?P<flag>-W[^\]]+)\]$")


def normalize(path: str) -> str:
    path = path.strip()
    prefix = str(ROOT) + "/"
    if path.startswith(prefix):
        path = path[len(prefix):]
    return path


def counts(log: Path) -> dict[tuple[str, str], int]:
    found: dict[tuple[str, str], int] = {}
    for line in log.read_text(errors="replace").splitlines():
        match = WARNING.match(line)
        if not match:
            continue
        key = (normalize(match.group("path")), match.group("flag"))
        found[key] = found.get(key, 0) + 1
    return found


def load() -> dict[tuple[str, str], int]:
    baseline: dict[tuple[str, str], int] = {}
    for line in BASELINE.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        path, flag, number = line.rsplit(None, 2)
        baseline[(path, flag)] = int(number)
    return baseline


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", type=Path, help="captured build output")
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()

    if args.update:
        if not args.log:
            parser.error("--update needs --log")
        current = counts(args.log)
        lines = ["# path flag count"]
        lines.extend(f"{path} {flag} {number}"
                     for (path, flag), number in sorted(current.items()))
        BASELINE.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"conversion baseline updated: {sum(current.values())} warnings")
        return 0

    if not args.log:
        parser.error("--log is required")
    current = counts(args.log)
    baseline = load()
    failures = 0
    for key, number in sorted(current.items()):
        allowed = baseline.get(key, 0)
        if number > allowed:
            print(f"{key[0]} {key[1]}: {number} warnings, baseline {allowed}")
            failures += 1
    for path, flag in sorted(set(baseline) - set(current)):
        print(f"{path} {flag}: baseline entry is stale")
        failures += 1
    print(f"conversion warnings ok ({sum(current.values())} warnings, "
          f"{len(current)} groups)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
