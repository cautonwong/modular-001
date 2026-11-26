#!/usr/bin/env python3
"""Keep tariff table limits defined only in domain/tariff.

The legacy mappers used their own MaxSeasonNum/MaxTouNum/MaxFeeNum/
MaxHolidayNum/MaxTouTabNum copies; the table validator reads the
ND_TARIFF_MAX_* values from domain/tariff/tariff.h. A second definition of
any legacy name outside domain/tariff means a mapper can drift from the
validated limits again.
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASES = ("domain", "application", "components", "targets", "infra")
SKIP_PARTS = {"build", "backupfiles"}
LEGACY = ("MaxSeasonNum", "MaxTouNum", "MaxFeeNum", "MaxHolidayNum",
          "MaxTouTabNum")
DEFINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(" + "|".join(LEGACY) + r")\b",
                    re.MULTILINE)
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)


def strip_comments(text: str) -> str:
    return COMMENTS.sub(lambda match: "\n" * match.group(0).count("\n"), text)


def scan(root: Path = ROOT) -> list[str]:
    found: list[str] = []
    for base in BASES:
        if not (root / base).is_dir():
            continue
        for path in sorted((root / base).rglob("*")):
            if path.suffix not in (".c", ".h") or SKIP_PARTS & set(path.parts):
                continue
            rel = path.relative_to(root).as_posix()
            if rel.startswith("domain/tariff/"):
                continue
            text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
            for match in DEFINE.finditer(text):
                line = text.count("\n", 0, match.start()) + 1
                found.append(f"{rel}:{line}: defines {match.group(1)} outside "
                             "domain/tariff; use ND_TARIFF_MAX_*")
    return found


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root / "domain" / "tariff").mkdir(parents=True)
        (root / "domain" / "tariff" / "tariff.h").write_text(
            "#define ND_TARIFF_MAX_FEES 12\n", encoding="utf-8")
        (root / "targets" / "app").mkdir(parents=True)
        (root / "targets" / "app" / "clean.c").write_text(
            "int uses_limits(void) { return ND_TARIFF_MAX_FEES; }\n",
            encoding="utf-8")
        if scan(root):
            print(f"tariff limits self-check failed: clean tree reported "
                  f"{scan(root)}")
            return False
        (root / "targets" / "app" / "leak.h").write_text(
            "#define MaxFeeNum 12\n", encoding="utf-8")
        found = scan(root)
        if not any("MaxFeeNum" in message for message in found):
            print(f"tariff limits self-check failed: duplicate define reported "
                  f"{found}")
            return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if not tree_check():
        return 1
    found = scan()
    if args.self_check and not found:
        print("tariff limits self-check ok (domain define accepted, target "
              "copy rejected)")
    for message in found:
        print(f"error: {message}", file=sys.stderr)
    if found:
        return 1
    print("tariff limits ok (defined only in domain/tariff)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
