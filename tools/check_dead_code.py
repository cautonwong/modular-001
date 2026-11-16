#!/usr/bin/env python3
"""Ratchet `#if 0` dead blocks in owned sources.

The baseline records the current debt per file. A new block or a higher
count fails; delete the block and run --update to lower the baseline.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "dead_code_baseline.txt"
START = re.compile(r"^\s*#\s*if\s+0\b")
IF = re.compile(r"^\s*#\s*if")
ENDIF = re.compile(r"^\s*#\s*endif")


def blocks(path: Path) -> int:
    depth = 0
    count = 0
    for line in path.read_text(errors="replace").splitlines():
        if START.match(line) and depth == 0:
            count += 1
        if IF.match(line):
            depth += 1
        elif ENDIF.match(line) and depth > 0:
            depth -= 1
    return count


def counts() -> dict[str, int]:
    found: dict[str, int] = {}
    for base in ("domain", "application", "components", "infra", "targets"):
        for path in sorted((ROOT / base).rglob("*.c")):
            rel = path.relative_to(ROOT)
            if "third_party" in rel.parts:
                continue
            number = blocks(path)
            if number:
                found[rel.as_posix()] = number
    return found


def load() -> dict[str, int]:
    baseline: dict[str, int] = {}
    for line in BASELINE.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        path, number = line.rsplit(None, 1)
        baseline[path] = int(number)
    return baseline


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()

    current = counts()
    if args.update:
        lines = ["# file blocks", *[f"{path} {n}" for path, n in current.items()]]
        BASELINE.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"dead-code baseline updated: {sum(current.values())} blocks")
        return 0

    baseline = load()
    failures = 0
    for path, number in sorted(current.items()):
        allowed = baseline.get(path, 0)
        if number > allowed:
            print(f"{path}: {number} #if 0 blocks, baseline {allowed}")
            failures += 1
    for path in sorted(set(baseline) - set(current)):
        print(f"{path}: baseline entry is stale")
        failures += 1
    print(f"dead code ok ({sum(current.values())} blocks, {len(current)} files)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
