#!/usr/bin/env python3
"""Fail when a selected component's public functions have no firmware caller.

For every component selected by a product.yaml, except product-layer target
components, every public C function declared in a header at the component
root or under include/ is a candidate entry point. The component passes when
at least one candidate has a direct call site in a .c file outside the
component's paths and outside tests/, formal/, and targets/host (the host
runner is not firmware). Calls inside the component do not count. A call
credits the component only when a header under the component's paths that
declares that function is visible to the caller's translation unit, directly
or through other includes, so a same-named function in another component
cannot stand in for it. Header-only components and components that declare no
public function are reported and skipped.

tools/reachability_baseline.txt records components whose only current
consumers are tests or the host runner, with one caller path each. The check
fails a component that is not in the baseline and has no firmware caller, and
fails a baseline entry that gained a firmware caller. --update rewrites the
baseline for the current tree.

--self-check builds a throwaway tree, proves a same-name caller without the
component header does not credit it, proves a test caller that includes the
header is accepted only through the baseline, proves a targets/host caller is
not firmware and is baselined with its host path, and proves a firmware
caller that includes the header passes. It also checks the prototype parser
skips keywords, type names, and function-pointer typedefs.
"""
from __future__ import annotations

import argparse
import io
import os
import re
import sys
import tempfile
from contextlib import redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_components

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "reachability_baseline.txt"
IDENT = re.compile(r"[A-Za-z_]\w*")
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
PREPROCESSOR = re.compile(r"(?m)^\s*#.*$")
TYPEDEF = re.compile(r"\btypedef\b")
CALL_PREFIX = re.compile(
    r"[=(),;]|\b(?:return|else|do|case|goto|if|while|for|switch)\b")
SKIP_DIRS = {"build", "backupfiles", ".git", "__pycache__"}
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
        dirs[:] = [name for name in dirs if name not in SKIP_DIRS]
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


def read_sources(root):
    texts = {}
    for dirpath, dirs, files in os.walk(root):
        dirs[:] = [name for name in dirs if name not in SKIP_DIRS]
        for name in files:
            if name.endswith(".c"):
                path = Path(dirpath) / name
                texts[path] = path.read_text(encoding="utf-8", errors="replace")
    return texts


def verification(path, root):
    """Tests, formal harnesses, and the host runner ship no firmware."""
    parts = path.relative_to(root).parts
    return bool({"tests", "formal"} & set(parts)) or parts[:2] == ("targets", "host")


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


def load_baseline(path):
    baseline = {}
    if not path.exists():
        return baseline
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) != 2:
            raise SystemExit(f"{path}: bad baseline line {line!r}")
        baseline[parts[0]] = parts[1]
    return baseline


def targets(root):
    """Resolve manifests and product selections under root."""
    root = Path(root).resolve()
    components = {c["name"]: c for c in check_components.discover(root)}
    selected = {}
    for product in sorted((root / "products").glob("*/product.yaml")):
        data = check_components.parse_manifest(product)
        for name in data.get("components", []):
            selected.setdefault(name, []).append(product.parent.name)
    return root, components, selected


def inspect(root):
    """Return one row per selected, checkable component plus skip messages."""
    root, components, selected = targets(root)
    texts = read_sources(root)
    typedefs = header_typedefs(root)
    projects = check_components.ewp_projects(root)
    declarations = header_declarations(root, components.values(), typedefs)
    visible = {}
    production = {path: text for path, text in texts.items() if not verification(path, root)}
    nonproduction = {path: text for path, text in texts.items() if verification(path, root)}
    rows = []
    skips = []
    for name in sorted(selected):
        component = components.get(name)
        if component is None:
            continue  # check_components reports unknown selections
        products = ", ".join(selected[name])
        owned = [(root / path).resolve() for path in component.get("paths", [])]
        if component.get("layer") == "product":
            skips.append(f"skip {name} (product layer; selected by {products})")
            continue
        if not any(owned_by(path, owned) for path in texts):
            skips.append(f"skip {name} (no C sources; selected by {products})")
            continue
        entries = public_entries(root, component, typedefs)
        if not entries:
            skips.append(f"skip {name} (no public function; selected by {products})")
            continue
        owned_headers = {header for header in declarations if owned_by(header, owned)}

        def credit(path, symbol, owned_headers=owned_headers):
            return any(symbol in declarations[header]
                       for header in visible_headers(path, projects, root, visible)
                       if header in owned_headers)

        pattern = name_pattern(entries)
        rows.append((name, products, entries, owned,
                     first_call(production, owned, pattern, typedefs, credit),
                     first_call(nonproduction, owned, pattern, typedefs, credit)))
    return root, rows, skips


