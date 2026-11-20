#!/usr/bin/env python3
"""Prove every selected component source reaches a product build.

Firmware products are derived from targets/, not from a hand-written list.
A target with an app/build CMakeLists is firmware and names its product with
set(PRODUCT ...). A target with an "iar for arm" .ewp is firmware and maps to
the products whose selected component paths live under it. A firmware target
with no product fails, as does a non_firmware entry for a target-backed
product. Every product that is neither is rejected.

Firmware products: every .c under a selected component's paths, after the
manifest exclude and tests/ rules, must be an input to that product's link.
The stategrid evidence is the set of translation units compiled and passed to
the GNU ld command by tools/check-firmware-link.py, read back from the
manifest it writes after a successful link. This is link input, not symbol
survival: -Wl,--gc-sections may discard every section of an input source.
The two IDIS products use the executable link statement in the generated
build.ninja. A source compiled only in tests, used only through its header,
or not used by the product is an explicit source exception in
tools/product_link_baseline.txt. Each exception line names a check= that
proves its stated premise: a linked source reaches the includer, the
includer includes a repo-relative header owned by the exception component,
the caller component is unselected and outside the closure, the
test build compiles the source, or the component is in manifest closure. A
premise that stops holding fails the gate. A component with no .c is
header-only.

A non-firmware product is recorded with a non_firmware reason, and every .c
of its selected components must resolve into the product manifest source
list. The non-firmware gas and water products select application.metering
but build no image.

A missing build, a link object with no compile rule, a newly unlinked source,
a stale exception, a false exception premise, and an unregistered product all
fail. --self-check proves the per-source rule and the target derivation in
memory and runs on every invocation.
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
    sources = {}
    non_firmware = {}
    if not path.exists():
        return sources, non_firmware
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        kind, rest = line.split(None, 1)
        try:
            if kind == "source":
                product, component, source, check, reason = rest.split(None, 4)
                if not check.startswith("check="):
                    raise ValueError
                sources[(product, component,
                         os.path.normpath(str(ROOT / source)))] = (
                             reason, check[len("check="):])
            elif kind == "non_firmware":
                product, reason = rest.split(None, 1)
                non_firmware[product] = reason
            else:
                raise ValueError
        except ValueError:
            raise SystemExit(f"{path}: bad baseline line {raw!r}") from None
    return sources, non_firmware


def show(path):
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def component_coverage(product, selected, index, linked, source_exceptions):
    """Return (errors, linked components, header-only components)."""
    errors = []
    linked_components = 0
    header_only = 0
    for name in selected:
        component = index.get(name)
        if component is None:
            errors.append(f"{product}: unknown component {name}")
            continue
        sources = product_manifest.component_sources(component)
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
            key = (product, name, os.path.normpath(str(source)))
            if key in source_exceptions:
                continue
            errors.append(
                f"{product}: {name} source {show(source)} is not linked; "
                f"link it or add a source exception with a reason to {BASELINE.name}")
    return errors, linked_components, header_only


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


def dependency_closure(index, name):
    seen = set()
    stack = [name]
    while stack:
        for dependency in index.get(stack.pop(), {}).get("dependencies", []):
            if dependency not in seen:
                seen.add(dependency)
                stack.append(dependency)
    return seen


def owns_path(component, path):
    target = os.path.normpath(str(ROOT / path))
    for owned in component.get("paths", []):
        root = os.path.normpath(str(ROOT / owned))
        if target == root or target.startswith(root + os.sep):
            return True
    return False


def include_reaches(sources, includer, index):
    """Follow #include edges from the linked sources until the includer."""
    target = os.path.normpath(str(ROOT / includer))
    roots = [ROOT] + [ROOT / owned for entry in index.values()
                      for owned in entry.get("paths", [])]
    seen = set()
    stack = [Path(source) for source in sources]
    while stack:
        current = stack.pop()
        key = os.path.normpath(str(current))
        if key in seen:
            continue
        seen.add(key)
        try:
            text = current.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for name in re.findall(r'#\s*include\s*[<"]([^">]+)[>"]', text):
            for candidate in [current.parent / name] + [root / name for root in roots]:
                if os.path.normpath(str(candidate)) == target:
                    return True
                if candidate.suffix in (".h", ".hpp") and candidate.is_file():
                    stack.append(candidate)
    return False


