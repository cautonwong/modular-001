#!/usr/bin/env python3
"""Require every firmware-selected component to have a caller in its product.

A firmware product is one that maps to a target with a CMake PRODUCT or an
IAR project (see check_product_component_usage.py). Its closure is the
component sources resolved from product.yaml plus every .c under the target
directories that product claims, except tests and build output. A
non-product component's public function must be called from a .c in that
same closure, outside the component's own paths, in a translation unit that
includes an owned header declaring the function. A caller from another
product, a test or the host runner does not credit. There is no baseline: a
selected component without such a caller fails, so the product.yaml list has
to shrink instead of growing an exception.

Non-firmware products (comm_plc, comm_rf, meter_gas, meter_water) are source
assemblies with no firmware link. Their closure has no target source, so
their selected components' callers are not in the assembly by construction.
check_product_component_usage.py proves their sources resolve; caller
reachability is a firmware invariant and is checked only for firmware
products.
"""
from __future__ import annotations

import argparse
import os
import re
import sys
import tempfile
from contextlib import contextmanager
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_components
import check_product_component_usage as usage
import product_manifest

ROOT = Path(__file__).resolve().parents[1]
SKIP_PARTS = {"tests", "build", "backupfiles", ".git", ".claude", "__pycache__"}
IDENT = re.compile(r"[A-Za-z_]\w*")
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
PREPROCESSOR = re.compile(r"(?m)^\s*#.*$")
TYPEDEF = re.compile(r"\btypedef\b")
CALL_PREFIX = re.compile(
    r"[=(),;]|\b(?:return|else|do|case|goto|if|while|for|switch)\b")
KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while", "_Bool", "_Static_assert",
}
TYPEISH = KEYWORDS | {
    "size_t", "ssize_t", "bool", "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t", "uintptr_t",
}


def strip_comments(text):
    return COMMENTS.sub(lambda match: "\n" * match.group(0).count("\n"), text)


def typedef_name(chunk):
    chunk = chunk.strip()
    match = re.search(r"}\s*([A-Za-z_]\w*)\s*$", chunk, re.DOTALL)
    if match:
        return match.group(1)
    match = re.search(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)", chunk)
    if match:
        return match.group(1)
    words = IDENT.findall(chunk)
    return words[-1] if words else None


def typedef_names(text):
    text = strip_comments(text)
    names = set()
    for match in TYPEDEF.finditer(text):
        depth = 0
        for position in range(match.end(), len(text)):
            char = text[position]
            if char in "({[":
                depth += 1
            elif char in ")}]":
                depth = max(0, depth - 1)
            elif char == ";" and depth == 0:
                name = typedef_name(text[match.end():position])
                if name:
                    names.add(name)
                break
    return names


def header_typedefs(root):
    names = set()
    for dirpath, dirs, files in os.walk(root):
        dirs[:] = [name for name in dirs if name not in SKIP_PARTS]
        for name in files:
            if name.endswith(".h"):
                path = Path(dirpath) / name
                names |= typedef_names(
                    path.read_text(encoding="utf-8", errors="replace"))
    return names


def prototype_name(chunk, typedefs):
    if "(" not in chunk:
        return None
    if re.match(r"\s*(typedef|struct|union|enum)\b", chunk):
        return None
    paren = chunk.find("(")
    match = re.search(r"([A-Za-z_]\w*)\s*$", chunk[:paren])
    if not match:
        return None
    name = match.group(1)
    if name in KEYWORDS or name in typedefs:
        return None
    return name


def public_functions(text, typedefs):
    text = strip_comments(text)
    text = PREPROCESSOR.sub(lambda match: "\n" * match.group(0).count("\n"), text)
    found = []
    start = 0
    depth = 0
    for position, char in enumerate(text):
        if char == "{":
            depth += 1
        elif char == "}":
            depth = max(0, depth - 1)
            if depth == 0:
                start = position + 1
        elif char == ";" and depth == 0:
            name = prototype_name(text[start:position], typedefs)
            start = position + 1
            if name:
                found.append(name)
    return found


def public_entries(root, component, typedefs):
    entries = set()
    for owned in component.get("paths", []):
        base = root / owned
        headers = list(base.glob("*.h")) + list((base / "include").rglob("*.h"))
        for header in headers:
            entries.update(public_functions(
                header.read_text(encoding="utf-8", errors="replace"), typedefs))
    return sorted(entries)


def include_paths(path, projects, root):
    """Headers and sources a file includes, resolved like check_components."""
    includes = []
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return includes
    for line in text.splitlines():
        match = check_components.INCLUDE.match(line)
        if not match:
            continue
        resolved = check_components.resolve_include(
            str(path), match.group(1), projects, root)
        if resolved is not None:
            includes.append(Path(resolved))
    return includes


