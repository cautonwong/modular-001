#!/usr/bin/env python3
"""Ratchet public struct definitions in domain and application headers.

`typedef struct { ... } name_t;` in a public header exposes the field layout
to every consumer, so callers can offsetof or mutate it and the type can never
become an aggregate. The baseline records the current set as one `path name`
line per definition: a new definition fails, and a baseline entry with no
definition behind it also fails until --update rewrites the baseline.

Making an aggregate opaque keeps the typedef name in the header and moves the
body into the .c file, so the definition leaves the scan and its baseline line
goes with it. That is the only way the baseline may shrink or change.

The scan reads header text, not a preprocessed translation unit, so
macro-generated typedefs are a blind spot. Comment bodies and opaque
`typedef struct tag name_t;` declarations are not counted.
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "public_struct_baseline.txt"
BASES = ("domain", "application")
TYPEDEF_STRUCT = re.compile(r"\btypedef\s+struct\b")
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
IDENT = re.compile(r"[A-Za-z_]\w*")


def strip_comments(text: str) -> str:
    return COMMENTS.sub(lambda match: "\n" * match.group(0).count("\n"), text)


def closing_brace(text: str, start: int) -> int | None:
    depth = 0
    for position in range(start, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return position
    return None


def definitions(text: str) -> list[str]:
    text = strip_comments(text)
    found = []
    for match in TYPEDEF_STRUCT.finditer(text):
        open_brace = text.find("{", match.end())
        semicolon = text.find(";", match.end())
        if open_brace < 0 or 0 <= semicolon < open_brace:
            continue
        end = closing_brace(text, open_brace)
        if end is None:
            continue
        name = IDENT.search(text[end + 1:text.find(";", end)])
        if name is not None:
            found.append(name.group(0))
    return found


def scan(root: Path = ROOT) -> dict[str, set[str]]:
    found: dict[str, set[str]] = {}
    for base in BASES:
        for path in sorted((root / base).rglob("*.h")):
            rel = path.relative_to(root).as_posix()
            for name in definitions(
                    path.read_text(encoding="utf-8", errors="replace")):
                found.setdefault(rel, set()).add(name)
    return found


def parse(lines: list[str]) -> dict[str, set[str]]:
    baseline: dict[str, set[str]] = {}
    for line in lines:
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        path, name = line.rsplit(None, 1)
        baseline.setdefault(path, set()).add(name)
    return baseline


def load(path: Path = BASELINE) -> dict[str, set[str]]:
    return parse(path.read_text(encoding="utf-8").splitlines())


def render(current: dict[str, set[str]]) -> str:
    lines = ["# path name"]
    for path in sorted(current):
        for name in sorted(current[path]):
            lines.append(f"{path} {name}")
    return "\n".join(lines) + "\n"


def summary(current: dict[str, set[str]]) -> str:
    total = sum(len(found) for found in current.values())
    return f"{total} entries in {len(current)} files"


def diff(baseline: dict[str, set[str]],
         current: dict[str, set[str]]) -> list[str]:
    failures = []
    for path in sorted(current):
        for name in sorted(current[path] - baseline.get(path, set())):
            failures.append(f"{path}: new public struct {name!r}; declare it "
                            "opaque, or --update if it is a DTO")
    for path in sorted(baseline):
        for name in sorted(baseline[path] - current.get(path, set())):
            failures.append(f"{path}: baseline entry {name!r} has no struct "
                            "definition; run --update")
    return failures


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root / "domain").mkdir()
        (root / "application" / "ports").mkdir(parents=True)
        header = root / "domain" / "thing.h"
        header.write_text(
            "typedef struct {\n"
            "    int value;\n"
            "} nd_thing_t;\n"
            "/* typedef struct { int hidden; } nd_commented_t; */\n"
            "typedef struct nd_opaque nd_opaque_t;\n",
            encoding="utf-8")
        (root / "application" / "ports" / "store.h").write_text(
            "typedef struct nd_store { void *ctx; } nd_store_t;\n",
            encoding="utf-8")
        baseline = root / "baseline.txt"
        first = scan(root)
        expected = {
            "domain/thing.h": {"nd_thing_t"},
            "application/ports/store.h": {"nd_store_t"},
        }
        if first != expected:
            print(f"public structs self-check failed: scan found {first}, "
                  f"expected {expected}")
            return False
        baseline.write_text(render(first), encoding="utf-8")
        if diff(load(baseline), first):
            print("public structs self-check failed: --update output does "
                  "not clear")
            return False
        header.write_text(header.read_text(encoding="utf-8")
                          + "typedef struct { int probe; } nd_probe_t;\n",
                          encoding="utf-8")
        added = diff(load(baseline), scan(root))
        if len(added) != 1 or "nd_probe_t" not in added[0]:
            print(f"public structs self-check failed: new struct reported "
                  f"{added}")
            return False
        header.write_text("typedef struct nd_thing nd_thing_t;\n",
                          encoding="utf-8")
        stale = diff(load(baseline), scan(root))
        if len(stale) != 1 or "nd_thing_t" not in stale[0]:
            print(f"public structs self-check failed: removed struct reported "
                  f"{stale}")
            return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if not tree_check():
        return 1
    if args.self_check:
        print("public structs self-check ok")
        return 0

    current = scan()
    if args.update:
        BASELINE.write_text(render(current), encoding="utf-8")
        print(f"public structs baseline updated: {summary(current)}")
        return 0

    failures = diff(load(), current)
    for failure in failures:
        print(failure)
    if failures:
        return 1
    print(f"public structs ok ({summary(current)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
