#!/usr/bin/env python3
"""Prove every selected component source reaches a product build.

Firmware products are derived from targets/, not from a hand-written list.
A target with an app/build CMakeLists is firmware and names its product with
set(PRODUCT ...). A target with an "iar for arm" .ewp is firmware and maps to
the products whose selected component paths live under it. A firmware target
with no product fails, as does a non_firmware entry for a target-backed
product. Every product that is neither is rejected.

Firmware products: a selected component's resolved sources must be inputs to
that product's link. A component without a sources: entry resolves to every .c
under its paths, after the manifest exclude and tests/ rules. The reverse also
holds: a source a selected component delivers must be in the product's
resolved source list, and every repository link input must sit under a
component path; vendor trees and paths outside the repository are build inputs
no manifest describes. The stategrid evidence is the set of translation units
compiled and passed to the GNU ld command by tools/check-firmware-link.py,
read back from the manifest it writes after a successful link. This is link
input, not symbol survival: -Wl,--gc-sections may discard every section of an
input source.
The two IDIS products use the executable link statement in the generated
build.ninja. A component with no .c is header-only.

A source of a component some firmware product selects must be selected by a
firmware product or compiled by tests/CMakeLists.txt. The four DLMS sources
are host-test-only through that rule; a removed test-list entry fails.

A non-firmware product is recorded with a non_firmware reason, and every .c
of its selected components must resolve into the product manifest source
list. The non-firmware gas and water products select application.metering
but build no image.

A missing build, a link object with no compile rule, a newly unlinked source,
an undeclared link input, an unaccounted component source and an unregistered
product all fail. --self-check proves the per-source rule, the reverse rule,
the test-list scan and the target derivation in memory and runs on every
invocation.
"""
from __future__ import annotations

import argparse
import io
import os
import re
import subprocess
import sys
import tempfile
from contextlib import redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import product_manifest
from ci_matrix import cmake_dir, target_kind

ROOT = product_manifest.ROOT
BASELINE = ROOT / "tools/product_link_baseline.txt"
FIRMWARE_LINK = ROOT / "tools/check-firmware-link.py"


def logical_lines(text):
    """Join ninja line continuations so one statement is one string."""
    pending = ""
    for raw in text.splitlines():
        if raw.endswith("$"):
            pending += raw[:-1]
            continue
        yield pending + raw
        pending = ""
    if pending:
        yield pending


def ninja_sources(build_dir):
    path = build_dir / "build.ninja"
    compile = {}
    links = []
    for line in logical_lines(path.read_text(encoding="utf-8", errors="replace")):
        match = re.match(r"build (.+?): (C_COMPILER\S*) (.+)", line)
        if match:
            outputs = match.group(1).split("|")[0].split()
            inputs = match.group(3).split("|")[0].split()
            if len(outputs) == 1 and inputs and inputs[0].endswith(".c"):
                compile[outputs[0]] = os.path.normpath(
                    os.path.abspath(build_dir / inputs[0]))
            continue
        match = re.match(r"build (.+?): (C_EXECUTABLE_LINKER\S*) (.+)", line)
        if match:
            links.append(match.group(3).split("|")[0].split())
    if len(links) != 1:
        raise RuntimeError(f"{path}: expected one executable link, found {len(links)}")
    unmapped = [obj for obj in links[0] if obj not in compile]
    if unmapped:
        raise RuntimeError(f"{path}: link object without a compile rule: {unmapped[0]}")
    return {compile[obj] for obj in links[0]}


def ewp_link_sources(product):
    """Link the IAR source list with gcc and return the compiled sources."""
    print(f"stategrid: linking the {product} IAR source list with gcc")
    with tempfile.TemporaryDirectory(prefix="product-link-") as tmp:
        manifest = Path(tmp) / "sources.txt"
        result = subprocess.run(
            [sys.executable, str(FIRMWARE_LINK), "--manifest", str(manifest),
             "--product", product],
            cwd=ROOT, capture_output=True, text=True, errors="replace")
        if result.returncode != 0:
            lines = [line for line in
                     (result.stdout + result.stderr).splitlines() if line.strip()]
            raise RuntimeError("stategrid gcc link failed: "
                               + (lines[-1] if lines
                                  else f"exit {result.returncode}"))
        if not manifest.is_file():
            raise RuntimeError("stategrid gcc link wrote no source manifest")
        sources = [line for line in
                   manifest.read_text(encoding="utf-8").splitlines() if line.strip()]
    if not sources:
        raise RuntimeError("stategrid gcc link manifest is empty")
    return {os.path.normpath(source) for source in sources}


