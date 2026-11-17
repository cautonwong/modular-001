#!/usr/bin/env python3
"""Enforce the device-driver boundary under infra/drivers.

Drivers talk to a bus injected by the board. They must not reach into the
legacy kernel, the target tree or a SoC header, and every driver directory
carries its own component manifest. See docs/guides/device-driver.md.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DRIVERS = ROOT / "infra" / "drivers"

INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
BANNED_INCLUDES = (
    "type_def.h",
    "target.h",
    "cpu_surport.h",
    "FreeRTOS.h",
    "zephyr.h",
    "cmsis_os2.h",
    "lib_uart.h",
    "lib_gpio.h",
)
BANNED_PREFIXES = (
    "kernel/",
    "targets/",
    "application/",
    "domain/",
    "components/",
)
BANNED_TOKENS = (
    "struct dev_info_t",
    "struct bus_ops_t",
    "driver_ops_t",
    "dev_driver_ops",
    "dev_bus_ops",
)


def driver_dirs() -> list[Path]:
    if not DRIVERS.is_dir():
        return []
    return sorted(path for path in DRIVERS.glob("*/*") if path.is_dir())


def main() -> int:
    errors: list[str] = []
    files = 0
    for directory in driver_dirs():
        sources = sorted(directory.glob("*.[ch]"))
        if not sources:
            continue
        manifest = directory / "component.yaml"
        if not manifest.is_file():
            errors.append(f"{directory.relative_to(ROOT)}: missing component.yaml")
        else:
            text = manifest.read_text(encoding="utf-8", errors="replace")
            if "layer: platform" not in text:
                errors.append(f"{manifest.relative_to(ROOT)}: layer must be platform")
        for path in sources:
            files += 1
            rel = path.relative_to(ROOT)
            for number, line in enumerate(
                    path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
                match = INCLUDE.match(line)
                if match:
                    target = match.group(1)
                    if target in BANNED_INCLUDES or target.startswith(BANNED_PREFIXES):
                        errors.append(f"{rel}:{number}: driver may not include {target}")
                for token in BANNED_TOKENS:
                    if token in line:
                        errors.append(f"{rel}:{number}: legacy token {token!r}")
    for error in errors:
        print(error)
    if errors:
        return 1
    print(f"driver boundary ok ({files} files in {len(driver_dirs())} drivers)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
