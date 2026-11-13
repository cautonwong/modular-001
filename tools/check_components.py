#!/usr/bin/env python3
"""Check component manifests and component include direction.

Each component owns one or more paths and declares its layer. The checker
walks the declared paths, maps every repo-relative include to a layer, and
fails when a component includes a layer it may not depend on. Legacy
components are checked for metadata only.
"""
import os
import re
import sys
import xml.etree.ElementTree as ET

import legacy_baseline

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPONENT_ROOTS = ("components", "domain", "application", "infra", "nidavellir", "targets")
LAYERS = ("core", "domain", "application", "protocol", "services", "infra", "platform", "product")
LEGACY_LAYER = "legacy"
FORBIDDEN = {
    "core": {"domain", "application", "protocol", "services", "infra", "platform", LEGACY_LAYER, "product"},
    "domain": {"application", "protocol", "services", "infra", "platform", LEGACY_LAYER, "product"},
    "application": {"protocol", "infra", "platform", LEGACY_LAYER, "product"},
    "protocol": {"infra", "platform", LEGACY_LAYER, "product"},
    "services": {"infra", "platform", LEGACY_LAYER, "product"},
    "infra": {"product"},
    "platform": {"product"},
    "product": set(),
}
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
SOURCE = re.compile(r"\.(c|h)$")


def parse_manifest(path):
    data = {}
    with open(path, encoding="utf-8", errors="replace") as handle:
        for raw in handle:
            line = raw.split("#", 1)[0].strip()
            if not line or ":" not in line:
                continue
            key, value = line.split(":", 1)
            key = key.strip()
            value = value.strip()
            if value.startswith("[") and value.endswith("]"):
                items = [item.strip().strip("'\"") for item in value[1:-1].split(",")]
                data[key] = [item for item in items if item]
            elif value.lower() in ("true", "false"):
                data[key] = value.lower() == "true"
            else:
                data[key] = value.strip("'\"")
    return data


def discover():
    components = []
    for base in COMPONENT_ROOTS:
        root = os.path.join(ROOT, base)
        for dirpath, _dirs, files in os.walk(root):
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
    if include.startswith(("kernel/", "nidavellir/")):
        return LEGACY_LAYER
    return None


def ewp_projects():
    """Include directories and source lists of every IAR project in the tree."""
    projects = []
    for dirpath, dirs, files in os.walk(ROOT):
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


def resolve_include(source, include, projects):
    include = include.replace("\\", "/")
    candidates = [os.path.join(os.path.dirname(source), include)]
    for project in projects_for_source(source, projects):
        candidates.extend(os.path.join(base, include) for base in project["includes"])
    candidates.append(os.path.join(ROOT, include))
    for candidate in candidates:
        candidate = os.path.normpath(candidate)
        if os.path.isfile(candidate):
            return candidate
    return None


def main():
    errors = []
    components = discover()
    projects = ewp_projects()
    owned_components = [
        (os.path.join(ROOT, path), component)
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
        rel = os.path.relpath(component["_file"], ROOT)
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
            if not os.path.exists(os.path.join(ROOT, path)):
                errors.append(f"{rel}: path does not exist: {path}")
        for path in component.get("exclude", []):
            if not os.path.exists(os.path.join(ROOT, path)):
                errors.append(f"{rel}: exclude path does not exist: {path}")

        if layer == "product":
            continue

        excludes = [os.path.join(ROOT, path) for path in component.get("exclude", [])]
        for owned in component.get("paths", []):
            owned_root = os.path.join(ROOT, owned)
            for dirpath, _dirs, files in os.walk(owned_root):
                if any(dirpath == ex or dirpath.startswith(ex + os.sep) for ex in excludes):
                    continue
                for filename in files:
                    if not SOURCE.search(filename):
                        continue
                    source = os.path.join(dirpath, filename)
                    if any(source == ex for ex in excludes):
                        continue
                    rel_source = os.path.relpath(source, ROOT)
                    with open(source, encoding="utf-8", errors="replace") as handle:
                        for number, line in enumerate(handle, 1):
                            match = INCLUDE.match(line)
                            if not match:
                                continue
                            include = match.group(1)
                            target = target_layer(include)
                            if target is not None and target != layer:
                                if target in FORBIDDEN.get(layer, set()):
                                    errors.append(
                                        f"{rel_source}:{number}: {layer} component "
                                        f"may not include {include} ({target})"
                                    )
                            dependency = None
                            resolved = resolve_include(source, include, projects)
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
        rel = os.path.relpath(component["_file"], ROOT)
        for dependency in component.get("dependencies", []):
            if dependency not in names:
                errors.append(f"{rel}: unknown dependency {dependency}")

    owned_roots = [
        os.path.join(ROOT, path)
        for component in components
        for path in component.get("paths", [])
    ]
    for base in COMPONENT_ROOTS:
        for dirpath, _dirs, files in os.walk(os.path.join(ROOT, base)):
            for filename in files:
                if not SOURCE.search(filename):
                    continue
                source = os.path.join(dirpath, filename)
                if not any(
                    source == owned or source.startswith(owned + os.sep)
                    for owned in owned_roots
                ):
                    rel_source = os.path.relpath(source, ROOT)
                    errors.append(f"{rel_source}: source is not owned by any component")

    # Product descriptors may only select components that exist.
    products_root = os.path.join(ROOT, "products")
    if os.path.isdir(products_root):
        for dirpath, _dirs, files in os.walk(products_root):
            if "product.yaml" not in files:
                continue
            path = os.path.join(dirpath, "product.yaml")
            data = parse_manifest(path)
            rel = os.path.relpath(path, ROOT)
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

    for error in errors:
        print(error)
    if not errors:
        print(f"components ok ({len(components)} manifests)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
