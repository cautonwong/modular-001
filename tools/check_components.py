#!/usr/bin/env python3
"""Check component manifests and component dependency direction.

Each component owns one or more paths and declares its layer. The checker
walks the declared paths, maps every include to a layer, and fails when a
component includes a layer it may not depend on. Declared
`dependencies:` are checked against the same FORBIDDEN table, so a manifest
cannot bypass the include ban. Legacy components are checked for metadata
only.

A relative include is resolved against the including file's directory before
its layer is classified, so a parent-relative path cannot dodge the ban.
System includes and paths outside the repo are skipped.

--self-check builds a throwaway tree and proves the include ban for both a
repo-relative and a parent-relative include, the dependency-direction ban for
protocol and services, and a clean component. The self-check runs on every
invocation, so the CI gate always exercises it.
"""
import argparse
import os
import re
import sys
import tempfile
import xml.etree.ElementTree as ET

import legacy_baseline

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPONENT_ROOTS = ("components", "domain", "application", "infra", "targets")
LAYERS = ("core", "domain", "application", "protocol", "services", "infra", "platform", "product")
LEGACY_LAYER = "legacy"
FORBIDDEN = {
    "core": {"domain", "application", "protocol", "services", "infra", "platform", LEGACY_LAYER, "product"},
    "domain": {"application", "protocol", "services", "infra", "platform", LEGACY_LAYER, "product"},
    "application": {"protocol", "infra", "platform", LEGACY_LAYER, "product"},
    "protocol": {"application", "infra", "platform", LEGACY_LAYER, "product"},
    "services": {"protocol", "application", "infra", "platform", LEGACY_LAYER, "product"},
    "infra": {"product"},
    "platform": {"product"},
    "product": set(),
}
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
SOURCE = re.compile(r"\.(c|h)$")
# One FIFO storage type. A copy reappears as a struct with the same fields.
FIFO_FIELDS = {"buf_ptr", "rd", "wr", "buf_size"}
STRUCT_BODY = re.compile(r"struct\s+\w+\s*\{([^{}]*)\}", re.DOTALL)
STRUCT_MEMBER = re.compile(r"\b(\w+)\s*(?:\[[^\]]*\])?\s*;")


def _manifest_value(text):
    text = text.strip()
    if text.startswith("[") and text.endswith("]"):
        return [item.strip().strip("'\"") for item in text[1:-1].split(",")
                if item.strip()]
    if text.lower() in ("true", "false"):
        return text.lower() == "true"
    return text.strip("'\"")


def parse_manifest(path):
    data = {}
    block = None
    entry = None
    kind = None
    with open(path, encoding="utf-8", errors="replace") as handle:
        for number, raw in enumerate(handle, 1):
            text = raw.split("#", 1)[0].rstrip()
            if not text.strip():
                continue
            line = text.strip()
            if block is not None and text[0].isspace():
                if line.startswith("- "):
                    if kind == "mapping" and entry is None:
                        raise SystemExit(
                            f"{path}:{number}: list item outside a mapping entry")
                    item = line[2:].strip().strip("'\"")
                    if item:
                        if kind == "mapping":
                            data[block][entry].append(item)
                        else:
                            if kind is None:
                                kind = "list"
                            data[block].append(item)
                    continue
                if ":" not in line:
                    continue
                if kind == "list":
                    raise SystemExit(
                        f"{path}:{number}: mapping entry after list items")
                if kind is None:
                    kind = "mapping"
                    data[block] = {}
                key, _, value = line.partition(":")
                key = key.strip()
                value = value.strip()
                data[block][key] = _manifest_value(value) if value else []
                entry = key if not value else None
                continue
            block = None
            entry = None
            kind = None
            if line.startswith("- ") or ":" not in line:
                continue
            key, value = line.split(":", 1)
            key = key.strip()
            value = value.strip()
            if not value:
                data[key] = []
                block = key
            else:
                data[key] = _manifest_value(value)
    return data


