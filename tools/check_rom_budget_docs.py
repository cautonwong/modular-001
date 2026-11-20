#!/usr/bin/env python3
"""Keep the IDIS ROM budget identical in the linker, checker and docs.

ROM_BYTES in check_idis_firmware.py is the single source of truth. Every
linker assert, review rule and doc sentence quotes it as "<K> KiB"; this
check fails when one of them still says 102 KiB or disagrees.
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
import tempfile
from pathlib import Path

BUDGET_SOURCE = Path("tools/check_idis_firmware.py")
BUDGET = re.compile(r"^ROM_BYTES = (\d+) \* (\d+)$", re.MULTILINE)

STALE = ("102 KiB", "102K")


def markers(k: int) -> dict[Path, tuple[str, ...]]:
    rom = f"ORIGIN(ROM) + {k}K"
    limit = f"product limit is {k} KiB"
    return {
        Path("targets/idis_ip_meter/build/idis.ld"): (rom, limit),
        Path("targets/idis_ip_meter_freertos/build/idis.ld"): (rom, limit),
        Path("AGENTS.md"): (f"IDIS 的 {k} KiB 链接断言",
                            f"ROM 必须小于 {k} KiB"),
        Path(".opencodereview/rule.json"): (f"IDIS {k} KiB",),
        Path("docs/guides/product-platform-extension.md"): (f"{k} KiB",),
        Path("docs/idis-object-registry.md"): (f"{k} KiB",),
    }


def budget_kib(root: Path) -> int:
    text = (root / BUDGET_SOURCE).read_text(encoding="utf-8")
    match = BUDGET.search(text)
    if match is None:
        raise SystemExit(f"{root / BUDGET_SOURCE}: no 'ROM_BYTES = <n> * 1024'")
    return int(match.group(1)) * int(match.group(2)) // 1024


def run(root: Path, quiet: bool = False) -> int:
    k = budget_kib(root)
    failures: list[str] = []
    for rel, required in markers(k).items():
        path = root / rel
        if not path.exists():
            failures.append(f"{rel}: missing")
            continue
        text = path.read_text(encoding="utf-8")
        failures += [f"{rel}: missing {needle!r}"
                     for needle in required if needle not in text]
        if k != 102:
            failures += [f"{rel}: stale {stale!r}"
                         for stale in STALE if stale in text]
    if not quiet:
        print(f"IDIS ROM budget {k} KiB")
        for failure in failures:
            print(failure, file=sys.stderr)
    return 1 if failures else 0


def self_check(root: Path) -> int:
    if run(root, quiet=True) != 0:
        print("self-check: the current tree does not pass", file=sys.stderr)
        return 1

    k = budget_kib(root)
    with tempfile.TemporaryDirectory() as tmp:
        tmp_root = Path(tmp)
        for rel in set(markers(k)) | {BUDGET_SOURCE}:
            if (root / rel).exists():
                (tmp_root / rel).parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(root / rel, tmp_root / rel)

        target = Path("targets/idis_ip_meter/build/idis.ld")
        text = (tmp_root / target).read_text(encoding="utf-8")
        text = text.replace(f"{k}K", "102K").replace(f"{k} KiB", "102 KiB")
        (tmp_root / target).write_text(text, encoding="utf-8")
        if run(tmp_root, quiet=True) == 0:
            print("self-check: a 102 KiB regression was accepted",
                  file=sys.stderr)
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
