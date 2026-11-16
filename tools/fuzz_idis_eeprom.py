#!/usr/bin/env python3
"""Corrupt IDIS EEPROM pages and require the host runner to survive.

A deterministic sample, not a random fuzzer. It seeds one EEPROM image with
the sanitized runner, flips one byte per stride, and runs again. A non-zero
exit or a sanitizer report fails the step.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def run(runner: Path, directory: Path, seconds: int) -> subprocess.CompletedProcess:
    return subprocess.run(
        [str(runner), "--backup-files", str(directory), "--seconds", str(seconds)],
        capture_output=True, text=True, timeout=60)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--workdir", type=Path, required=True)
    parser.add_argument("--stride", type=int, default=512)
    parser.add_argument("--value", type=lambda text: int(text, 0), default=0xFF)
    parser.add_argument("--seconds", type=int, default=1)
    args = parser.parse_args()

    if args.stride < 1:
        parser.error("--stride must be positive")
    if not args.runner.is_file():
        parser.error(f"runner not found: {args.runner}")

    if args.workdir.exists():
        shutil.rmtree(args.workdir)
    args.workdir.mkdir(parents=True)
    seed = args.workdir / "seed"
    seed.mkdir()
    result = run(args.runner, seed, args.seconds)
    if result.returncode != 0:
        print(f"seed run failed ({result.returncode}): {result.stderr[-400:]}")
        return 1
    image = seed / "eeprom.bin"
    flash = seed / "flash.bin"
    if not image.is_file():
        print(f"seed run produced no {image}")
        return 1
    data = image.read_bytes()

    failures = 0
    checked = 0
    for offset in range(0, len(data), args.stride):
        with tempfile.TemporaryDirectory(dir=args.workdir) as tmp:
            case = Path(tmp)
            corrupted = bytearray(data)
            corrupted[offset] ^= args.value
            (case / "eeprom.bin").write_bytes(corrupted)
            if flash.is_file():
                shutil.copyfile(flash, case / "flash.bin")
            result = run(args.runner, case, args.seconds)
            checked += 1
            if result.returncode != 0:
                failures += 1
                print(f"offset {offset} value 0x{args.value:02X}: "
                      f"exit {result.returncode}: {result.stderr[-400:]}")
    print(f"eeprom corruption ok ({checked} offsets, stride {args.stride})")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