def discover(root=ROOT):
    components = []
    for base in COMPONENT_ROOTS:
        root_dir = os.path.join(root, base)
        for dirpath, _dirs, files in os.walk(root_dir):
            if "build" in os.path.relpath(dirpath, root).split(os.sep):
                continue  # generated build trees can contain copied manifests
            if "component.yaml" in files:
                path = os.path.join(dirpath, "component.yaml")
                data = parse_manifest(path)
                data["_file"] = path
                components.append(data)
    return components


def target_layer(include):
    if include.startswith("components/"):
        parts = include.split("/")
        if len(parts) > 2:
            category = parts[1]
            if category in ("core", "domain", "application", "protocol", "services", "platform"):
                return category
        return "infra"
    for layer in ("domain", "application", "infra", "targets", "products"):
        if include.startswith(layer + "/"):
            return "product" if layer in ("targets", "products") else layer
    return None


def include_layer(include, source, component, root=ROOT):
    """Layer of an include, resolving a relative path against the source dir."""
    layer = target_layer(include)
    if layer is not None:
        return layer
    resolved = os.path.normpath(os.path.join(os.path.dirname(source), include))
    if not os.path.isfile(resolved):
        return None
    rel = os.path.relpath(resolved, root)
    if rel == ".." or rel.startswith(".." + os.sep):
        return None
    owned = [os.path.join(root, path) for path in component.get("paths", [])]
    if any(resolved == path or resolved.startswith(path + os.sep) for path in owned):
        return None
    return target_layer(rel.replace(os.sep, "/"))


def ewp_projects(root=ROOT):
    """Include directories and source lists of every IAR project in the tree."""
    projects = []
    for dirpath, dirs, files in os.walk(root):
        dirs[:] = [name for name in dirs if not name.startswith(".")]
        for filename in sorted(files):
            if not filename.endswith(".ewp"):
                continue
            path = os.path.join(dirpath, filename)
            tree = ET.parse(path)
            configuration = legacy_baseline.first_configuration(tree)
            project_dir = os.path.dirname(path)
            includes = []
            for raw in legacy_baseline.collect_option(configuration, "CCIncludePath2"):
                resolved = os.path.normpath(
                    raw.replace("$PROJ_DIR$", project_dir).replace("\\", "/"))
                if os.path.isdir(resolved):
                    includes.append(resolved)
            sources = set()
            for element in tree.getroot().iter("name"):
                text = element.text or ""
                if text.endswith(".c"):
                    sources.add(os.path.normpath(
                        text.replace("$PROJ_DIR$", project_dir).replace("\\", "/")))
            projects.append({"ewp": path, "includes": includes, "sources": sources})
    return sorted(projects, key=lambda project: project["ewp"])


def projects_for_source(path, projects):
    """Projects whose include trail reaches the file, most specific first."""
    best_length = -1
    best = []
    for project in projects:
        if path in project["sources"]:
            return [project]
        for include_dir in project["includes"]:
            if path == include_dir or path.startswith(include_dir + os.sep):
                if len(include_dir) > best_length:
                    best_length = len(include_dir)
                    best = [project]
                elif len(include_dir) == best_length and project not in best:
                    best.append(project)
    return best


def resolve_include(source, include, projects, root=ROOT):
    include = include.replace("\\", "/")
    candidates = [os.path.join(os.path.dirname(source), include)]
    for project in projects_for_source(source, projects):
        candidates.extend(os.path.join(base, include) for base in project["includes"])
    candidates.append(os.path.join(root, include))
    for candidate in candidates:
        candidate = os.path.normpath(candidate)
        if os.path.isfile(candidate):
            return candidate
    return None


