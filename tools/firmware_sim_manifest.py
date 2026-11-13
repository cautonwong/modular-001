#!/usr/bin/env python3
"""Emit the source and include lists for the host firmware simulation.

The simulation reuses the authoritative IAR project's source list, minus the
register-level board, SOC and driver-entry sources that are replaced by
tests/sim stubs and file-backed devices. Keeping this derived from the .ewp
means the simulation cannot silently drift away from the firmware build.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
EWP = ROOT / "targets/stategrid_meter/iar for arm/VG_DTZY.ewp"
PROJECT_DIR = ROOT / "targets/stategrid_meter/iar for arm"

EXCLUDED = (
    "/board/",
    "/boards/",
    "targets/stategrid_meter/app/driver_entry.c",
    "infra/soc/",
)


def ewp_sources() -> list[str]:
    text = EWP.read_text(encoding="latin-1")
    names = re.findall(r"<name>\$PROJ_DIR\$([^<]+\.c)</name>", text)
    result = []
    for name in names:
        path = os.path.normpath(os.path.join(PROJECT_DIR, name.replace("\\", "/").lstrip("/")))
        relative = os.path.relpath(path, ROOT).replace(os.sep, "/")
        if any(marker in relative for marker in EXCLUDED):
            continue
        result.append(relative)
    return sorted(set(result))


def ewp_includes() -> list[str]:
    text = EWP.read_text(encoding="latin-1")
    result = ["tests/sim/include", "tests", "."]
    for name in re.findall(r"<state>\$PROJ_DIR\$([^<]+)</state>", text):
        path = os.path.normpath(os.path.join(PROJECT_DIR, name.replace("\\", "/").lstrip("/")))
        if not os.path.isdir(path):
            continue
        relative = os.path.relpath(path, ROOT).replace(os.sep, "/")
        if relative not in result:
            result.append(relative)
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cmake", action="store_true", help="write CMake set() blocks")
    parser.add_argument("--check", action="store_true", help="verify every source exists")
    args = parser.parse_args()

    sources = ewp_sources()
    includes = ewp_includes()

    if args.check:
        missing = [path for path in sources if not (ROOT / path).is_file()]
        if missing:
            for path in missing:
                print(f"missing source: {path}", file=sys.stderr)
            return 1
        print(f"firmware simulation sources: {len(sources)}")
        return 0

    if args.cmake:
        print("set(FIRMWARE_SIM_SOURCES")
        for path in sources:
            print(f'    "${{BLADE_ROOT}}/{path}"')
        print(")")
        print("set(FIRMWARE_SIM_INCLUDES")
        for path in includes:
            print(f'    "${{BLADE_ROOT}}/{path}"' if path != "." else '    "${BLADE_ROOT}"')
        print(")")
        return 0

    for path in sources:
        print(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