def linked_sources(spec):
    if spec[0] == "ewp-link":
        return ewp_link_sources(spec[1])
    build_dir = ROOT / spec[1]
    if not (build_dir / "build.ninja").is_file():
        raise RuntimeError(f"{build_dir}: build.ninja missing; run the target build first")
    return ninja_sources(build_dir)


def product_target_claims(index):
    """Map product -> absolute component paths, for target ownership."""
    claims = {}
    for product_file in product_manifest.product_paths():
        components, _ = product_manifest.closure(product_file, index)
        paths = []
        for name in components:
            for owned in index[name].get("paths", []):
                paths.append(os.path.normpath(str(ROOT / owned)))
        claims[product_file.parent.name] = paths
    return claims


def firmware_products(root, claims):
    """Derive {product: spec} from the targets/ build descriptors.

    A target with a CMake build is firmware and names its product with
    set(PRODUCT ...). A target with an "iar for arm" project is firmware and
    owns every product that selects a component under its directory. Specs
    are the evidence readers, not a product list.
    """
    firmware = {}
    errors = []
    legacy_targets = []
    for target in sorted(path for path in (root / "targets").iterdir()
                         if path.is_dir()):
        kind = target_kind(target)
        if kind == "cmake":
            cmake = cmake_dir(target)
            text = (cmake / "CMakeLists.txt").read_text(
                encoding="utf-8", errors="replace")
            match = re.search(r"^\s*set\(PRODUCT\s+([^\s)]+)\s*\)", text, re.M)
            if not match:
                errors.append(f"{show(target)}: firmware build sets no PRODUCT")
                continue
            products = [match.group(1)]
            spec = ("ninja", f"build/ci-{target.name}")
        elif kind == "legacy":
            legacy_targets.append(target.name)
            if len(legacy_targets) > 1:
                errors.append(f"{show(target)}: only one IAR firmware target is "
                              f"supported by {FIRMWARE_LINK.name}")
                continue
            prefix = str(target) + os.sep
            products = sorted(name for name, paths in claims.items()
                              if any(path == str(target) or path.startswith(prefix)
                                     for path in paths))
            if not products:
                errors.append(f"{show(target)}: firmware target has no product "
                              f"selecting its sources")
                continue
            spec = ("ewp-link",)
        else:
            continue
        for product in products:
            entry = spec if spec[0] != "ewp-link" else ("ewp-link", product)
            if product in firmware and firmware[product] != entry:
                errors.append(f"{product}: mapped to more than one firmware target")
                continue
            firmware[product] = entry
    return firmware, errors


def load_baseline(path):
    non_firmware = {}
    if not path.exists():
        return non_firmware
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        try:
            kind, rest = line.split(None, 1)
            if kind != "non_firmware":
                raise ValueError
            product, reason = rest.split(None, 1)
            non_firmware[product] = reason
        except ValueError:
            raise SystemExit(f"{path}: bad baseline line {raw!r}") from None
    return non_firmware


def show(path):
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


THIRD_PARTY = "infra/third_party/"


def source_claims(index):
    """Return two source maps: raw path claims and manifest source claims."""
    raw = {}
    delivered = {}
    for name, component in index.items():
        for owned in component.get("paths", []):
            root = ROOT / owned
            if root.is_file():
                candidates = [root]
            elif root.is_dir():
                candidates = sorted(root.rglob("*.c"))
            else:
                candidates = []
            for source in candidates:
                if source.suffix == ".c":
                    raw.setdefault(os.path.normpath(str(source)), set()).add(name)
        for source in product_manifest.component_sources(component):
            delivered.setdefault(os.path.normpath(str(source)), set()).add(name)
    return raw, delivered