def check(root=ROOT):
    errors = []
    components = discover(root)
    projects = ewp_projects(root)
    owned_components = [
        (os.path.join(root, path), component)
        for component in components
        for path in component.get("paths", [])
    ]

    def component_for(path):
        match = None
        for owned, component in owned_components:
            if path == owned or path.startswith(owned + os.sep):
                if match is None or len(owned) > len(match[0]):
                    match = (owned, component)
        return match[1] if match else None

    names = {}
    for component in components:
        rel = os.path.relpath(component["_file"], root)
        for key in ("name", "layer", "paths"):
            if key not in component:
                errors.append(f"{rel}: missing {key}")
        name = component.get("name")
        layer = component.get("layer")
        if name in names:
            errors.append(f"{rel}: duplicate component name {name}")
        names[name] = component
        if layer not in LAYERS:
            errors.append(f"{rel}: unknown layer {layer}")
        for path in component.get("paths", []):
            if not os.path.exists(os.path.join(root, path)):
                errors.append(f"{rel}: path does not exist: {path}")
        for path in component.get("exclude", []):
            if not os.path.exists(os.path.join(root, path)):
                errors.append(f"{rel}: exclude path does not exist: {path}")

        if layer == "product":
            continue

        excludes = [os.path.join(root, path) for path in component.get("exclude", [])]
        for owned in component.get("paths", []):
            owned_root = os.path.join(root, owned)
            for dirpath, _dirs, files in os.walk(owned_root):
                if any(dirpath == ex or dirpath.startswith(ex + os.sep) for ex in excludes):
                    continue
                for filename in files:
                    if not SOURCE.search(filename):
                        continue
                    source = os.path.join(dirpath, filename)
                    if any(source == ex for ex in excludes):
                        continue
                    rel_source = os.path.relpath(source, root)
                    with open(source, encoding="utf-8", errors="replace") as handle:
                        text = handle.read()
                    if "/components/core/fifo/" not in "/" + rel_source.replace(
                            os.sep, "/"):
                        for body in STRUCT_BODY.findall(text):
                            members = set(STRUCT_MEMBER.findall(body))
                            if len(members & FIFO_FIELDS) >= 3:
                                errors.append(
                                    f"{rel_source}: a second FIFO type; use "
                                    "struct fifo_t from "
                                    "components/core/fifo/fifo.h"
                                )
                                break
                    for number, line in enumerate(text.splitlines(), 1):
                            match = INCLUDE.match(line)
                            if not match:
                                continue
                            include = match.group(1)
                            target = include_layer(include, source, component, root)
                            if target is not None and target != layer:
                                if target in FORBIDDEN.get(layer, set()):
                                    errors.append(
                                        f"{rel_source}:{number}: {layer} component "
                                        f"may not include {include} ({target})"
                                    )
                            dependency = None
                            resolved = resolve_include(source, include, projects, root)
                            if resolved is not None:
                                dependency = component_for(resolved)
                            if (dependency is not None
                                    and dependency is not component
                                    and dependency.get("name") not in component.get(
                                        "dependencies", [])):
                                errors.append(
                                    f"{rel_source}:{number}: undeclared component "
                                    f"dependency {dependency.get('name')} "
                                    f"for {include}"
                                )

    for component in components:
        rel = os.path.relpath(component["_file"], root)
        for dependency in component.get("dependencies", []):
            if dependency not in names:
                errors.append(f"{rel}: unknown dependency {dependency}")
            elif names[dependency].get("layer") in FORBIDDEN.get(component.get("layer"), set()):
                errors.append(
                    f"{rel}: {component.get('layer')} component may not depend "
                    f"on {dependency} ({names[dependency].get('layer')})"
                )

    owned_roots = [
        os.path.join(root, path)
        for component in components
        for path in component.get("paths", [])
    ]
    for base in COMPONENT_ROOTS:
        for dirpath, _dirs, files in os.walk(os.path.join(root, base)):
            for filename in files:
                if not SOURCE.search(filename):
                    continue
                source = os.path.join(dirpath, filename)
                if not any(
                    source == owned or source.startswith(owned + os.sep)
                    for owned in owned_roots
                ):
                    rel_source = os.path.relpath(source, root)
                    errors.append(f"{rel_source}: source is not owned by any component")

    # Product descriptors may only select components that exist.
    products_root = os.path.join(root, "products")
    if os.path.isdir(products_root):
        for dirpath, _dirs, files in os.walk(products_root):
            if "product.yaml" not in files:
                continue
            path = os.path.join(dirpath, "product.yaml")
            data = parse_manifest(path)
            rel = os.path.relpath(path, root)
            if "name" not in data:
                errors.append(f"{rel}: missing name")
            selected = data.get("components", [])
            for component in selected:
                if component not in names:
                    errors.append(f"{rel}: unknown component {component}")

            # A product must select the full dependency closure, otherwise a
            # generated build would miss headers or implementations.
            seen = set()
            pending = [name for name in selected if name in names]
            while pending:
                name = pending.pop()
                if name in seen:
                    continue
                seen.add(name)
                for dependency in names[name].get("dependencies", []):
                    if dependency not in names:
                        continue
                    if dependency not in selected:
                        errors.append(
                            f"{rel}: component {name} needs {dependency}, "
                            f"but the product does not select it"
                        )
                    pending.append(dependency)

    return errors