def check_precondition(spec, product, selected, closure, component, source, index,
                       linked):
    """Prove the premise an exception reason states; return an error or None."""
    kind, _, argument = spec.partition("=")
    if kind == "header":
        includer, _, header = argument.partition(":")
        path = ROOT / includer
        pattern = r'#\s*include\s*[<"]' + re.escape(header) + r'[>"]'
        text = path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""
        if not re.search(pattern, text):
            return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} check fails: "
                    f"{includer} does not include {header}")
        if not include_reaches(linked, includer, index):
            return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} check fails: "
                    f"no linked source reaches {includer}")
        if not any(owns_path(entry, header) for entry in index.values()):
            return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} check fails: "
                    f"header {header} is not under any component; use its "
                    f"repository-relative path")
        if not owns_path(index.get(component, {}), header):
            return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} check fails: "
                    f"{component} does not own {header}")
    elif kind == "unselected":
        if argument not in index:
            return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} check fails: "
                    f"unknown component {argument}")
        if argument in selected or argument in closure:
            how = ("selects" if argument in selected
                   else "reaches through its dependency closure")
            return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} check fails: "
                    f"{product} {how} {argument}")
    elif kind == "test":
        path = ROOT / argument
        text = path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""
        relative = os.path.relpath(source, ROOT)
        lines = [line.split("#", 1)[0] for line in text.splitlines()]
        tokens = [token for line in lines
                  for token in re.split(r"[\s();]+", line) if token]
        if not any(token.endswith(relative) for token in tokens):
            return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} check fails: "
                    f"{argument} does not compile it")
    elif kind == "closure":
        if component not in dependency_closure(index, argument):
            return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} check fails: "
                    f"{argument} does not depend on {component}")
    else:
        return (f"{BASELINE.name}: {os.path.relpath(source, ROOT)} has "
                f"unknown check kind {kind}")
    return None


def check():
    errors = []
    index = product_manifest.component_index()
    source_exceptions, non_firmware = load_baseline(BASELINE)
    firmware, mapping_errors = firmware_products(
        ROOT, product_target_claims(index))
    errors.extend(mapping_errors)
    evidence = {}
    products = {}

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
        evidence[product] = linked
        selected = product_manifest.parse_manifest(product_file).get("components", [])
        product_errors, linked_count, header_only = component_coverage(
            product, selected, index, linked, source_exceptions)
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

    for product, name, source in sorted(source_exceptions):
        key = (product, name, os.path.normpath(source))
        component = index.get(name)
        if product not in evidence or component is None:
            errors.append(f"{BASELINE.name}: {product} {name} names no firmware component")
            continue
        selected = product_manifest.parse_manifest(products[product]).get("components", [])
        if name not in selected:
            errors.append(f"{BASELINE.name}: {product} {name} is not selected; exception is stale")
            continue
        if key[2] not in {os.path.normpath(str(path))
                          for path in product_manifest.component_sources(component)}:
            errors.append(f"{BASELINE.name}: {product} {name} does not own "
                          f"{os.path.relpath(source, ROOT)}")
            continue
        if key[2] in evidence[product]:
            errors.append(f"{BASELINE.name}: {product} {os.path.relpath(source, ROOT)} "
                          f"is linked; exception is stale")
            continue
        closure = product_manifest.closure(products[product], index)[0]
        spec = source_exceptions[key][1]
        error = check_precondition(spec, product, selected, closure, name, key[2],
                                   index, evidence[product])
        if error:
            errors.append(error)

    if errors:
        for error in errors:
            print(error)
        return 1
    print(f"product link ok ({len(firmware)} firmware products, "
          f"{len(source_exceptions)} source exceptions, "
          f"{len(non_firmware)} non-firmware products)")
    return 0