def reverse_coverage(product, selected, declared, linked, raw, delivered):
    """Fail when a link input is not declared, the dual of forward coverage.

    A linked source a selected component delivers must appear in the product's
    resolved source list. Every other repository source must at least sit under
    a component path; vendor trees and paths outside the repository are build
    inputs no manifest describes. Manifest excludes stay declared by their
    path: the IDIS targets compile them explicitly.
    """
    errors = []
    for source in sorted(linked):
        relative = os.path.relpath(source, ROOT)
        if relative.startswith(".."):
            continue
        relative = relative.replace(os.sep, "/")
        if relative.startswith(THIRD_PARTY):
            continue
        if source not in raw:
            errors.append(f"{product}: linked source {show(Path(source))} is not "
                          f"declared by any component")
        elif delivered.get(source, set()) & selected and source not in declared:
            errors.append(f"{product}: linked source {show(Path(source))} is not "
                          f"declared for this product by its selected component")
    return errors


def component_coverage(product, groups, linked):
    """Return (errors, linked components, header-only components)."""
    errors = []
    linked_components = 0
    header_only = 0
    for name, sources in groups:
        if not sources:
            header_only += 1
            continue
        hits = [source for source in sources
                if os.path.normpath(str(source)) in linked]
        missing = [source for source in sources
                   if os.path.normpath(str(source)) not in linked]
        if hits:
            linked_components += 1
        if missing:
            print(f"{product}: {name} {len(hits)}/{len(sources)} sources linked")
        for source in missing:
            errors.append(
                f"{product}: {name} source {show(source)} is not linked; "
                f"link it, delete it, or narrow the selection in product.yaml")
    return errors, linked_components, header_only


TEST_LIST = "tests/CMakeLists.txt"


def compiled_in(source, manifest=TEST_LIST):
    """Return True when an uncommented test-list token compiles this source."""
    path = ROOT / manifest
    text = path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""
    relative = os.path.relpath(source, ROOT)
    tokens = [token for line in text.splitlines()
              for token in re.split(r"[\s();]+", line.split("#", 1)[0]) if token]
    return any(token.endswith(relative) for token in tokens)


def accounting_errors(scope, declared, index):
    """Require every source of a firmware-selected component to land.

    A component source that no firmware product selects must be compiled by
    the host test list.
    """
    errors = []
    for name in sorted(scope):
        for source in product_manifest.component_sources(index[name]):
            key = os.path.normpath(str(source))
            if key in declared or compiled_in(key):
                continue
            errors.append(
                f"{name} source {show(source)} is not selected by any firmware "
                f"product and not compiled by {TEST_LIST}")
    return errors


def non_firmware_errors(product_file, index):
    product = product_file.parent.name
    components, errors = product_manifest.closure(product_file, index)
    if errors:
        return errors
    manifest_sources = set()
    for name in components:
        for source in product_manifest.component_sources(index[name]):
            manifest_sources.add(os.path.normpath(str(source)))
    selected = product_manifest.parse_manifest(product_file).get("components", [])
    for name in selected:
        component = index.get(name)
        if component is None:
            errors.append(f"{product}: unknown component {name}")
            continue
        for source in product_manifest.component_sources(component):
            if os.path.normpath(str(source)) not in manifest_sources:
                errors.append(
                    f"{product}: {name} source {show(source)} is not in the "
                    f"product manifest source list")
    print(f"{product}: non-firmware, {len(components)} components, "
          f"{len(manifest_sources)} manifest sources, no link")
    return errors