def _write(root, relative, text):
    path = os.path.join(root, relative)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(text)


def _manifest(name, layer, path, dependencies="[]"):
    return (f"name: {name}\nversion: 0.1.0\nlayer: {layer}\n"
            f"paths: [{path}]\ndependencies: {dependencies}\nlegacy: false\n")


def self_check():
    with tempfile.TemporaryDirectory() as tmp:
        _write(tmp, "application/ports/component.yaml",
               _manifest("application.ports", "application", "application/ports"))
        _write(tmp, "application/ports/uart_timeout.h",
               "#ifndef DEMO_FIFO_H\n#define DEMO_FIFO_H\n#endif\n")
        for name, layer, dependencies in (
                ("demo.clean", "protocol", "[demo.protocol]"),
                ("demo.protocol", "protocol", "[]"),
                ("demo.app", "application", "[]"),
                ("demo.protocol_include", "protocol", "[]"),
                ("demo.protocol_relative", "protocol", "[]"),
                ("demo.protocol_dep", "protocol", "[demo.app]"),
                ("demo.services_dep", "services", "[demo.protocol]"),
                ("demo.fifo", "protocol", "[]")):
            short = name.split(".")[1]
            path = f"components/demo/{short}"
            _write(tmp, f"{path}/component.yaml",
                   _manifest(name, layer, path, dependencies))
            _write(tmp, f"{path}/{short}.c", "")
        _write(tmp, "components/demo/protocol_include/protocol_include.c",
               '#include "application/ports/uart_timeout.h"\n')
        _write(tmp, "components/demo/protocol_relative/protocol_relative.c",
               '#include "../../../application/ports/uart_timeout.h"\n')
        _write(tmp, "components/demo/fifo/fifo.h",
               "struct second_fifo {\n    uint8_t *buf_ptr;\n"
               "    volatile uint16_t rd;\n    volatile uint16_t wr;\n"
               "    uint16_t buf_size;\n};\n")

        joined = "\n".join(check(tmp))
        expected = (
            "protocol component may not include application/ports/uart_timeout.h "
            "(application)",
            "protocol component may not include ../../../application/ports/uart_timeout.h "
            "(application)",
            "protocol component may not depend on demo.app (application)",
            "services component may not depend on demo.protocol (protocol)",
            "components/demo/fifo/fifo.h: a second FIFO type",
        )
        for fragment in expected:
            if fragment not in joined:
                print(f"components self-check failed: missing {fragment!r}")
                return False
        if "demo.clean" in joined:
            print("components self-check failed: clean component was rejected")
            return False
    print("components self-check ok: repo-relative and parent-relative include "
          "bans, dependency direction, clean pass")
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if not self_check():
        return 1
    if args.self_check:
        return 0
    errors = check()
    for error in errors:
        print(error)
    if not errors:
        print(f"components ok ({len(discover())} manifests)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
