#!/usr/bin/env python3
"""Build the IDIS object registry blob from the host audit.

Path C in docs/idis-object-registry.md. The blob is written to EEPROM by the
production tool; the firmware validates and loads it at boot.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

MAGIC = b"NDRG"
FORMAT = 1
ENTRY_SIZE = 8
HEADER_SIZE = 8
TRAILER_SIZE = 2
MAX_ENTRIES = 360
DEFAULT_OFFSET = 55296
EEPROM_SIZE = 64 * 1024

AUDIT = re.compile(r"^AUDIT (\d+) (\S+) (\d+) (-?\d+)$")
OBIS = re.compile(r"^(\d+)-(\d+):(\d+)\.(\d+)\.(\d+)\.(\d+)$")


def audit_entries(runner: Path) -> list[tuple[int, bytes]]:
    with tempfile.TemporaryDirectory(prefix="idis-registry-") as work:
        output = subprocess.check_output(
            [str(runner), "--backup-files", work, "--seconds", "0", "--audit"],
            text=True)
    entries: dict[tuple[int, bytes], None] = {}
    for line in output.splitlines():
        match = AUDIT.match(line)
        if not match or match.group(3) != "1" or match.group(4) != "0":
            continue
        class_id = int(match.group(1))
        obis = OBIS.match(match.group(2))
        if obis is None or not 0 < class_id < 256:
            continue
        raw = bytes(int(part) for part in obis.groups())
        if any(value > 255 for value in raw):
            continue
        entries[(class_id, raw)] = None
    return list(entries)


def build_blob(entries: list[tuple[int, bytes]]) -> bytes:
    if len(entries) > MAX_ENTRIES:
        raise SystemExit(
            f"{len(entries)} entries do not fit the {MAX_ENTRIES}-entry budget")
    body = bytearray()
    body += MAGIC
    body += bytes([FORMAT, ENTRY_SIZE])
    body += len(entries).to_bytes(2, "big")
    for class_id, raw in entries:
        body += bytes([class_id, 0])
        body += raw
    checksum = sum(body) & 0xFFFF
    body += checksum.to_bytes(2, "big")
    return bytes(body)


def patch_image(path: Path, offset: int, blob: bytes) -> None:
    if path.exists():
        image = bytearray(path.read_bytes())
    else:
        image = bytearray(EEPROM_SIZE)
    if len(image) < offset + len(blob):
        image.extend(b"\x00" * (offset + len(blob) - len(image)))
    image[offset:offset + len(blob)] = blob
    path.write_bytes(image)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--eeprom", type=Path,
                        help="also patch a raw EEPROM image")
    parser.add_argument("--offset", type=int, default=DEFAULT_OFFSET)
    args = parser.parse_args()

    if not args.runner.is_file():
        print(f"runner not found: {args.runner}", file=sys.stderr)
        return 1

    entries = audit_entries(args.runner)
    blob = build_blob(entries)
    args.out.write_bytes(blob)
    if args.eeprom is not None:
        patch_image(args.eeprom, args.offset, blob)
    print(f"registry: {len(entries)} entries, {len(blob)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
