#!/usr/bin/env python3
"""Verify the Lean build produced every module and the #check pins match.

`lake build` succeeds when a module is dropped from the build (nothing to
compile) and when a theorem is added without a pin; a rename only fails if
the pin list is kept. This checker requires one .olean per source module in
the lake build directory (`lib/lean/BladeFormal` or `lib/BladeFormal`,
depending on the Lake version) and cross-checks every theorem declaration
against the pinned #check list in BladeFormal/Theorems.lean.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent
THEOREM = re.compile(r"^theorem\s+([A-Za-z_][\w.]*)", re.MULTILINE)
CHECK = re.compile(r"^#check\s+([A-Za-z_][\w.]*)\s*$", re.MULTILINE)


def build_dir(lean: Path) -> Path:
    for relative in (".lake/build/lib/lean/BladeFormal",
                     ".lake/build/lib/BladeFormal"):
        candidate = lean / relative
        if candidate.is_dir():
            return candidate
    return lean / ".lake/build/lib/lean/BladeFormal"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT)
    root = parser.parse_args().root.resolve()
    lean = root / "formal" / "lean"
    modules = lean / "BladeFormal"
    pins = modules / "Theorems.lean"
    build = build_dir(lean)

    errors: list[str] = []
    sources = sorted(modules.glob("*.lean"))
    if not sources:
        errors.append(f"no Lean modules under {modules.relative_to(root)}")
    for source in sources:
        olean = build / f"{source.stem}.olean"
        if not olean.is_file():
            errors.append(f"missing build artifact {olean.relative_to(root)}; "
                          "run lake build")
    if not pins.is_file():
        errors.append(f"missing pin file {pins.relative_to(root)}")
        pinned: list[str] = []
    else:
        pinned = CHECK.findall(pins.read_text(encoding="utf-8"))
    theorems: list[str] = []
    for source in sources:
        if source.name == "Theorems.lean":
            continue
        theorems.extend(THEOREM.findall(source.read_text(encoding="utf-8")))
    unpinned = sorted(set(theorems) - set(pinned))
    unknown = sorted(set(pinned) - set(theorems))
    if unpinned or unknown:
        errors.append(f"#check pins out of step: unpinned={unpinned} "
                      f"unknown={unknown}")
    if errors:
        print("\n".join(errors))
        print("lean manifest: FAILED")
        return 1
    print(f"lean manifest ok ({len(sources)} modules, {len(theorems)} theorems)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