def check(root, baseline):
    root, rows, skips = inspect(root)
    errors = []
    checked = baselined = 0
    seen = set()
    for line in skips:
        print(line)
    for name, products, entries, _owned, production, nonproduction in rows:
        seen.add(name)
        if production:
            if name in baseline:
                errors.append(
                    f"{name}: baseline entry is stale; firmware caller "
                    f"{production[1].relative_to(root)}")
            else:
                print(f"ok {name}: {production[0]} -> {production[1].relative_to(root)}")
                checked += 1
            continue
        if name in baseline:
            recorded = baseline[name]
            actual = (nonproduction[1].relative_to(root).as_posix()
                      if nonproduction else None)
            if actual == recorded:
                print(f"ok {name} (non-production): {nonproduction[0]} -> {actual}")
                baselined += 1
            else:
                errors.append(
                    f"{name}: baseline caller {recorded} no longer calls "
                    f"{', '.join(entries)}; run --update")
            continue
        where = (f"; non-production caller {nonproduction[1].relative_to(root)}"
                 if nonproduction else "")
        errors.append(
            f"{name}: selected by {products}, but no firmware call site outside "
            f"{[os.path.relpath(path, root) for path in _owned]} for "
            f"{', '.join(entries)}{where}")

    for name in sorted(set(baseline) - seen):
        errors.append(f"{name}: baseline entry is stale; component is not checked")

    for error in errors:
        print(error)
    print(f"component reachability: {checked} checked, {baselined} baselined, "
          f"{len(skips)} skipped")
    return errors


def update(root, path):
    root, rows, _skips = inspect(root)
    baseline = {}
    dead = []
    for name, products, entries, _owned, production, nonproduction in rows:
        if production:
            continue
        if nonproduction:
            baseline[name] = nonproduction[1].relative_to(root).as_posix()
        else:
            dead.append(
                f"{name}: selected by {products}, but no firmware, test, or "
                f"host call site for {', '.join(entries)}")
    lines = ["# component non-production caller",
             *[f"{name} {caller}" for name, caller in sorted(baseline.items())]]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"reachability baseline updated: {len(baseline)} non-production-only "
          f"components")
    for line in dead:
        print(line)
    return 1 if dead else 0


