#!/usr/bin/env python3
"""Ratchet the legacy kernel types left in the product targets.

The terminal state has no instance, no pipeline and no nidavellir. Every
target source that still names them is recorded here; the count may only
go down. A file that grows fails, and so does a baseline entry that no
longer exists.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "target_kernel_baseline.txt"
PATTERN = re.compile(
    r'nidavellir\.h|"kernel/|legacy_control_block|'
    r'struct\s+pipeline_t|struct\s+instance_t|\bpid_[a-z_]+')


def counts() -> dict[str, int]:
    found: dict[str, int] = {}
    for path in sorted((ROOT / "targets").rglob("*")):
        if path.suffix not in (".c", ".h") or "build" in path.parts:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        number = len(PATTERN.findall(text))
        if number:
            found[path.relative_to(ROOT).as_posix()] = number
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
        lines = ["# target/file references", *[f"{path} {n}" for path, n in current.items()]]
        BASELINE.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"target kernel baseline updated: {sum(current.values())} "
              f"references in {len(current)} files")
        return 0

    baseline = load()
    failures = 0
    for path, number in sorted(current.items()):
        allowed = baseline.get(path, 0)
        if number > allowed:
            print(f"{path}: {number} references, baseline {allowed}")
            failures += 1
    for path in baseline:
        if path not in current:
            print(f"{path}: baseline entry is stale")
            failures += 1
    print(f"target kernel ok ({sum(current.values())} references in "
          f"{len(current)} files)")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