def self_check():
    with tempfile.TemporaryDirectory(prefix="product-link-self-") as tmp:
        root = Path(tmp)
        linked = root / "linked.c"
        unlinked = root / "unlinked.c"
        linked.write_text("", encoding="utf-8")
        unlinked.write_text("", encoding="utf-8")
        index = {"demo.partial": {"name": "demo.partial", "paths": [str(root)]}}
        linked_set = {os.path.normpath(str(linked))}
        with redirect_stdout(io.StringIO()):
            errors, _, _ = component_coverage(
                "demo", ["demo.partial"], index, linked_set, {})
        if len(errors) != 1 or "unlinked.c" not in errors[0]:
            print("product link self-check failed: a partially linked component "
                  f"was not rejected on its unlinked source: {errors}")
            return 1
        exceptions = {
            ("demo", "demo.partial", os.path.normpath(str(unlinked))): "probe"}
        with redirect_stdout(io.StringIO()):
            errors, _, _ = component_coverage(
                "demo", ["demo.partial"], index, linked_set, exceptions)
        if errors:
            print("product link self-check failed: a recorded source exception "
                  f"was rejected: {errors}")
            return 1
        with redirect_stdout(io.StringIO()):
            errors, _, _ = component_coverage(
                "demo", ["demo.partial"], index,
                linked_set | {os.path.normpath(str(unlinked))}, {})
        if errors:
            print(f"product link self-check failed: fully linked sources were "
                  f"rejected: {errors}")
            return 1
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
        (root / "other").mkdir()
        (root / "tests").mkdir()
        includer = root / "demo/inc.h"
        includer.write_text('#include "other/notowned.h"\n', encoding="utf-8")
        source = root / "demo/inc.c"
        source.write_text('#include "inc.h"\n', encoding="utf-8")
        user = root / "other/use.c"
        user.write_text('#include "demo/inc.h"\n', encoding="utf-8")
        lone = root / "demo/lone.c"
        lone.write_text("", encoding="utf-8")
        (root / "tests/CMakeLists.txt").write_text(
            "# ${BLADE_ROOT}/demo/inc.c\n", encoding="utf-8")
        global ROOT
        saved = ROOT
        ROOT = root
        try:
            index = {"demo.partial": {"name": "demo.partial",
                                      "paths": [str(root / "demo")]},
                     "demo.dep": {"name": "demo.dep",
                                  "paths": [str(root / "demo")]}}
            linked = {os.path.normpath(str(source))}
            if not include_reaches({os.path.normpath(str(user))}, "demo/inc.h",
                                   index):
                print("product link self-check failed: a repository-relative "
                      "include root was not searched")
                return 1
            error = check_precondition("header=demo/inc.h:other/notowned.h",
                                       "demo", [], set(), "demo.partial", source,
                                       index, {os.path.normpath(str(lone))})
            if not error or "reaches" not in error:
                print("product link self-check failed: a header premise no linked "
                      f"source reaches was accepted: {error}")
                return 1
            error = check_precondition("unselected=demo.dep", "demo", [],
                                       {"demo.dep"}, "demo.partial", source,
                                       index, linked)
            if not error or "dependency closure" not in error:
                print("product link self-check failed: an unselected component "
                      f"in the closure was accepted: {error}")
                return 1
            error = check_precondition("header=demo/inc.h:other/notowned.h",
                                       "demo", [], set(), "demo.partial", source,
                                       index, linked)
            if not error or "under any component" not in error:
                print("product link self-check failed: a header outside every "
                      f"component was accepted: {error}")
                return 1
            error = check_precondition("test=tests/CMakeLists.txt", "demo", [],
                                       set(), "demo.partial", source, index,
                                       linked)
            if not error or "does not compile" not in error:
                print("product link self-check failed: a commented test entry "
                      f"was accepted: {error}")
                return 1
        finally:
            ROOT = saved
    print("product link self-check ok: a partially linked component fails on its "
          "unlinked source; a recorded source exception passes; firmware products "
          "derive from CMake PRODUCT and IAR component paths; a target with no "
          "product and a product under two targets both fail; a closure-only "
          "unselected premise, a header no linked source reaches, a header "
          "outside every component and a commented test entry are rejected")
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
