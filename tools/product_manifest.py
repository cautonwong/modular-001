#!/usr/bin/env python3
"""Resolve a product descriptor into the component dependency closure.

The output is a deterministic source list for a product build. This keeps
product.yaml as the selection input; the IDIS CMake firmware targets consume
`--cmake`, and the legacy IAR project remains the other build path.

A product can narrow a selected component with `sources:`: a mapping from
component name to repository-relative .c files. A selected component without
an entry contributes every source it owns.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

from check_components import ROOT as ROOT_TEXT, discover, parse_manifest

ROOT = Path(ROOT_TEXT)
CYCLE_ALLOWLIST = ROOT / "tools/component_cycles.txt"


def cycle_allowlist():
    allowed = set()
    if not CYCLE_ALLOWLIST.exists():
        return allowed
    for line in CYCLE_ALLOWLIST.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) != 2:
            raise SystemExit(f"{CYCLE_ALLOWLIST}: bad cycle line {line!r}")
        allowed.add(tuple(sorted(parts)))
    return allowed


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
    visiting = set()
    allowed = cycle_allowlist()

    def visit(name, parent=None):
        if name in seen:
            return
        if name not in index:
            errors.append(f"{product_file.relative_to(ROOT)}: unknown component {name}")
            return
        if name in visiting:
            if parent is not None and tuple(sorted((parent, name))) in allowed:
                return
            errors.append(
                f"{product_file.relative_to(ROOT)}: dependency cycle at {name}")
            return
        visiting.add(name)
        for dependency in index[name].get("dependencies", []):
            visit(dependency, name)
        visiting.discard(name)
        seen.add(name)
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


def product_sources(product_file: Path, index: dict):
    """Resolve a product into ([(component, [source])], errors).

    Groups follow dependency order. A component under sources: contributes
    exactly the declared files; every other selected component contributes all
    sources it owns.
    """
    data = parse_manifest(product_file)
    selected = data.get("components", [])
    selection = data.get("sources", {})
    if "sources" in data and not isinstance(selection, dict):
        raise SystemExit(
            f"{product_file}: sources must be a component: path mapping")
    for name, value in selection.items():
        if not isinstance(value, (str, list)):
            raise SystemExit(
                f"{product_file}: sources {name} must be a path or list")
    selection = {name: [value] if isinstance(value, str) else value
                 for name, value in selection.items()}
    components, errors = closure(product_file, index)
    groups = []
    for name in components:
        component = index.get(name)
        if component is None:
            continue
        sources = component_sources(component)
        if name not in selection:
            groups.append((name, sources))
            continue
        owned = {os.path.normpath(str(source)): source for source in sources}
        chosen = []
        for text in selection[name]:
            key = os.path.normpath(str(ROOT / text))
            if key not in owned:
                errors.append(
                    f"{product_file.relative_to(ROOT)}: sources {text} is not "
                    f"a source of {name}")
            elif key in {os.path.normpath(str(source)) for source in chosen}:
                errors.append(
                    f"{product_file.relative_to(ROOT)}: sources {text} is "
                    f"declared twice for {name}")
            else:
                chosen.append(owned[key])
        groups.append((name, chosen))
    for name in selection:
        if name not in selected:
            errors.append(
                f"{product_file.relative_to(ROOT)}: sources names component "
                f"{name}, which the product does not select")
    return groups, errors


def render_cmake(product_file: Path, index: dict) -> str:
    data = parse_manifest(product_file)
    groups, errors = product_sources(product_file, index)
    if errors:
        raise SystemExit("\n".join(errors))
    lines = [
        f'set(PRODUCT_NAME "{data.get("name", product_file.parent.name)}")',
        "set(PRODUCT_COMPONENTS",
    ]
    lines.extend(f'    "{name}"' for name, _ in groups)
    lines.append(")")
    lines.append("set(PRODUCT_SOURCES")
    lines.extend(f'    "{source.relative_to(ROOT).as_posix()}"'
                 for _, sources in groups for source in sources)
    lines.append(")")
    return "\n".join(lines) + "\n"


def self_check() -> int:
    """Prove source selection in a throwaway tree and that it is a no-op."""
    global ROOT
    with tempfile.TemporaryDirectory(prefix="product-manifest-self-") as tmp:
        root = Path(tmp)
        for relative in ("demo/one.c", "demo/two.c", "other/three.c"):
            path = root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("", encoding="utf-8")
        product_file = root / "products/demo/product.yaml"
        product_file.parent.mkdir(parents=True)
        saved = ROOT
        ROOT = root
        try:
            index = {
                "demo.other": {"name": "demo.other", "paths": ["other"],
                               "dependencies": []},
                "demo.demo": {"name": "demo.demo", "paths": ["demo"],
                              "dependencies": ["demo.other"]},
            }
            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n", encoding="utf-8")
            if parse_manifest(product_file).get("sources"):
                print("product manifest self-check failed: a product without "
                      "sources selected files")
                return 1
            groups, errors = product_sources(product_file, index)
            flat = [source for _, sources in groups for source in sources]
            legacy = [source for name in closure(product_file, index)[0]
                      for source in component_sources(index[name])]
            if errors or flat != legacy or len(flat) != 3:
                print("product manifest self-check failed: a product without "
                      f"sources did not resolve exactly as before: {flat} {errors}")
                return 1

            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  demo.demo: demo/one.c\n", encoding="utf-8")
            groups, errors = product_sources(product_file, index)
            flat = [source.name for _, sources in groups for source in sources]
            if errors or flat != ["three.c", "one.c"]:
                print("product manifest self-check failed: a single declared "
                      f"source did not narrow its component: {flat} {errors}")
                return 1
            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  demo.demo: [demo/one.c, demo/two.c]\n",
                encoding="utf-8")
            groups, errors = product_sources(product_file, index)
            flat = [source.name for _, sources in groups for source in sources]
            if errors or flat != ["three.c", "one.c", "two.c"]:
                print("product manifest self-check failed: a bracketed source "
                      f"list did not resolve: {flat} {errors}")
                return 1
            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  demo.demo:\n    - demo/one.c\n    - demo/two.c\n",
                encoding="utf-8")
            groups, errors = product_sources(product_file, index)
            flat = [source.name for _, sources in groups for source in sources]
            if errors or flat != ["three.c", "one.c", "two.c"]:
                print("product manifest self-check failed: a nested source "
                      f"list did not resolve: {flat} {errors}")
                return 1

            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  demo.demo: other/three.c\n", encoding="utf-8")
            _, errors = product_sources(product_file, index)
            if not any("not a source of" in error for error in errors):
                print("product manifest self-check failed: a source outside "
                      f"its component was accepted: {errors}")
                return 1
            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  demo.other: other/three.c\n", encoding="utf-8")
            _, errors = product_sources(product_file, index)
            if not any("does not select" in error for error in errors):
                print("product manifest self-check failed: sources for an "
                      f"unselected component were accepted: {errors}")
                return 1
            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  demo.demo: [demo/one.c, demo/one.c]\n",
                encoding="utf-8")
            _, errors = product_sources(product_file, index)
            if not any("declared twice" in error for error in errors):
                print("product manifest self-check failed: a duplicate source "
                      f"was accepted: {errors}")
                return 1
            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  - demo/one.c\n", encoding="utf-8")
            try:
                product_sources(product_file, index)
            except SystemExit:
                pass
            else:
                print("product manifest self-check failed: a bare sources "
                      "list was accepted")
                return 1
            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  demo.demo: demo/one.c\n  - demo/two.c\n",
                encoding="utf-8")
            try:
                product_sources(product_file, index)
            except SystemExit:
                pass
            else:
                print("product manifest self-check failed: a list item "
                      "outside a mapping entry was accepted")
                return 1
            product_file.write_text(
                "name: demo\ncomponents: [demo.demo]\n"
                "sources:\n  - demo/one.c\n  demo.demo: demo/two.c\n",
                encoding="utf-8")
            try:
                product_sources(product_file, index)
            except SystemExit:
                pass
            else:
                print("product manifest self-check failed: a mixed list and "
                      "mapping block was accepted")
                return 1
        finally:
            ROOT = saved
    print("product manifest self-check ok: a product without sources resolves "
          "the legacy source list; scalar and bracketed sources narrow their "
          "component; a source outside its component, a source for an "
          "unselected component and a duplicate source are rejected",
          file=sys.stderr)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check-all", action="store_true")
    parser.add_argument("--cmake", metavar="PRODUCT")
    parser.add_argument("--output", metavar="FILE")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if args.self_check:
        return self_check()
    if self_check() != 0:
        return 1

    index = component_index()
    if args.check_all:
        failed = False
        for product_file in product_paths():
            groups, errors = product_sources(product_file, index)
            if errors:
                failed = True
                for error in errors:
                    print(error)
            else:
                print(f"{product_file.parent.name}: {len(groups)} components")
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
