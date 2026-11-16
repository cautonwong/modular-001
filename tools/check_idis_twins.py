#!/usr/bin/env python3
"""Keep the two IDIS target trees from drifting apart.

The baremetal and FreeRTOS targets carry copies of the same COSEM and mapper
sources. Every shared file must match once the target directory name is
normalised. A file present on one side only also fails, so the twin is
always either synchronised or deleted from both.
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TWINS = (
    ("targets/idis_ip_meter/cosem", "targets/idis_ip_meter_freertos/cosem"),
    ("targets/idis_ip_meter/mapper_idis", "targets/idis_ip_meter_freertos/mapper_idis"),
)
NAMES = ("idis_ip_meter_freertos", "idis_ip_meter")


def normalise(path: Path) -> str:
    text = path.read_text(encoding="utf-8", errors="replace")
    for name in NAMES:
        text = text.replace(name, "idis_ip_meter")
    return text


def compare(left_dir: Path, right_dir: Path) -> list[str]:
    errors: list[str] = []
    left = {path.relative_to(left_dir) for path in left_dir.rglob("*") if path.is_file()}
    right = {path.relative_to(right_dir) for path in right_dir.rglob("*") if path.is_file()}
    for rel in sorted(left - right):
        errors.append(f"{left_dir / rel}: missing in {right_dir}")
    for rel in sorted(right - left):
        errors.append(f"{right_dir / rel}: missing in {left_dir}")
    for rel in sorted(left & right):
        if normalise(left_dir / rel) != normalise(right_dir / rel):
            errors.append(f"{left_dir / rel}: differs from {right_dir / rel}")
    return errors


def main() -> int:
    errors: list[str] = []
    count = 0
    for left_name, right_name in TWINS:
        left = ROOT / left_name
        right = ROOT / right_name
        if not left.is_dir() or not right.is_dir():
            errors.append(f"missing twin directory: {left} or {right}")
            continue
        errors.extend(compare(left, right))
        count += sum(1 for path in left.rglob("*") if path.is_file())
    for error in errors:
        print(error)
    if errors:
        return 1
    print(f"idis twins ok ({count} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
