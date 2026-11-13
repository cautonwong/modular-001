#!/usr/bin/env python3
"""Guard the domain layer against COSEM/DLMS/target leakage."""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "domain"

BANNED = [
    r"\bnd_obis_t\b",
    r"\bND_OBIS_",
    r"\bclass_id\b",
    r"\battribute_id\b",
    r"\bnd_axdr",
    r'"components/protocol/dlms',
    r'"targets/',
]


def main():
    violations = []
    for path in sorted(DOMAIN.rglob("*.h")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for pattern in BANNED:
            for match in re.finditer(pattern, text):
                line = text.count("\n", 0, match.start()) + 1
                violations.append(f"{path.relative_to(ROOT)}:{line}: {pattern}")
    if violations:
        print("domain contract violations:")
        print("\n".join(violations))
        return 1
    print("domain contracts clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
