#!/usr/bin/env python3
"""Enforce the component standard in docs/guides/component-standard.md.

Every manifest needs a semver, a known layer and existing paths. The name is
derived from the directory, except for the documented special roots (boards,
SoCs, third-party libraries and the two platform adapters). New components therefore cannot invent a name that does
not match their location.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import product_manifest

ROOT = Path(__file__).resolve().parent.parent
LAYERS = {"core", "domain", "application", "protocol", "services", "infra",
          "platform", "product"}
VERSION = re.compile(r"^\d+\.\d+\.\d+$")
NAME = re.compile(r"^[a-z][a-z0-9_]*(\.[a-z0-9_]+)+$")
# Dissolved 2026-11-18: FIFO to core.fifo, maths to core.types, timer to
# infra/soc. The path must stay gone so a new bucket cannot reappear.
BANNED_PATHS = ("infra/kernel",)


def special_name(parts: list[str]) -> str | None:
    if parts[:2] == ["infra", "soc"] and len(parts) == 4:
        return f"soc.{parts[2]}_{parts[3]}"
    if parts[:2] == ["infra", "third_party"] and len(parts) == 4:
        return f"third_party.{parts[2]}_{parts[3]}"
    if parts[:2] == ["infra", "drivers"] and len(parts) == 4:
        return f"driver.{parts[2]}_{parts[3]}"
    if parts[:3] == ["targets", "stategrid_meter", "board"]:
        return "platform.stategrid_board"
    if parts[:3] == ["targets", "stategrid_meter", "upgrade"]:
        return "platform.stategrid_upgrade"
    return None


def expected_name(path: str) -> str | None:
    parts = path.replace("\\", "/").split("/")
    special = special_name(parts)
    if special is not None:
        return special
    if parts[0] == "components" and len(parts) >= 3:
        return ".".join(parts[1:])
    if parts[0] == "targets" and len(parts) >= 2:
        rest = [part for part in parts[1:] if part != "app"]
        return "target." + ".".join(part.replace(" ", "_").replace("-", "_")
                                    for part in rest).lower()
    if parts[0] in ("domain", "application", "infra"):
        return ".".join(part.replace(" ", "_") for part in parts).lower()
    return None


def allowed_layers(path: str) -> set[str]:
    parts = path.split("/")
    if parts[0] == "domain":
        return {"domain"}
    if parts[0] == "application":
        return {"application", "protocol"}
    if parts[0] == "components" and len(parts) > 1:
        return {parts[1]}
    if parts[0] == "infra":
        if len(parts) >= 3 and parts[1] in ("boards", "soc", "third_party", "drivers"):
            return {"platform"}
        return {"infra"}
    if parts[0] == "targets":
        return {"product", "platform"}
    return set()


def main() -> int:
    errors: list[str] = []
    index = product_manifest.component_index()
    for banned in BANNED_PATHS:
        if (ROOT / banned).exists():
            errors.append(f"{banned}: dissolved into core.fifo, core.types and "
                          "soc.v85xxp_v8500; do not recreate it")
    for name, component in sorted(index.items()):
        path = Path(component["_file"]).parent.relative_to(ROOT).as_posix()
        manifest = Path(component["_file"]).relative_to(ROOT)
        version = str(component.get("version", ""))
        if not VERSION.fullmatch(version):
            errors.append(f"{manifest}: version {version!r} is not semver")
        if component.get("layer") not in LAYERS:
            errors.append(f"{manifest}: unknown layer {component.get('layer')!r}")
        elif component["layer"] not in allowed_layers(path):
            errors.append(f"{manifest}: layer {component['layer']!r} does not "
                          f"match {path}")
        if not NAME.fullmatch(name):
            errors.append(f"{manifest}: name {name!r} is not dotted snake_case")
        expected = expected_name(path)
        if expected is not None and name != expected:
            errors.append(f"{manifest}: name {name!r} should be {expected!r} "
                          f"for {path}")
        if not component.get("paths"):
            errors.append(f"{manifest}: paths is empty")
        for owned in component.get("paths", []):
            if not (ROOT / owned).exists():
                errors.append(f"{manifest}: path {owned!r} does not exist")
        dependencies = component.get("dependencies", [])
        if len(dependencies) != len(set(dependencies)):
            errors.append(f"{manifest}: duplicate dependency")
        for dependency in dependencies:
            if dependency not in index:
                errors.append(f"{manifest}: unknown dependency {dependency!r}")
    for error in errors:
        print(error)
    if errors:
        return 1
    print(f"component standard ok ({len(index)} components)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
