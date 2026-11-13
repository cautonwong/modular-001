#!/usr/bin/env python3
"""Link a product source list into a final GNU ELF using the V85XXP map."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import legacy_baseline as baseline
import product_manifest

ROOT = Path(baseline.ROOT)
GNU_DIR = ROOT / "targets/stategrid_meter/firmware/gnu"


def compile_one(source: Path, obj: Path, flags: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["arm-none-eabi-gcc", *flags, "-c", str(source), "-o", str(obj)],
        capture_output=True,
        text=True,
        errors="replace",
        timeout=60,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--keep", action="store_true")
    parser.add_argument("--product")
    args = parser.parse_args()

    product_sources = None
    if args.product:
        product_file = ROOT / "products" / args.product / "product.yaml"
        if not product_file.exists():
            print(f"unknown product: {args.product}")
            return 1
        index = product_manifest.component_index()
        ordered, errors = product_manifest.closure(product_file, index)
        if errors:
            print("\n".join(errors))
            return 1
        product_sources = []
        seen = set()
        for name in ordered:
            for source in product_manifest.component_sources(index[name]):
                key = os.path.normpath(str(source))
                if key not in seen:
                    seen.add(key)
                    product_sources.append(key)

    import xml.etree.ElementTree as ET

    tree = ET.parse(baseline.EWP)
    configuration = baseline.first_configuration(tree)
    includes = [baseline.resolve(path)
                for path in baseline.collect_option(configuration, "CCIncludePath2")]
    defines = baseline.collect_option(configuration, "CCDefines")
    sources = baseline.collect_sources(tree)

    if product_sources is not None:
        ewp_sources = {os.path.normpath(source) for source in sources}
        missing = sorted(set(product_sources) - ewp_sources)
        extra = sorted(ewp_sources - set(product_sources))
        if missing or extra:
            if missing:
                print(f"product {args.product} sources missing from the IAR project:")
                for source in missing:
                    print("  " + os.path.relpath(source, ROOT))
            if extra:
                print(f"IAR project sources outside product {args.product}:")
                for source in extra:
                    print("  " + os.path.relpath(source, ROOT))
            print("the product source list and the .ewp must match; "
                  "update the manifest or the IAR project")
            return 1
        sources = product_sources

    flags = [
        "-std=gnu11",
        "-mcpu=cortex-m0",
        "-mthumb",
        "-ffreestanding",
        "-w",
        "-fpermissive",
        "-Os",
        "-ffunction-sections",
        "-fdata-sections",
        "-I",
        str(ROOT),
    ]
    for path in includes:
        flags += ["-I", path]
    for define in defines:
        flags.append("-D" + define)

    build_dir = Path(tempfile.mkdtemp(prefix="blade-firmware-"))
    objects = []
    for index, source in enumerate(sources):
        if not os.path.isfile(source):
            continue
        obj = build_dir / f"{index:03d}.o"
        result = compile_one(Path(source), obj, flags)
        if result.returncode != 0:
            print(f"compile failed: {os.path.relpath(source, ROOT)}")
            print(result.stderr[:2000])
            if not args.keep:
                shutil.rmtree(build_dir, ignore_errors=True)
            return 1
        objects.append(obj)

    for extra in ("startup.c", "link_stubs.c"):
        result = compile_one(GNU_DIR / extra, build_dir / extra.replace(".c", ".o"), flags)
        if result.returncode != 0:
            print(f"compile failed: {extra}")
            print(result.stderr[:2000])
            if not args.keep:
                shutil.rmtree(build_dir, ignore_errors=True)
            return 1
        objects.append(build_dir / extra.replace(".c", ".o"))

    elf = build_dir / "firmware.elf"
    link = [
        "arm-none-eabi-gcc",
        "-mcpu=cortex-m0",
        "-mthumb",
        "-nostartfiles",
        "-T",
        str(GNU_DIR / "link.ld"),
        "-Wl,--gc-sections",
        "-Wl,-Map," + str(build_dir / "firmware.map"),
        "-o",
        str(elf),
        *(str(obj) for obj in objects),
        "-Wl,--start-group",
        "-lc",
        "-lgcc",
        "-lnosys",
        "-Wl,--end-group",
    ]
    result = subprocess.run(link, capture_output=True, text=True, errors="replace")
    if result.returncode != 0:
        print("firmware link failed")
        print(result.stderr[:4000])
        if args.keep:
            print(f"kept {build_dir}")
        else:
            shutil.rmtree(build_dir, ignore_errors=True)
        return 1

    undefined = subprocess.run(
        ["arm-none-eabi-nm", "-u", str(elf)],
        capture_output=True,
        text=True,
        errors="replace",
    )
    names = [line.split()[-1] for line in undefined.stdout.splitlines() if line.split()]
    if names:
        print("unexpected undefined symbols: " + ", ".join(names[:20]))
        if not args.keep:
            shutil.rmtree(build_dir, ignore_errors=True)
        return 1

    print(f"firmware link ok ({len(objects)} objects, {elf.stat().st_size} bytes)")
    if args.keep:
        print(f"kept {build_dir}")
    else:
        shutil.rmtree(build_dir, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