def header_declarations(root, components, typedefs):
    """Public function names declared by every header a component owns."""
    declarations = {}
    for component in components:
        for owned in component.get("paths", []):
            base = root / owned
            headers = list(base.glob("*.h")) + list((base / "include").rglob("*.h"))
            for header in headers:
                declarations[header.resolve()] = set(public_functions(
                    header.read_text(encoding="utf-8", errors="replace"), typedefs))
    return declarations


def visible_headers(path, projects, root, cache):
    """All headers a translation unit includes, directly or transitively."""
    path = path.resolve()
    if path in cache:
        return cache[path]
    seen = {path}
    pending = [path]
    while pending:
        for target in include_paths(pending.pop(), projects, root):
            target = target.resolve()
            if target not in seen:
                seen.add(target)
                pending.append(target)
    cache[path] = seen
    return seen


def definition_at(text, match):
    """True when the match is the entry's own definition, not a consumer."""
    close = text.find(")", match.end())
    if close == -1:
        return False
    i = close + 1
    while i < len(text) and text[i].isspace():
        i += 1
    return i < len(text) and text[i] == "{"


def statement_start(text, position):
    for index in range(position - 1, -1, -1):
        if text[index] in ";{}":
            return index + 1
    return 0


def declaration_at(text, match, typedefs):
    """True when the match is a prototype, not a call."""
    prefix = text[statement_start(text, match.start()):match.start()]
    if not prefix.strip() or CALL_PREFIX.search(prefix):
        return False
    words = IDENT.findall(prefix)
    return bool(words) and (words[-1] in TYPEISH or words[-1] in typedefs)


def name_pattern(names):
    alternatives = "|".join(re.escape(name) for name in
                            sorted(names, key=len, reverse=True))
    return re.compile(r"\b(" + alternatives + r")\s*\(")


def calls(text, pattern, typedefs, credit=None):
    text = strip_comments(text)
    for match in pattern.finditer(text):
        if definition_at(text, match) or declaration_at(text, match, typedefs):
            continue
        if credit is None or credit(match.group(1)):
            return match.group(1)
    return None


def first_call(texts, owned, pattern, typedefs, credit=None):
    for path in sorted(texts):
        if owned_by(path, owned):
            continue
        credited = (lambda name, caller=path: credit(caller, name)) if credit else None
        called = calls(texts[path], pattern, typedefs, credited)
        if called:
            return called, path
    return None


def owned_by(path, owned):
    return any(path == base or base in path.parents for base in owned)


def verification(path, root):
    """Tests, formal harnesses, and the host runner ship no firmware."""
    parts = path.relative_to(root).parts
    return bool({"tests", "formal"} & set(parts)) or parts[:2] == ("targets", "host")


def claimed_targets(product, claims, root):
    """Target directories a product's component paths claim."""
    names = set()
    for claimed in claims.get(product, []):
        parts = Path(claimed).relative_to(root).parts
        if len(parts) > 1 and parts[0] == "targets":
            names.add(parts[1])
    return names


def product_targets(product, spec, claims, root):
    targets = set()
    if spec[0] == "ninja":
        targets.add(Path(spec[1]).name.removeprefix("ci-"))
    targets |= claimed_targets(product, claims, root)
    return {name for name in targets if (root / "targets" / name).is_dir()}


def target_sources(targets, root):
    """Every .c under the given target directories, except tests and output."""
    texts = {}
    for target in targets:
        for path in (root / "targets" / target).rglob("*.c"):
            if SKIP_PARTS.isdisjoint(path.relative_to(root).parts):
                texts[path.resolve()] = path.read_text(
                    encoding="utf-8", errors="replace")
    return texts


def product_closure(product, spec, index, claims, root=ROOT):
    product_file = root / "products" / product / "product.yaml"
    groups, errors = product_manifest.product_sources(product_file, index)
    if errors:
        raise SystemExit(f"{product}: {'; '.join(errors)}")
    closure = {path.resolve() for _component, sources in groups for path in sources}
    closure |= set(target_sources(product_targets(product, spec, claims, root), root))
    return closure


def evaluate(component_name, component, closure, declarations, typedefs,
             projects, root):
    owned = [(root / path).resolve() for path in component.get("paths", [])]
    entries = public_entries(root, component, typedefs)
    if not entries:
        return None
    owned_headers = {header for header in declarations if owned_by(header, owned)}

    def credit(path, symbol):
        if verification(path, root):
            return False
        return any(symbol in declarations[header]
                   for header in visible_headers(path, projects, root, {})
                   if header in owned_headers)

    pattern = name_pattern(entries)
    hit = first_call(closure, owned, pattern, typedefs, credit)
    if hit is None:
        return (f"{component_name}: selected by this product, but no caller in "
                f"the product closure for {', '.join(entries)}")
    return None


