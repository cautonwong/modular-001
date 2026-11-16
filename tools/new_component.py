#!/usr/bin/env python3
"""Scaffold a component that already follows the standard.

Creates the manifest, a public header and one source file. It never edits
product.yaml; the final line prints the name to add there.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_component_standard as standard
import product_manifest

ROOT = Path(__file__).resolve().parent.parent


def guard(path: str) -> str:
    name = re.sub(r"[^A-Za-z0-9]+", "_", path).strip("_").upper()
    return f"ND_{name}_H"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--path", required=True,
                        help="repo-relative directory, for example domain/foo")
    parser.add_argument("--name", help="override the derived component name")
    parser.add_argument("--layer", help="manifest layer")
    parser.add_argument("--dep", action="append", default=[],
                        help="dependency component name, repeatable")
    args = parser.parse_args()

    path = Path(args.path).as_posix().strip("/")
    directory = ROOT / path
    if directory.exists():
        print(f"{path} already exists", file=sys.stderr)
        return 1

    expected = standard.expected_name(path)
    if expected is None:
        print(f"{path} does not belong to a standard root", file=sys.stderr)
        return 1
    name = args.name or expected
    if name != expected:
        print(f"name {name!r} does not match the standard {expected!r}",
              file=sys.stderr)
        return 1

    layers = standard.allowed_layers(path)
    layer = args.layer or (sorted(layers)[0] if layers else None)
    if layer not in layers:
        print(f"layer {layer!r} is not allowed for {path} (choose from "
              f"{sorted(layers)})", file=sys.stderr)
        return 1

    index = product_manifest.component_index()
    for dependency in args.dep:
        if dependency not in index:
            print(f"unknown dependency {dependency!r}", file=sys.stderr)
            return 1

    stem = path.rsplit("/", 1)[-1]
    directory.mkdir(parents=True)
    (directory / "component.yaml").write_text(
        f"name: {name}\n"
        f"version: 0.1.0\n"
        f"layer: {layer}\n"
        f"paths: [{path}]\n"
        f"dependencies: [{', '.join(args.dep)}]\n",
        encoding="utf-8")
    (directory / f"{stem}.h").write_text(
        f"#ifndef {guard(path)}\n"
        f"#define {guard(path)}\n"
        f"\n"
        f"/* Public interface for {name}. */\n"
        f"\n"
        f"#endif\n",
        encoding="utf-8")
    (directory / f"{stem}.c").write_text(
        f'#include "{path}/{stem}.h"\n',
        encoding="utf-8")

    print(f"created {path} ({name}, layer {layer})")
    print(f"next: add {name} to the product.yaml components list, then run ./ci.sh")
    return 0


if __name__ == "__main__":
    sys.exit(main())