def check():
    errors = []
    index = product_manifest.component_index()
    raw_claims, delivered_claims = source_claims(index)
    non_firmware = load_baseline(BASELINE)
    firmware, mapping_errors = firmware_products(
        ROOT, product_target_claims(index))
    errors.extend(mapping_errors)
    products = {}
    scope = set()
    declared_any = set()

    for product_file in product_manifest.product_paths():
        product = product_file.parent.name
        products[product] = product_file
        if product not in firmware:
            if product in non_firmware:
                errors.extend(non_firmware_errors(product_file, index))
            else:
                errors.append(
                    f"{product}: no firmware link and no non_firmware reason "
                    f"in {BASELINE.name}")
            continue
        try:
            linked = linked_sources(firmware[product])
        except (OSError, RuntimeError) as error:
            errors.append(str(error))
            continue
        groups, resolver_errors = product_manifest.product_sources(
            product_file, index)
        errors.extend(resolver_errors)
        selected = {name for name, _ in groups}
        declared = {os.path.normpath(str(source))
                    for _, sources in groups for source in sources}
        scope.update(selected)
        declared_any.update(declared)
        errors.extend(reverse_coverage(product, selected, declared, linked,
                                       raw_claims, delivered_claims))
        product_errors, linked_count, header_only = component_coverage(
            product, groups, linked)
        errors.extend(product_errors)
        print(f"{product}: {len(selected)} selected, {linked_count} with linked "
              f"sources, {header_only} header-only")

    for product in sorted(set(firmware) - set(products)):
        errors.append(f"{product}: firmware product.yaml is missing")

    for product in sorted(non_firmware):
        if product not in products:
            errors.append(
                f"{BASELINE.name}: non_firmware {product} has no product.yaml")
        elif product in firmware:
            errors.append(
                f"{BASELINE.name}: non_firmware {product} is a firmware product")

    errors.extend(accounting_errors(scope, declared_any, index))

    if errors:
        for error in errors:
            print(error)
        return 1
    print(f"product link ok ({len(firmware)} firmware products, "
          f"0 source exceptions, {len(non_firmware)} non-firmware products)")
    return 0


