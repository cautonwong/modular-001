#!/usr/bin/env python3
"""Design and plan records must cite repo paths or gates, not ephemeral or
out-of-repo evidence.

A /tmp path is gone after a reboot or a cleanup, and a /home, /root,
/var/tmp, /mnt, home-relative, parent-relative or Windows path exists only on
one machine, so a reader cannot reproduce the claim. Untracked design and plan
records are scanned too. Command examples that need a scratch directory belong
in the guides, not in a design or plan record.
"""
from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCAN_ROOTS = ("docs/design", "docs/plans")


def tracked_markdown(root: Path) -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard",
         "--", *SCAN_ROOTS],
        cwd=root, check=True, capture_output=True, text=True,
    )
    names = [name for name in result.stdout.split("\0")
             if name.endswith(".md")]
    return [root / name for name in names]


EPHEMERAL = re.compile(
    r"(?:(?<![\w.-])/(?:tmp|home|root|var/tmp|mnt|dev/shm)(?:/|\b)"
    r"|(?<![\w.-])~/"
    r"|\$(?:HOME|\{HOME\})/"
    r"|(?<![\w.])\.\./(?!\.)"
    r"|[A-Za-z]:\\)")


def ephemeral_reference(line: str) -> bool:
    return EPHEMERAL.search(line) is not None


def failures(root: Path) -> list[str]:
    errors: list[str] = []
    for path in tracked_markdown(root):
        for number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(),
                start=1):
            if ephemeral_reference(line):
                errors.append(
                    f"{path.relative_to(root)}:{number}: design and plan "
                    "records must cite a repo path or a gate, not temporary "
                    "or out-of-repo evidence")
    return errors


def self_check() -> list[str]:
    errors: list[str] = []
    for text, expected in (("see /tmp/arena/design.md", True),
                           ("see /home/edge/evidence/design.md", True),
                           ("see /root/evidence/design.md", True),
                           ("see /var/tmp/evidence/design.md", True),
                           ("see /mnt/c/Users/edge/run.log", True),
                           ("see /dev/shm/run.log", True),
                           ("see ~/run.log", True),
                           ("see $HOME/run.log", True),
                           ("see ${HOME}/run.log", True),
                           ("see ../../outside/run.log", True),
                           ("see C:\\Users\\edge\\run.log", True),
                           ("see /homepage/design.md", False),
                           ("see /tmpfoo", False),
                           ("see docs/design/record-retrieval.md", False)):
        if ephemeral_reference(text) != expected:
            errors.append(f"self-check failed for {text!r}")
    with tempfile.TemporaryDirectory(prefix="doc-evidence-") as tmp:
        root = Path(tmp)
        (root / "docs/design").mkdir(parents=True)
        (root / "docs/plans").mkdir(parents=True)
        (root / "docs/design/clean.md").write_text(
            "see docs/design/record-retrieval.md\n", encoding="utf-8")
        (root / "docs/plans/clean.md").write_text(
            "see tools/ci_matrix.py\n", encoding="utf-8")
        subprocess.run(["git", "init", "-q"], cwd=root, check=True)
        subprocess.run(["git", "add", "-A"], cwd=root, check=True)
        if failures(root):
            errors.append("self-check: a clean tree reported failures")
        (root / "docs/design/dirty.md").write_text(
            "see /tmp/design-log\n", encoding="utf-8")
        subprocess.run(["git", "add", "-A"], cwd=root, check=True)
        reported = failures(root)
        if len(reported) != 1 or "docs/design/dirty.md" not in reported[0]:
            errors.append("self-check: the docs/design negative was not caught")
        (root / "docs/plans/dirty.md").write_text(
            "see /tmp/plan-log\n", encoding="utf-8")
        subprocess.run(["git", "add", "-A"], cwd=root, check=True)
        reported = failures(root)
        if len(reported) != 2 or not any(
                "docs/plans/dirty.md" in error for error in reported):
            errors.append("self-check: the docs/plans negative was not caught")
        (root / "docs/design/untracked.md").write_text(
            "see /tmp/untracked-log\n", encoding="utf-8")
        reported = failures(root)
        if len(reported) != 3 or not any(
                "docs/design/untracked.md" in error for error in reported):
            errors.append("self-check: the untracked record was not caught")
    return errors


def main() -> int:
    errors = self_check()
    if errors:
        print("\n".join(errors))
        return 1
    if "--self-check" in sys.argv:
        print("design evidence self-check ok")
        return 0
    errors = failures(ROOT)
    if errors:
        print("\n".join(errors))
        print("design evidence: FAILED")
        return 1
    print("design evidence ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
