#!/usr/bin/env python3
"""Ratchet `#if 0` dead blocks in owned sources.

The scan covers .c and .h files and counts every `#if 0` directive line at
any nesting depth: a dead block nested inside a normal #if still ships in
the source, so it is debt. A nested `#if 0` inside another `#if 0` counts
as its own line; there is no deduplication because deleting the outer
block removes both lines and --update lowers the count accordingly.

The baseline records the current debt per file. A new block or a higher
count fails; delete the block and run --update to lower the baseline.
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "dead_code_baseline.txt"
START = re.compile(r"^\s*#\s*if\s+0\b")


def blocks(path: Path) -> int:
    return sum(1 for line in path.read_text(errors="replace").splitlines()
               if START.match(line))


def counts(root: Path = ROOT) -> dict[str, int]:
    found: dict[str, int] = {}
    for base in ("domain", "application", "components", "infra", "targets"):
        if not (root / base).is_dir():
            continue
        for path in sorted((root / base).rglob("*")):
            if path.suffix not in (".c", ".h"):
                continue
            rel = path.relative_to(root)
            if "third_party" in rel.parts:
                continue
            number = blocks(path)
            if number:
                found[rel.as_posix()] = number
    return found


def load(path: Path = BASELINE) -> dict[str, int]:
    baseline: dict[str, int] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        path, number = line.rsplit(None, 1)
        baseline[path] = int(number)
    return baseline


def render(current: dict[str, int]) -> str:
    return "\n".join(["# file blocks", *[f"{path} {n}" for path, n in current.items()]]) + "\n"


def diff(baseline: dict[str, int], current: dict[str, int]) -> list[str]:
    failures = []
    for path, number in sorted(current.items()):
        allowed = baseline.get(path, 0)
        if number > allowed:
            failures.append(f"{path}: {number} #if 0 blocks, baseline {allowed}")
        elif number < allowed:
            failures.append(f"{path}: baseline says {allowed}, found {number}; "
                            "run --update")
    for path in sorted(set(baseline) - set(current)):
        failures.append(f"{path}: baseline entry is stale; run --update")
    return failures


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root / "domain").mkdir()
        header = root / "domain" / "dead.h"
        header.write_text("#if 1\n#if 0\nint nested_in_header;\n#endif\n#endif\n"
                          "#if 0\nint top_in_header;\n#endif\n"
                          "#if defined(X)\nint live;\n#endif\n", encoding="utf-8")
        source = root / "domain" / "dead.c"
        source.write_text("#if defined(X)\n#if 0\nint nested_in_source;\n#endif\n"
                          "#endif\n", encoding="utf-8")
        (root / "infra" / "third_party").mkdir(parents=True)
        (root / "infra" / "third_party" / "vendor.h").write_text(
            "#if 0\nint vendor;\n#endif\n", encoding="utf-8")
        got = counts(root)
        expected = {"domain/dead.h": 2, "domain/dead.c": 1}
        if got != expected:
            print(f"dead code self-check failed: scan counted {got}, "
                  f"expected {expected}")
            return False
        baseline = root / "baseline.txt"
        baseline.write_text(render(got), encoding="utf-8")
        if diff(load(baseline), got):
            print("dead code self-check failed: clean baseline rejected")
            return False
        header.write_text(header.read_text(encoding="utf-8")
                          + "#if 0\nint new_block;\n#endif\n", encoding="utf-8")
        if not any("dead.h" in message for message in diff(load(baseline), counts(root))):
            print("dead code self-check failed: new block not rejected")
            return False
        source.write_text("#if defined(X)\nint live;\n#endif\n", encoding="utf-8")
        if not any("dead.c" in message for message in diff(load(baseline), counts(root))):
            print("dead code self-check failed: removed block not rejected")
            return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if not tree_check():
        return 1

    current = counts()
    if args.update:
        BASELINE.write_text(render(current), encoding="utf-8")
        print(f"dead-code baseline updated: {sum(current.values())} blocks")
        return 0

    errors = diff(load(), current)
    if args.self_check and not errors:
        print("dead code self-check ok (nested, header, new and removed blocks "
              "classified)")
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    if errors:
        return 1
    print(f"dead code ok ({sum(current.values())} blocks, {len(current)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