def self_check():
    global ROOT
    with tempfile.TemporaryDirectory(prefix="product-link-self-") as tmp:
        root = Path(tmp)
        linked = root / "linked.c"
        unlinked = root / "unlinked.c"
        linked.write_text("", encoding="utf-8")
        unlinked.write_text("", encoding="utf-8")
        groups = [("demo.partial", [linked, unlinked])]
        linked_set = {os.path.normpath(str(linked))}
        with redirect_stdout(io.StringIO()):
            errors, _, _ = component_coverage("demo", groups, linked_set)
        if len(errors) != 1 or "unlinked.c" not in errors[0]:
            print("product link self-check failed: a partially linked component "
                  f"was not rejected on its unlinked source: {errors}")
            return 1
        with redirect_stdout(io.StringIO()):
            errors, _, _ = component_coverage(
                "demo", groups, linked_set | {os.path.normpath(str(unlinked))})
        if errors:
            print(f"product link self-check failed: fully linked sources were "
                  f"rejected: {errors}")
            return 1
    with tempfile.TemporaryDirectory(prefix="product-link-reverse-") as tmp:
        root = Path(tmp)
        for directory in ("selected", "spare"):
            (root / directory).mkdir()
        declared = root / "selected/declared.c"
        sneaked = root / "selected/sneaked.c"
        structural = root / "spare/structural.c"
        stray = root / "stray.c"
        for path in (declared, sneaked, structural, stray):
            path.write_text("", encoding="utf-8")
        (root / "infra/third_party/freertos").mkdir(parents=True)
        vendored = root / "infra/third_party/freertos/tasks.c"
        vendored.write_text("", encoding="utf-8")
        index = {"demo.selected": {"name": "demo.selected",
                                   "paths": ["selected"]},
                 "demo.spare": {"name": "demo.spare", "paths": ["spare"]}}
        saved = ROOT
        saved_manifest_root = product_manifest.ROOT
        ROOT = root
        product_manifest.ROOT = root
        try:
            raw, delivered = source_claims(index)
            declared_set = {os.path.normpath(str(declared))}
            errors = reverse_coverage(
                "demo", {"demo.selected"}, declared_set,
                {os.path.normpath(str(path))
                 for path in (declared, sneaked)}, raw, delivered)
            if len(errors) != 1 or "not declared for this product" not in errors[0]:
                print("product link self-check failed: a source linked from a "
                      f"selected component without declaration was accepted: "
                      f"{errors}")
                return 1
            errors = reverse_coverage(
                "demo", {"demo.selected"}, declared_set,
                {os.path.normpath(str(path))
                 for path in (declared, structural)}, raw, delivered)
            if errors:
                print("product link self-check failed: a source from an "
                      f"unselected component was rejected: {errors}")
                return 1
            errors = reverse_coverage(
                "demo", {"demo.selected"}, declared_set,
                {os.path.normpath(str(path))
                 for path in (declared, vendored)}, raw, delivered)
            if errors:
                print("product link self-check failed: a vendored build input "
                      f"was rejected: {errors}")
                return 1
            errors = reverse_coverage(
                "demo", {"demo.selected"}, declared_set,
                {os.path.normpath(str(path))
                 for path in (declared, stray)}, raw, delivered)
            if len(errors) != 1 or "not declared by any component" not in errors[0]:
                print("product link self-check failed: a link input no "
                      f"component claims was accepted: {errors}")
                return 1
        finally:
            ROOT = saved
            product_manifest.ROOT = saved_manifest_root
    with tempfile.TemporaryDirectory(prefix="product-target-self-") as tmp:
        root = Path(tmp)
        (root / "targets/cmake_target/build").mkdir(parents=True)
        (root / "targets/cmake_target/build/CMakeLists.txt").write_text(
            "set(PRODUCT demo_product)\n", encoding="utf-8")
        (root / "targets/iar_target/iar for arm").mkdir(parents=True)
        (root / "targets/iar_target/iar for arm/demo.ewp").write_text(
            "", encoding="utf-8")
        (root / "targets/host").mkdir()
        claims = {"demo_legacy": [str(root / "targets/iar_target/src/demo.c")]}
        firmware, errors = firmware_products(root, claims)
        expected = {"demo_product": ("ninja", "build/ci-cmake_target"),
                    "demo_legacy": ("ewp-link", "demo_legacy")}
        if errors or firmware != expected:
            print(f"product link self-check failed: firmware targets derived "
                  f"{firmware} with errors {errors}, expected {expected}")
            return 1
        firmware, errors = firmware_products(root, {})
        if not any("no product" in error for error in errors):
            print("product link self-check failed: a firmware target with no "
                  f"product was accepted: {firmware} {errors}")
            return 1
        (root / "targets/cmake_twin/build").mkdir(parents=True)
        (root / "targets/cmake_twin/build/CMakeLists.txt").write_text(
            "set(PRODUCT demo_product)\n", encoding="utf-8")
        _, errors = firmware_products(root, claims)
        if not any("more than one firmware target" in error for error in errors):
            print("product link self-check failed: a product under two firmware "
                  f"targets was accepted: {errors}")
            return 1
    with tempfile.TemporaryDirectory(prefix="product-link-rules-") as tmp:
        root = Path(tmp)
        (root / "demo").mkdir()
        (root / "tests").mkdir()
        source = root / "demo/inc.c"
        source.write_text("", encoding="utf-8")
        test_list = root / "tests/CMakeLists.txt"
        test_list.write_text("# ${BLADE_ROOT}/demo/inc.c\n", encoding="utf-8")
        index = {"demo.partial": {"name": "demo.partial", "paths": ["demo"]}}
        saved = ROOT
        saved_manifest_root = product_manifest.ROOT
        ROOT = root
        product_manifest.ROOT = root
        try:
            key = os.path.normpath(str(source))
            if compiled_in(key):
                print("product link self-check failed: a commented test entry "
                      "was accepted")
                return 1
            errors = accounting_errors({"demo.partial"}, set(), index)
            if len(errors) != 1 or "not selected by any firmware product" not in errors[0]:
                print("product link self-check failed: a component source with "
                      f"no landing was accepted: {errors}")
                return 1
            if accounting_errors({"demo.partial"}, {key}, index):
                print("product link self-check failed: a selected source was "
                      "rejected by the accounting rule")
                return 1
            test_list.write_text("${BLADE_ROOT}/demo/inc.c\n", encoding="utf-8")
            if not compiled_in(key):
                print("product link self-check failed: a test-list entry was "
                      "not scanned")
                return 1
            if accounting_errors({"demo.partial"}, set(), index):
                print("product link self-check failed: a test-compiled source "
                      "was rejected by the accounting rule")
                return 1
        finally:
            ROOT = saved
            product_manifest.ROOT = saved_manifest_root
    print("product link self-check ok: a partially linked component fails on its "
          "unlinked source; firmware products derive from CMake PRODUCT and IAR "
          "component paths; a target with no product and a product under two "
          "targets both fail; a link input a selected component does not declare, "
          "an unclaimed link input, a component source no product selects and a "
          "commented test entry are rejected, while a vendored input and a "
          "test-compiled source pass")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    if self_check() != 0:
        return 1
    return check()


if __name__ == "__main__":
    sys.exit(main())
