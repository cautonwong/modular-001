#!/usr/bin/env python3
"""Keep instance and meter-base layouts out of domain and application code."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
SCAN_DIRS = ("domain", "application")

FORBIDDEN_INCLUDES = (
    "kernel/instance.h",
    "kernel/legacy_control_block.h",
)
FORBIDDEN_DEFINITIONS = (
    re.compile(r"struct\s+instance_t\s*\{"),
    re.compile(r"struct\s+mbase_control_block_t\s*\{"),
)


def main() -> int:
    failures = []
    for directory in SCAN_DIRS:
        for path in sorted((ROOT / directory).rglob("*")):
            if path.suffix not in (".c", ".h"):
                continue
            text = path.read_text(errors="replace")
            rel = path.relative_to(ROOT)
            for include in FORBIDDEN_INCLUDES:
                if re.search(r"#\s*include\s*[<\"][^>\"]*" + re.escape(include), text):
                    failures.append(f"{rel}: forbidden include {include}")
            for pattern in FORBIDDEN_DEFINITIONS:
                if pattern.search(text):
                    failures.append(f"{rel}: forbidden layout definition {pattern.pattern}")

    if failures:
        for failure in failures:
            print(failure)
        return 1
    print("domain and application keep instance and meter-base layouts opaque")
    return 0


if __name__ == "__main__":
    sys.exit(main())
