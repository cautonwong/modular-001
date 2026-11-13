#!/usr/bin/env python3
"""Resolve a product descriptor into the component dependency closure.

The output is a deterministic source list for a product build. This keeps
product.yaml as the selection input while the legacy IAR project remains the
authoritative firmware build until the CMake firmware target lands.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from check_components import ROOT as ROOT_TEXT, discover, parse_manifest

ROOT = Path(ROOT_TEXT)


def component_index():
    return {component["name"]: component for component in discover()}


def product_paths():
    return sorted((ROOT / "products").glob("*/product.yaml"))


def closure(product_file: Path, index: dict) -> list[str]:
    data = parse_manifest(product_file)
    selected = data.get("components", [])
    errors = []
    ordered = []
    seen = set()

    def visit(name):
        if name in seen:
            return
        if name not in index:
            errors.append(f"{product_file.relative_to(ROOT)}: unknown component {name}")
            return
        seen.add(name)
        for dependency in index[name].get("dependencies", []):
            visit(dependency)
        ordered.append(name)

    for name in selected:
        visit(name)
    return ordered, errors


def component_sources(component) -> list[Path]:
    paths = []
    excludes = [ROOT / path for path in component.get("exclude", [])]
    for owned in component.get("paths", []):
        root = ROOT / owned
        if root.is_file():
            if root.suffix == ".c" and root not in excludes:
                paths.append(root)
            continue
        if not root.is_dir():
            continue
        for source in sorted(root.rglob("*.c")):
            if any(source == ex or ex in source.parents for ex in excludes):
                continue
            if "tests" in source.parts:
                continue
            paths.append(source)
    return paths


def render_cmake(product_file: Path, index: dict) -> str:
    data = parse_manifest(product_file)
    components = []
    errors = []
    components, errors = closure(product_file, index)
    if errors:
        raise SystemExit("\n".join(errors))
    sources = []
    for name in components:
        sources.extend(component_sources(index[name]))
    lines = [
        f'set(PRODUCT_NAME "{data.get("name", product_file.parent.name)}")',
        "set(PRODUCT_COMPONENTS",
    ]
    lines.extend(f'    "{name}"' for name in components)
    lines.append(")")
    lines.append("set(PRODUCT_SOURCES")
    lines.extend(f'    "{source.relative_to(ROOT).as_posix()}"' for source in sources)
    lines.append(")")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check-all", action="store_true")
    parser.add_argument("--cmake", metavar="PRODUCT")
    parser.add_argument("--output", metavar="FILE")
    args = parser.parse_args()

    index = component_index()
    if args.check_all:
        failed = False
        for product_file in product_paths():
            components, errors = closure(product_file, index)
            if errors:
                failed = True
                for error in errors:
                    print(error)
            else:
                print(f"{product_file.parent.name}: {len(components)} components")
        return 1 if failed else 0

    if args.cmake:
        product_file = ROOT / "products" / args.cmake / "product.yaml"
        if not product_file.exists():
            print(f"unknown product: {args.cmake}", file=sys.stderr)
            return 1
        rendered = render_cmake(product_file, index)
        if args.output:
            target = Path(args.output)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(rendered)
        else:
            sys.stdout.write(rendered)
        return 0

    parser.error("select --check-all or --cmake PRODUCT")
    return 2


if __name__ == "__main__":
    sys.exit(main())
