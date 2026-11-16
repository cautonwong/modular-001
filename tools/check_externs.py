#!/usr/bin/env python3
"""Ratchet the ad-hoc extern declarations in owned sources.

The scan covers targets/, application/, components/ and infra/kernel.
Vendor trees (infra/soc, infra/boards, infra/third_party) stay out.

A declaration belongs in the header of the module that defines the
symbol. The baseline records the current debt. The check fails when a file
grows past its baseline or when a baseline entry no longer exists.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "extern_baseline.txt"
EXTERN = re.compile(r"^\s*extern\s")


def counts() -> dict[str, int]:
    found: dict[str, int] = {}
    for base in (ROOT / "targets", ROOT / "application",
                 ROOT / "components", ROOT / "infra" / "kernel"):
        for path in sorted(base.rglob("*.c")):
            rel = path.relative_to(ROOT)
            if "build" in rel.parts:
                continue
            number = sum(1 for line in path.read_text(errors="replace").splitlines()
                         if EXTERN.match(line))
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
        lines = ["# target/file count", *[f"{path} {n}" for path, n in current.items()]]
        BASELINE.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"extern baseline updated: {sum(current.values())} declarations")
        return 0

    baseline = load()
    failures = 0
    for path, number in sorted(current.items()):
        allowed = baseline.get(path, 0)
        if number > allowed:
            print(f"{path}: {number} externs, baseline {allowed}")
            failures += 1
    stale = [path for path in baseline if path not in current]
    for path in stale:
        print(f"{path}: baseline entry is stale")
        failures += 1
    print(f"externs ok ({sum(current.values())} declarations, "
          f"{len(current)} files)")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
