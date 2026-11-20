#!/usr/bin/env python3
"""Check the stategrid metadata budget counts against the registration calls.

ram_budget.h sizes the stategrid_meter metadata arena from the number of
block_device_t and resource_block_t objects the boot code allocates. Those
objects come from the registration API in
targets/stategrid_meter/tools/app_resource_tool/resource_tool.c:

  stategrid_context_create  allocates one block_device_t and one resource_block_t
  add_device                allocates one block_device_t
  register_map              allocates one block_device_t-sized record
  add_block                 allocates one resource_block_t
  add_inside_block          allocates one resource_block_t
  register_parameter        allocates one resource_block_t through __register
  register_log              allocates one resource_block_t through __register

The scan reads the product sources under targets/stategrid_meter/app, so a
call added without bumping ram_budget.h fails here instead of at boot time.
A file-scope occurrence is a prototype or definition, so only calls inside a
function body count.
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
import tempfile
from collections import Counter
from pathlib import Path

BUDGET = Path("targets/stategrid_meter/app/ram_budget.h")
APP = Path("targets/stategrid_meter/app")

# name -> (block_device_t, resource_block_t) charged per call
CALLS = {
    "stategrid_context_create": (1, 1),
    "add_device": (1, 0),
    "register_map": (1, 0),
    "add_block": (0, 1),
    "add_inside_block": (0, 1),
    "register_parameter": (0, 1),
    "register_log": (0, 1),
}

COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)
STRING = re.compile(r'"(?:\\.|[^"\\])*"')
CALL = re.compile(r"\b(" + "|".join(CALLS) + r")\s*\(")
DEFINE = re.compile(r"#\s*define\s+(STG_RAM_METADATA_(?:BLOCKS|DEVICES))\s+(\d+)[uU]?\b")

EXTRA_BLOCK = """
static void stg_ram_budget_extra_block(stategrid_context_t *instance)
{
    (void)add_block(instance, RES_DEV_RAM, control_block_cache, 4u);
}
"""

DEVICE_CALL = re.compile(r"\badd_device\s*\(\s*instance\s*,\s*RES_DEV_EEPROM\s*,\s*0x00\s*\)")


def collect(root: Path) -> tuple[dict[str, int], Counter]:
    budget: dict[str, int] = {}
    text = COMMENT.sub(" ", (root / BUDGET).read_text(encoding="utf-8", errors="replace"))
    for name, value in DEFINE.findall(text):
        budget[name] = int(value)
    for name in ("STG_RAM_METADATA_BLOCKS", "STG_RAM_METADATA_DEVICES"):
        if name not in budget:
            raise SystemExit(f"{root / BUDGET}: missing {name}")

    counts: Counter = Counter()
    for path in sorted((root / APP).rglob("*.c")):
        body = STRING.sub('""', COMMENT.sub(" ", path.read_text(encoding="utf-8",
                                                              errors="replace")))
        depth = pos = 0
        for match in CALL.finditer(body):
            depth += body.count("{", pos, match.start()) - body.count("}", pos, match.start())
            pos = match.start()
            if depth > 0:
                counts[match.group(1)] += 1
    return budget, counts


def totals(counts: Counter) -> tuple[int, int]:
    blocks = sum(blocks * counts[name] for name, (_dev, blocks) in CALLS.items())
    devices = sum(devices * counts[name] for name, (devices, _blk) in CALLS.items())
    return blocks, devices


def run(root: Path, quiet: bool = False) -> int:
    budget, counts = collect(root)
    blocks, devices = totals(counts)
    failures = []
    if blocks != budget["STG_RAM_METADATA_BLOCKS"]:
        failures.append(f"STG_RAM_METADATA_BLOCKS is {budget['STG_RAM_METADATA_BLOCKS']}"
                        f" but the call sites allocate {blocks} resource_block_t")
    if devices != budget["STG_RAM_METADATA_DEVICES"]:
        failures.append(f"STG_RAM_METADATA_DEVICES is {budget['STG_RAM_METADATA_DEVICES']}"
                        f" but the call sites allocate {devices} block_device_t")
    if not quiet:
        print(f"ram_budget.h: STG_RAM_METADATA_BLOCKS={budget['STG_RAM_METADATA_BLOCKS']}"
              f" STG_RAM_METADATA_DEVICES={budget['STG_RAM_METADATA_DEVICES']}")
        detail = " ".join(f"{name}={counts[name]}" for name in CALLS)
        print(f"call sites: {detail} -> blocks={blocks} devices={devices}")
        for failure in failures:
            print(failure, file=sys.stderr)
    return 1 if failures else 0


def self_check(root: Path) -> int:
    if run(root, quiet=True) != 0:
        print("self-check: the current tree does not pass", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory() as tmp:
        tmp_root = Path(tmp)

        extra = tmp_root / "extra"
        shutil.copytree(root / APP, extra / APP)
        entry = extra / APP / "stategrid_meter_entry.c"
        entry.write_text(entry.read_text(encoding="utf-8") + EXTRA_BLOCK, encoding="utf-8")
        if run(extra, quiet=True) == 0:
            print("self-check: one extra add_block was accepted", file=sys.stderr)
            return 1

        missing = tmp_root / "missing"
        shutil.copytree(root / APP, missing / APP)
        entry = missing / APP / "stategrid_meter_entry.c"
        text, drops = DEVICE_CALL.subn("1", entry.read_text(encoding="utf-8"), count=1)
        if drops != 1:
            print("self-check: no add_device call to drop", file=sys.stderr)
            return 1
        entry.write_text(text, encoding="utf-8")
        if run(missing, quiet=True) == 0:
            print("self-check: one missing add_device was accepted", file=sys.stderr)
            return 1

    print("self-check ok")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path,
                        help="repository root; defaults to this script's parent's parent")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    root = args.root or Path(__file__).resolve().parent.parent
    return self_check(root) if args.self_check else run(root)


if __name__ == "__main__":
    raise SystemExit(main())