def self_check():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "components/demo/dead/include").mkdir(parents=True)
        (root / "products/demo").mkdir(parents=True)
        (root / "components/demo/dead/component.yaml").write_text(
            "name: demo.dead\n"
            "version: 0.1.0\n"
            "layer: protocol\n"
            "paths: [components/demo/dead]\n"
            "dependencies: []\n"
            "legacy: false\n", encoding="utf-8")
        header = ("typedef int (*demo_handler_t)(void *ctx);\n"
                  "typedef struct demo_ops {\n"
                  "    void (*release)(void *ctx);\n"
                  "} demo_ops_t;\n"
                  "void *nd_dead_create(void);\n"
                  "int nd_dead_publish(void);\n")
        (root / "components/demo/dead/include/nd_dead.h").write_text(
            header, encoding="utf-8")
        (root / "components/demo/dead/nd_dead.c").write_text(
            '#include "include/nd_dead.h"\n'
            "void *nd_dead_create(void) { return 0; }\n"
            "int nd_dead_publish(void) { return 1; }\n", encoding="utf-8")
        (root / "products/demo/product.yaml").write_text(
            "name: demo\ncomponents: [demo.dead]\n", encoding="utf-8")
        (root / "infra").mkdir()
        (root / "infra/impl.c").write_text(
            "static int nd_dead_publish(void) { return 1; }\n"
            "void *nd_dead_create(void) { return 0; }\n"
            "int infra_own(void) { return nd_dead_publish() != 0; }\n",
            encoding="utf-8")

        parsed = public_functions(header, typedef_names(header))
        if sorted(set(parsed)) != ["nd_dead_create", "nd_dead_publish"]:
            print(f"self-check failed: parsed public functions {parsed}")
            return 1

        with redirect_stdout(io.StringIO()):
            errors = check(root, {})
        if not errors or "no firmware call site" not in errors[0]:
            print("self-check failed: same-name caller without the component "
                  "header credited the component")
            return 1

        (root / "tests/unit").mkdir(parents=True)
        (root / "tests/unit/test_dead.c").write_text(
            '#include "components/demo/dead/include/nd_dead.h"\n'
            "int test_dead(void) { return nd_dead_publish() != 0; }\n",
            encoding="utf-8")
        with redirect_stdout(io.StringIO()):
            errors = check(root, {})
        if not errors or "non-production caller tests/unit/test_dead.c" not in errors[0]:
            print("self-check failed: test caller that includes the component "
                  "header was not rejected")
            return 1
        baseline_file = root / "tools/reachability_baseline.txt"
        with redirect_stdout(io.StringIO()):
            update(root, baseline_file)
        baseline = load_baseline(baseline_file)
        if baseline != {"demo.dead": "tests/unit/test_dead.c"}:
            print(f"self-check failed: baseline update wrote {baseline}")
            return 1
        with redirect_stdout(io.StringIO()):
            errors = check(root, baseline)
        if errors:
            print("self-check failed: baselined test-only component was rejected")
            return 1

        (root / "targets/host").mkdir(parents=True)
        (root / "targets/host/host_main.c").write_text(
            '#include "components/demo/dead/include/nd_dead.h"\n'
            "int host_main(void) { return nd_dead_publish() != 0; }\n",
            encoding="utf-8")
        with redirect_stdout(io.StringIO()):
            errors = check(root, {})
        if (not errors
                or "non-production caller targets/host/host_main.c" not in errors[0]):
            print("self-check failed: host-only caller counted as firmware")
            return 1
        with redirect_stdout(io.StringIO()):
            update(root, baseline_file)
        baseline = load_baseline(baseline_file)
        if baseline != {"demo.dead": "targets/host/host_main.c"}:
            print(f"self-check failed: host path was not baselined: {baseline}")
            return 1
        with redirect_stdout(io.StringIO()):
            errors = check(root, baseline)
        if errors:
            print("self-check failed: baselined host-only component was rejected")
            return 1

        (root / "targets/demo").mkdir(parents=True)
        (root / "targets/demo/main.c").write_text(
            '#include "components/demo/dead/include/nd_dead.h"\n'
            "int main(void) { return nd_dead_publish() != 0; }\n",
            encoding="utf-8")
        with redirect_stdout(io.StringIO()):
            errors = check(root, baseline)
        if not errors or "baseline entry is stale" not in errors[0]:
            print("self-check failed: stale baseline entry was not rejected")
            return 1
        with redirect_stdout(io.StringIO()):
            errors = check(root, {})
        if errors:
            print("self-check failed: firmware caller that includes the "
                  "component header was rejected")
            return 1
        print("self-check ok: same-name caller without the component header "
              "does not count; header-included test and host callers are "
              "non-production; header-included firmware caller passes")
        return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    if self_check() != 0:
        return 1
    if args.update:
        return update(ROOT, BASELINE)
    return 1 if check(ROOT, load_baseline(BASELINE)) else 0


if __name__ == "__main__":
    sys.exit(main())
