#!/usr/bin/env python3
"""Design records must cite repo paths or gates, not ephemeral /tmp evidence.

docs/design records decisions and the evidence behind them. A /tmp path is
gone after a reboot or a cleanup, so a reader cannot reproduce the claim. The
checker scans tracked markdown under docs/design and fails on any `/tmp/`
reference. Command examples that need a scratch directory belong in the
guides, not in a design record.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCAN = "docs/design"


def tracked_markdown() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z", "--", f"{SCAN}/*.md", f"{SCAN}/**/*.md"],
        cwd=ROOT, check=True, capture_output=True, text=True,
    )
    names = [name for name in result.stdout.split("\0") if name]
    return [ROOT / name for name in names]


def ephemeral_reference(line: str) -> bool:
    return "/tmp/" in line


def failures() -> list[str]:
    errors: list[str] = []
    for path in tracked_markdown():
        for number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(),
                start=1):
            if ephemeral_reference(line):
                errors.append(
                    f"{path.relative_to(ROOT)}:{number}: design records must "
                    "cite a repo path or a gate, not /tmp evidence")
    return errors


def self_check() -> list[str]:
    errors: list[str] = []
    for text, expected in (("see /tmp/arena/design.md", True),
                           ("see docs/design/record-retrieval.md", False)):
        if ephemeral_reference(text) != expected:
            errors.append(f"self-check failed for {text!r}")
    return errors


def main() -> int:
    if "--self-check" in sys.argv:
        errors = self_check()
        if errors:
            print("\n".join(errors))
            return 1
        print("design evidence self-check ok")
        return 0
    errors = failures()
    if errors:
        print("\n".join(errors))
        print("design evidence: FAILED")
        return 1
    print("design evidence ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
