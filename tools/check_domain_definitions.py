#!/usr/bin/env python3
"""Require domain header declarations to have a definition in domain.

A function declared in a domain public header is part of the domain's
contract. If its definition lives in a target or application, the domain
header was only a staging point for a product API. Every such name must be
listed in the port allowlist with a reason: the allowlist documents
consumer-implemented ports. An allowlisted name that gains a domain
definition fails until the line is removed.
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "domain"
ALLOWLIST = ROOT / "tools" / "domain_definitions_allowlist.txt"
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
PROTOTYPE = re.compile(r"\b([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*;")
DEFINITION = re.compile(r"\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{")


def strip_comments(text: str) -> str:
    return COMMENTS.sub(lambda match: "\n" * match.group(0).count("\n"), text)


def brace_depths(text: str) -> list[int]:
    depths = [0] * (len(text) + 1)
    depth = 0
    for index, char in enumerate(text):
        depths[index] = depth
        if char == "{":
            depth += 1
        elif char == "}":
            depth = max(0, depth - 1)
    depths[len(text)] = depth
    return depths


def declarations(root: Path = ROOT) -> dict[str, str]:
    found: dict[str, str] = {}
    for path in sorted((root / "domain").rglob("*.h")):
        text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        depths = brace_depths(text)
        for match in PROTOTYPE.finditer(text):
            if depths[match.start()] != 0:
                continue
            line_start = text.rfind("\n", 0, match.start()) + 1
            statement = text[line_start:match.end()]
            if statement.lstrip().startswith("#") or "typedef" in statement.split("(", 1)[0]:
                continue
            if re.search(r"\(\s*\*", statement):
                continue
            found[match.group(1)] = path.relative_to(root).as_posix()
    return found


def definitions(root: Path = ROOT) -> set[str]:
    found: set[str] = set()
    for path in list((root / "domain").rglob("*.c")) + list((root / "domain").rglob("*.h")):
        text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        found.update(match.group(1) for match in DEFINITION.finditer(text))
    return found


def load_allowlist(path: Path = ALLOWLIST) -> dict[str, str]:
    entries: dict[str, str] = {}
    if not path.is_file():
        return entries
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        parts = line.split(None, 1)
        if len(parts) != 2 or not parts[1].strip():
            raise SystemExit(f"allowlist line {number} has no reason")
        entries[parts[0]] = parts[1].strip()
    return entries


def errors(root: Path = ROOT, allowlist_path: Path = ALLOWLIST) -> list[str]:
    declared = declarations(root)
    defined = definitions(root)
    allowlist = load_allowlist(allowlist_path)
    found: list[str] = []
    for name, path in sorted(declared.items()):
        if name in defined:
            if name in allowlist:
                found.append(f"{name}: allowlisted port now has a domain "
                             "definition; remove its allowlist line")
            continue
        if name not in allowlist:
            found.append(f"{path}: declares {name} but domain defines no "
                         "implementation; move the API to its owner or add "
                         "it to tools/domain_definitions_allowlist.txt")
    for name in sorted(set(allowlist) - set(declared)):
        found.append(f"allowlist entry {name} is stale: no domain header "
                     "declares it")
    return found


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root / "domain").mkdir()
        (root / "domain" / "aggregate.h").write_text(
            "int nd_defined(void);\n"
            "#define ND_MACRO(x) (x + 1)\n"
            "typedef int (*nd_callback_t)(void *ctx);\n", encoding="utf-8")
        (root / "domain" / "aggregate.c").write_text(
            "int nd_defined(void) { return 0; }\n", encoding="utf-8")
        (root / "domain" / "port.h").write_text(
            "int port_impl(void);\n", encoding="utf-8")
        allowlist = root / "allowlist.txt"
        allowlist.write_text("port_impl the target implements this port\n",
                             encoding="utf-8")
        if errors(root, allowlist):
            print(f"domain definitions self-check failed: clean tree rejected "
                  f"{errors(root, allowlist)}")
            return False
        (root / "targets").mkdir()
        (root / "targets" / "impl.c").write_text(
            "int nd_defined(void) { return 0; }\n"
            "int target_impl(void) { return 0; }\n", encoding="utf-8")
        (root / "domain" / "staged.h").write_text(
            "int target_impl(void);\n", encoding="utf-8")
        found = errors(root, allowlist)
        if not any("target_impl" in message for message in found):
            print(f"domain definitions self-check failed: target-defined "
                  f"declaration reported {found}")
            return False
        (root / "domain" / "staged.h").unlink()
        (root / "domain" / "port.c").write_text(
            "int port_impl(void) { return 0; }\n", encoding="utf-8")
        found = errors(root, allowlist)
        if not any("remove its allowlist line" in message for message in found):
            print(f"domain definitions self-check failed: port that moved "
                  f"into domain reported {found}")
            return False
        (root / "domain" / "port.h").unlink()
        found = errors(root, allowlist)
        if not any("stale" in message for message in found):
            print(f"domain definitions self-check failed: stale allowlist "
                  f"reported {found}")
            return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if not tree_check():
        return 1
    found = errors()
    if args.self_check and not found:
        print("domain definitions self-check ok (declared-in-domain passes, "
              "target definition fails, port allowlist passes, stale line "
              "fails)")
    print(f"domain definitions: {len(declarations())} declarations, "
          f"{len(load_allowlist())} allowlisted ports")
    for message in found:
        print(f"error: {message}", file=sys.stderr)
    if found:
        return 1
    print("domain definitions ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
