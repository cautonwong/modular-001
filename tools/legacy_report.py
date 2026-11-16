#!/usr/bin/env python3
"""List legacy components, their sources, and whether the IAR project builds them."""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path

from check_components import ROOT as ROOT_TEXT, discover

ROOT = Path(ROOT_TEXT)
EWP = ROOT / "targets/stategrid_meter/iar for arm/VG_DTZY.ewp"


def ewp_sources() -> set[Path]:
    text = EWP.read_text(encoding="latin-1")
    sources: set[Path] = set()
    for name in re.findall(r"<name>\$PROJ_DIR\$([^<]+\.c)</name>", text):
        path = os.path.normpath(
            os.path.join(EWP.parent, name.replace("\\", "/").lstrip("/")))
        sources.add(Path(path).resolve())
    return sources


def owned_sources(component) -> list[Path]:
    excludes = [ROOT / path for path in component.get("exclude", [])]
    files: list[Path] = []
    for owned in component.get("paths", []):
        root = ROOT / owned
        if root.is_file():
            if root.suffix == ".c":
                files.append(root)
            continue
        if not root.is_dir():
            continue
        for source in sorted(root.rglob("*.c")):
            if any(source == ex or ex in source.parents for ex in excludes):
                continue
            files.append(source)
    return files


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    built = ewp_sources()
    rows = []
    for component in sorted(discover(), key=lambda item: item.get("name", "")):
        if not component.get("legacy"):
            continue
        for source in owned_sources(component):
            rel = source.relative_to(ROOT).as_posix()
            lines = sum(1 for _ in open(source, encoding="utf-8", errors="replace"))
            rows.append({
                "component": component["name"],
                "file": rel,
                "lines": lines,
                "built": source.resolve() in built,
            })

    if args.json:
        print(json.dumps(rows, indent=2))
        return 0

    print(f"{'component':40} {'lines':>6}  {'build':6} file")
    for row in rows:
        print(f"{row['component']:40} {row['lines']:6}  "
              f"{'yes' if row['built'] else 'no':6} {row['file']}")
    total = sum(row["lines"] for row in rows)
    excluded = sum(1 for row in rows if not row["built"])
    print(f"\n{len(rows)} files, {total} lines, {excluded} not in the IAR project")
    return 0


if __name__ == "__main__":
    sys.exit(main())
