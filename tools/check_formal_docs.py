#!/usr/bin/env python3
"""Check that the verification guide lists every formal artifact and pins
the TLA constants it states."""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOC = ROOT / "docs" / "guides" / "verification.md"
ASSIGNMENT = re.compile(r"(?:^|\s)([A-Za-z_]\w*)\s*=\s*(\d+)\s*$")
LISTED = re.compile(
    r"`(formal/(?:tla/[^`]+\.tla|lean/BladeFormal/[^`]+\.lean|cbmc/[^`]+\.c))`")


def section(text: str, needle: str) -> str:
    lines = text.splitlines()
    try:
        start = next(i for i, line in enumerate(lines) if needle in line)
    except StopIteration:
        return ""
    found = [lines[start]]
    for line in lines[start + 1:]:
        if line.startswith("- ") or not line.strip():
            break
        found.append(line)
    return "\n".join(found)


def cfg_constants(path: Path) -> dict[str, int]:
    values: dict[str, int] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = ASSIGNMENT.search(line)
        if match:
            values[match.group(1)] = int(match.group(2))
    return values


def errors_for(text: str, root: Path) -> list[str]:
    errors: list[str] = []

    for path in sorted((root / "formal" / "tla").glob("*.tla")):
        name = f"formal/tla/{path.name}"
        if name not in text:
            errors.append(f"{name} is missing from the verification guide")
    for path in sorted((root / "formal" / "lean" / "BladeFormal").glob("*.lean")):
        if path.name == "Theorems.lean":
            continue
        name = f"formal/lean/BladeFormal/{path.name}"
        if name not in text:
            errors.append(f"{name} is missing from the verification guide")
    for path in sorted((root / "formal" / "cbmc").glob("*.c")):
        name = f"formal/cbmc/{path.name}"
        if name not in text:
            errors.append(f"{name} is missing from the verification guide")

    for name in sorted(set(LISTED.findall(text))):
        if not (root / name).is_file():
            errors.append(
                f"{name} is listed in the verification guide but does not exist")

    energy_cfg = cfg_constants(root / "formal" / "tla" / "EnergyPulse.cfg")
    energy_section = section(text, "EnergyPulse.tla")
    for key in ("Capacity", "Markers", "Granularity", "MaxGen", "MaxPulses"):
        value = energy_cfg.get(key)
        if value is None:
            errors.append(f"EnergyPulse.cfg has no {key} constant")
        elif f"{key}={value}" not in energy_section:
            errors.append(
                f"EnergyPulse section does not state {key}={value} from EnergyPulse.cfg")
    snapshot_cfg = cfg_constants(root / "formal" / "tla" / "Snapshot.cfg")
    snapshot_section = section(text, "Snapshot.tla")
    value = snapshot_cfg.get("MaxGen")
    if value is None:
        errors.append("Snapshot.cfg has no MaxGen constant")
    elif f"MaxGen={value}" not in snapshot_section:
        errors.append(
            f"Snapshot section does not state MaxGen={value} from Snapshot.cfg")

    return errors


def self_check(root: Path) -> list[str]:
    text = (root / "docs" / "guides" / "verification.md").read_text(encoding="utf-8")
    errors: list[str] = []
    if errors_for(text, root):
        errors.append("self-check: clean guide was rejected")
    missing = text + "\n`formal/tla/Missing.tla`\n"
    if not any("Missing.tla" in error and "does not exist" in error
               for error in errors_for(missing, root)):
        errors.append(
            "self-check: a listed but deleted artifact was not caught")
    mutations = (
        ("tla inventory", text.replace("formal/tla/Snapshot.tla",
                                       "formal/tla/Removed.tla", 1),
         "formal/tla/Snapshot.tla"),
        ("energy constant", text.replace("Capacity=2", "Capacity=9", 1),
         "Capacity=2"),
        ("lean inventory",
         text.replace("formal/lean/BladeFormal/Snapshot.lean",
                      "formal/lean/BladeFormal/Removed.lean", 1),
         "formal/lean/BladeFormal/Snapshot.lean"),
    )
    with tempfile.TemporaryDirectory() as temporary:
        for name, variant, expected in mutations:
            if variant == text:
                errors.append(f"self-check: {name} mutation did not change the guide")
                continue
            copy = Path(temporary) / f"verification-{name.replace(' ', '-')}.md"
            copy.write_text(variant, encoding="utf-8")
            found = errors_for(copy.read_text(encoding="utf-8"), root)
            if not any(expected in message for message in found):
                errors.append(f"self-check: {name} mutation was not rejected")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--doc", type=Path, default=None)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    root = args.root.resolve()
    doc = args.doc or (root / "docs" / "guides" / "verification.md")

    errors = errors_for(doc.read_text(encoding="utf-8"), root)
    if args.self_check:
        errors.extend(self_check(root))
    else:
        errors = [error for error in errors if "self-check" not in error]

    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        print("formal docs: FAILED")
        return 1
    tla = len(list((root / "formal" / "tla").glob("*.tla")))
    lean = len([path for path in (root / "formal" / "lean" / "BladeFormal").glob("*.lean")
                if path.name != "Theorems.lean"])
    cbmc = len(list((root / "formal" / "cbmc").glob("*.c")))
    print(f"formal docs ok ({tla} TLA files, {lean} Lean modules, {cbmc} CBMC harnesses)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