def check():
    index = product_manifest.component_index()
    components = {component["name"]: component
                  for component in check_components.discover(ROOT)}
    typedefs = header_typedefs(ROOT)
    projects = check_components.ewp_projects(ROOT)
    declarations = header_declarations(ROOT, components.values(), typedefs)
    claims = usage.product_target_claims(index)
    firmware, errors = usage.firmware_products(ROOT, claims)
    errors = list(errors)
    for product, spec in sorted(firmware.items()):
        closure = product_closure(product, spec, index, claims)
        texts = {path: path.read_text(encoding="utf-8", errors="replace")
                 for path in closure}
        selected, selection_errors = product_manifest.closure(
            ROOT / "products" / product / "product.yaml", index)
        errors.extend(selection_errors)
        checked = 0
        for name in selected:
            component = components.get(name)
            if component is None or component.get("layer") == "product":
                continue
            owned = [(ROOT / path).resolve() for path in component.get("paths", [])]
            if not any(owned_by(path, owned) for path in texts):
                continue
            error = evaluate(name, component, texts, declarations, typedefs,
                             projects, ROOT)
            if error is not None:
                errors.append(f"{product}: {error}")
            else:
                checked += 1
        print(f"ok {product}: {checked} components have a product caller")
    return errors


def tree_check():
    with tempfile_root() as root:
        components = {
            name: {"name": f"demo.{name}", "layer": "domain",
                   "paths": [f"domain/{name}"]}
            for name in ("a", "b", "c", "d")
        }
        for name in ("a", "b", "c", "d"):
            directory = root / "domain" / name
            directory.mkdir(parents=True)
            (directory / f"{name}.h").write_text(
                f"int nd_{name}(void);\n", encoding="utf-8")
        (root / "domain" / "c" / "c.c").write_text(
            '#include "domain/c/c.h"\nint nd_c(void) { return 0; }\n',
            encoding="utf-8")
        (root / "targets" / "one" / "app").mkdir(parents=True)
        (root / "targets" / "one" / "app" / "one.c").write_text(
            '#include "domain/a/a.h"\n'
            "void one(void) { nd_a(); }\n", encoding="utf-8")
        (root / "targets" / "one" / "tests").mkdir(parents=True)
        (root / "targets" / "one" / "tests" / "test_one.c").write_text(
            '#include "domain/d/d.h"\n'
            "void test(void) { nd_d(); }\n", encoding="utf-8")
        (root / "targets" / "host").mkdir(parents=True)
        (root / "targets" / "host" / "host_main.c").write_text(
            '#include "domain/d/d.h"\n'
            "void host(void) { nd_d(); }\n", encoding="utf-8")
        (root / "targets" / "two" / "app").mkdir(parents=True)
        (root / "targets" / "two" / "app" / "two.c").write_text(
            '#include "domain/b/b.h"\n'
            "void two(void) { nd_b(); }\n", encoding="utf-8")

        typedefs = set()
        declarations = {}
        for name in ("a", "b", "c", "d"):
            declarations[(root / "domain" / name / f"{name}.h").resolve()] = {
                f"nd_{name}"}
        closure = target_sources({"one"}, root)
        closure[(root / "domain" / "c" / "c.c").resolve()] = (
            root / "domain" / "c" / "c.c").read_text(encoding="utf-8")
        test_text = (root / "targets" / "one" / "tests" / "test_one.c").read_text(
            encoding="utf-8")
        host_text = (root / "targets" / "host" / "host_main.c").read_text(
            encoding="utf-8")
        closure[(root / "targets" / "one" / "tests" / "test_one.c").resolve()] = test_text
        closure[(root / "targets" / "host" / "host_main.c").resolve()] = host_text
        for path in closure:
            if path.parent.name == "two":
                print("product callers self-check failed: cross-product source "
                      "entered the closure")
                return False

        cases = [
            ("same-product caller passes", "a", False),
            ("cross-product-only caller fails", "b", True),
            ("self-only caller fails", "c", True),
            ("test/host-only caller fails", "d", True),
        ]
        for label, name, expect_failure in cases:
            failure = evaluate(
                f"demo.{name}", components[name], closure, declarations,
                typedefs, [], root) is not None
            if failure != expect_failure:
                print(f"product callers self-check failed: {label}")
                return False
    return True


@contextmanager
def tempfile_root():
    with tempfile.TemporaryDirectory() as temporary:
        yield Path(temporary)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if not tree_check():
        return 1
    errors = check()
    if args.self_check and not errors:
        print("product callers self-check ok (same-product caller passes; "
              "cross-product, self-only and test/host callers fail)")
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    if errors:
        return 1
    print("product callers ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
