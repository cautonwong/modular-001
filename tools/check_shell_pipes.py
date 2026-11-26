#!/usr/bin/env python3
"""Reject pipefail scripts whose pipeline can SIGPIPE the writer.

Under `set -o pipefail`, `writer | grep -q` fails with 141 when grep exits
after the first match and the writer is still writing. The same applies to
`head`, `grep -m`, `--max-count`, `sed ... q` and `awk ... exit`, in any
pipeline stage, including a pipeline continued with a trailing backslash.

Use a here-string (`grep pattern <<< "$output"`) or let grep read all output
(`grep pattern <<< "$output" >/dev/null`). An intentional pipeline carries an
on-line exception with a reason, an expiry date and an approver:

    # pipefail-ok: reason; expires=2027-01-31; approver=name
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PIPEFAIL = re.compile(
    r"set\s+(?:-[A-Za-z]+\s+)*-o\s+pipefail|set\s+-[A-Za-z]*o\s+pipefail")
PIPE = re.compile(r"(?<!\|)\|(?!\|)")
GREP = re.compile(r"\bgrep\b")
GREP_QUIET = re.compile(r"-[A-Za-z0-9]*q[A-Za-z0-9]*\b|--quiet\b")
GREP_MAX = re.compile(r"\s-m\s*\d|\s-m\b|--max-count")
HEAD = re.compile(r"\bhead\b")
SED_QUIT = re.compile(r"\bsed\b[^|]*q")
AWK_EXIT = re.compile(r"\bawk\b[^|]*\bexit\b")
EXCEPTION = re.compile(
    r"pipefail-ok:\s*(?P<reason>[^;]+);\s*expires=(?P<expires>\d{4}-\d{2}-\d{2});"
    r"\s*approver=(?P<approver>\S+)")

SELF_CHECK = (
    ("set -euo pipefail\nprintf '%s\\n' \"$out\" | grep -Eq '^x$'\n", 1),
    ("set -euo pipefail\ngrep -E '^x$' <<< \"$out\" >/dev/null\n", 0),
    ("printf '%s\\n' \"$out\" | grep -q x\n", 0),
    ("set -o pipefail\nvalue=$(printf x | head -1)\n", 1),
    ("set -o pipefail\nprintf x | grep -m1 y\n", 1),
    ("set -o pipefail\nprintf x | grep -qm1 y\n", 1),
    ("set -o pipefail\nprintf x | grep -q x | cat\n", 1),
    ("set -o pipefail\nprintf x | sed -n '1q'\n", 1),
    ("set -o pipefail\nprintf x | cat\n", 0),
    ("set -o pipefail\nfoo || grep -q x\n", 0),
    ("set -o pipefail\n# printf x | grep -q x\n", 0),
    ("set -o pipefail\nprintf x | \\\n    grep -q x\n", 1),
    ("set -e -o pipefail\nprintf x | grep -q y\n", 1),
    ("set -o pipefail\nprintf x | grep -q x # pipefail-ok: no expiry\n", 1),
    ("set -o pipefail\nprintf x | grep -q x # pipefail-ok: test; "
     "expires=2099-01-01; approver=reviewer\n", 0),
)


def logical_lines(lines: list[str]) -> list[tuple[int, str]]:
    joined: list[tuple[int, str]] = []
    buffer = ""
    start = 0
    for number, line in enumerate(lines, 1):
        if not buffer:
            start = number
        stripped = line.rstrip()
        if stripped.endswith("\\"):
            buffer += stripped[:-1] + " "
            continue
        joined.append((start, buffer + line))
        buffer = ""
    if buffer:
        joined.append((start, buffer))
    return joined


def early_reader(segment: str) -> bool:
    if GREP.search(segment):
        return bool(GREP_QUIET.search(segment) or GREP_MAX.search(segment))
    return bool(HEAD.search(segment) or SED_QUIT.search(segment)
                or AWK_EXIT.search(segment))


def pipeline_hits(line: str) -> bool:
    parts = PIPE.split(line)
    return len(parts) > 1 and any(early_reader(part) for part in parts[1:])


def exceptions(line: str) -> list[str]:
    match = EXCEPTION.search(line)
    if match is None:
        return ["malformed exception; expected reason; expires=YYYY-MM-DD; "
                "approver=name"]
    errors = []
    try:
        expires = date.fromisoformat(match.group("expires"))
    except ValueError:
        errors.append("malformed expiry date")
        return errors
    if expires < date.today():
        errors.append(f"exception expired on {expires}")
    if not match.group("reason").strip():
        errors.append("empty exception reason")
    return errors


def scan(root: Path) -> list[str]:
    errors: list[str] = []
    for path in sorted(root.rglob("*.sh")):
        if any(part in {"build", ".git", ".claude", "third_party", "vendor"}
               for part in path.relative_to(root).parts):
            continue
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except (OSError, UnicodeDecodeError):
            continue
        if not any(PIPEFAIL.search(line) for line in lines):
            continue
        for number, line in logical_lines(lines):
            if line.lstrip().startswith("#") or not pipeline_hits(line):
                continue
            if "pipefail-ok:" in line:
                for problem in exceptions(line):
                    errors.append(f"{path.relative_to(root)}:{number}: {problem}")
                continue
            errors.append(
                f"{path.relative_to(root)}:{number}: pipefail pipeline has an "
                f"early-exit reader (grep -q/-m, head, sed q, awk exit); use a "
                f"here-string or let the reader consume all output")
    return errors


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root / "tools").mkdir()
        for index, (text, expected) in enumerate(SELF_CHECK):
            for old in (root / "tools").glob("*.sh"):
                old.unlink()
            path = root / "tools" / f"case{index}.sh"
            path.write_text(text, encoding="utf-8")
            got = len(scan(root))
            if got != expected:
                print(f"shell pipes self-check failed: case {index} got "
                      f"{got} finding(s), expected {expected}")
                return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    if not tree_check():
        return 1
    if args.self_check:
        return 0
    errors = scan(args.root)
    for error in errors:
        print(error)
    if errors:
        return 1
    print("shell pipes ok (no pipefail early-exit pipelines)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
