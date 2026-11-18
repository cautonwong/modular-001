#!/usr/bin/env python3
"""Ratchet mutable file-scope globals in owned sources and headers.

The baseline records every mutable declaration as one line per path, name and
line. The line keeps two same-named globals in one file distinct: removing one
and adding another at a different line fails. Moving a declaration to another
line also fails until --update rewrites the current set.

The scan covers .c and .h files. It counts static declarations, function-local
statics included, and non-static file-scope declarations that start at column 0
and live at brace depth 0. Function-local non-statics are indented or inside
braces, so they do not count. const objects, const pointers, extern
declarations, typedefs, prototypes and function definitions are not counted.
IAR @ placement is dropped before the declarator name is read.

Owned sources are domain, application, components, targets and infra.
infra/third_party, infra/soc/**/vendor and infra/security/crypto are vendor trees,
tests are not shipped, and build/backupfiles directories are generated, so
the scan skips them. Declarations produced by macros and overlapping #if
branches are not parsed. Macro expansion needs per-file preprocessor flags,
so a macro-generated global stays a blind spot.
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "mutable_globals_baseline.txt"
BASES = ("domain", "application", "components", "targets", "infra")
SKIP_DIRS = {"build", "backupfiles", ".git", "__pycache__", "tests"}
VENDOR = {("infra", "third_party"), ("infra", "security", "crypto")}
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
STATIC = re.compile(r"^[ \t]*static\b", re.MULTILINE)
IDENT = re.compile(r"[A-Za-z_]\w*")
QUALIFIERS = {"const", "volatile", "static", "inline", "struct", "union",
              "enum", "register", "extern"}
STATEMENT_KEYWORDS = {"return", "goto", "if", "else", "while", "for",
                      "switch", "case", "default", "do", "break", "continue",
                      "sizeof", "asm", "__asm", "typedef"}

SELF_CHECK = (
    # static declarations, including function-local statics
    ("static int count;", [("count", 1)]),
    ("static uint8_t buffer[16];", [("buffer", 1)]),
    ("static const int *bound;", [("bound", 1)]),
    ("static int *const fixed = &value;", []),
    ("static const int limit = 3;", []),
    ("static const uint8_t table[2] = {1, 2};", []),
    ("static int helper(void) { return 0; }", []),
    ("    static int local;", [("local", 1)]),
    ("static int first, *second;", [("first", 1), ("second", 1)]),
    # two same-named statics in one file stay distinct
    ("void a(void) {\n    static int state;\n}\n"
     "void b(void) {\n    static int state;\n}",
     [("state", 2), ("state", 5)]),
    # non-static file-scope globals
    ("uint8_t *p;", [("p", 1)]),
    ("unsigned char table[4];", [("table", 1)]),
    ("struct foo value;", [("value", 1)]),
    ("__root __no_init volatile uint32_t AppRun @ 0x20000000;",
     [("AppRun", 1)]),
    ("const int limit = 3;", []),
    ("extern int shared;", []),
    ("int helper(void);", []),
    ("int helper(void) { return 0; }", []),
    ("void run(void) {\n    int local;\n}", []),
    ("void run(void) {\nint unindented;\n}", []),
)


def strip_comments(text: str) -> str:
    return COMMENTS.sub(lambda match: "\n" * match.group(0).count("\n"), text)


def statement(text: str, start: int) -> str | None:
    """Return the declaration at start, or None for a function definition."""
    brace = paren = 0
    for position in range(start, len(text)):
        char = text[position]
        if char == "(":
            paren += 1
        elif char == ")":
            paren -= 1
        elif char == "{":
            if brace == 0 and paren == 0 and ")" in text[start:position]:
                return None
            brace += 1
        elif char == "}":
            brace -= 1
        elif char == ";" and brace == 0 and paren == 0:
            return text[start:position]
    return None


def strip_braces(stmt: str) -> str:
    """Drop struct, enum and initializer bodies, keeping the declarators."""
    kept = []
    depth = 0
    for char in stmt:
        if char == "{":
            depth += 1
        elif char == "}":
            depth = max(0, depth - 1)
        elif depth == 0:
            kept.append(char)
    return "".join(kept)


def fragments(stmt: str) -> list[str]:
    """Split a declaration at top-level commas, including initializers."""
    parts: list[str] = []
    current: list[str] = []
    depth = 0
    for char in stmt:
        if char in "([{":
            depth += 1
        elif char in ")]}":
            depth = max(0, depth - 1)
        if char == "," and depth == 0:
            parts.append("".join(current))
            current = []
        else:
            current.append(char)
    parts.append("".join(current))
    return parts


def declared(stmt: str) -> list[tuple[str, str]]:
    """Classify every declarator in a declaration as a mutable pointer,
    array, or object."""
    found: list[tuple[str, str]] = []
    base_const = False
    for index, fragment in enumerate(fragments(strip_braces(stmt))):
        decl = fragment.split("@", 1)[0].split("=", 1)[0]
        if "(" in decl:
            continue
        bare = []
        depth = 0
        for char in decl:
            if char == "[":
                depth += 1
            elif char == "]":
                depth = max(0, depth - 1)
            elif depth == 0:
                bare.append(char)
        flat = "".join(bare)
        tokens = IDENT.findall(flat)
        if any(token in STATEMENT_KEYWORDS for token in tokens):
            continue
        names = [token for token in tokens if token not in QUALIFIERS]
        if not names or (index == 0 and len(names) < 2):
            continue
        name = names[-1]
        position = flat.rfind(name)
        star = flat.rfind("*", 0, position)
        if star != -1:
            if "const" in flat[star + 1:position].split():
                continue
            found.append(("pointer", name))
            if index == 0:
                base_const = "const" in flat[:star].split()
        else:
            if index == 0:
                base_const = "const" in flat[:position].split()
            if base_const or "const" in flat[:position].split():
                continue
            found.append(("array" if "[" in decl else "object", name))
    return found


def depth_zero_lines(text: str) -> list[int]:
    """Return line-start offsets whose brace depth is zero."""
    offsets = []
    depth = 0
    in_string = in_char = False
    escape = False
    line_start = True
    for index, char in enumerate(text):
        if line_start:
            if depth == 0 and not in_string and not in_char:
                offsets.append(index)
            line_start = False
        if in_string or in_char:
            if escape:
                escape = False
            elif char == "\\":
                escape = True
            elif in_string and char == '"':
                in_string = False
            elif in_char and char == "'":
                in_char = False
            elif char == "\n":
                in_string = in_char = escape = False
        else:
            if char == '"':
                in_string = True
            elif char == "'":
                in_char = True
            elif char == "{":
                depth += 1
            elif char == "}":
                depth = max(0, depth - 1)
        if char == "\n":
            line_start = True
    return offsets


def mutable(text: str) -> list[tuple[str, str, int]]:
    text = strip_comments(text)
    found: list[tuple[str, str, int]] = []
    for match in STATIC.finditer(text):
        stmt = statement(text, match.start())
        if stmt is not None:
            line = text.count("\n", 0, match.start()) + 1
            found.extend((kind, name, line) for kind, name in declared(stmt))
    for offset in depth_zero_lines(text):
        if not text[offset].isalpha() and text[offset] != "_":
            continue
        stmt = statement(text, offset)
        if stmt is None or re.match(r"\s*(static|extern|typedef)\b", stmt):
            continue
        line = text.count("\n", 0, offset) + 1
        found.extend((kind, name, line) for kind, name in declared(stmt))
    return found


def names(root: Path = ROOT) -> dict[str, set[tuple[str, int]]]:
    found: dict[str, set[tuple[str, int]]] = {}
    for base in BASES:
        if not (root / base).is_dir():
            continue
        for path in sorted((root / base).rglob("*")):
            if path.suffix not in (".c", ".h"):
                continue
            rel = path.relative_to(root)
            if any(rel.parts[:len(prefix)] == prefix for prefix in VENDOR):
                continue
            if any(part in SKIP_DIRS for part in rel.parts):
                continue
            if rel.parts[:2] == ("infra", "soc") and "vendor" in rel.parts:
                continue
            entries = {(name, line) for _, name, line in mutable(
                path.read_text(encoding="utf-8", errors="replace"))}
            if entries:
                found[rel.as_posix()] = entries
    return found


def parse(lines: list[str]) -> dict[str, set[tuple[str, int]]]:
    baseline: dict[str, set[tuple[str, int]]] = {}
    for line in lines:
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        path, name, number = line.rsplit(None, 2)
        baseline.setdefault(path, set()).add((name, int(number)))
    return baseline


def load(path: Path = BASELINE) -> dict[str, set[tuple[str, int]]]:
    return parse(path.read_text(encoding="utf-8").splitlines())


def render(current: dict[str, set[tuple[str, int]]]) -> str:
    lines = ["# path name line"]
    for path in sorted(current):
        for name, line in sorted(current[path],
                                 key=lambda item: (item[1], item[0])):
            lines.append(f"{path} {name} {line}")
    return "\n".join(lines) + "\n"


def summary(current: dict[str, set[tuple[str, int]]]) -> str:
    total = sum(len(found) for found in current.values())
    headers = sum(len(found) for path, found in current.items()
                  if path.endswith(".h"))
    return f"{total} entries in {len(current)} files, {headers} in headers"


def diff(baseline: dict[str, set[tuple[str, int]]],
         current: dict[str, set[tuple[str, int]]]) -> list[str]:
    failures = []
    for path in sorted(current):
        for name, line in sorted(current[path] - baseline.get(path, set()),
                                 key=lambda item: (item[1], item[0])):
            failures.append(f"{path}:{line}: new mutable global {name!r} "
                            "is not in the baseline")
    for path in sorted(baseline):
        for name, line in sorted(baseline[path] - current.get(path, set()),
                                 key=lambda item: (item[1], item[0])):
            failures.append(f"{path}:{line}: baseline entry {name!r} is stale")
    return failures


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root / "domain").mkdir()
        (root / "domain" / "state.h").write_text(
            "static int counter;\n"
            "static uint8_t buffer[8];\n"
            "static int *cursor;\n"
            "static const int limit = 3;\n"
            "extern int shared;\n"
            "int helper(void);\n"
            "static inline int read(void) { return 0; }\n",
            encoding="utf-8")
        (root / "domain" / "state.c").write_text(
            "static int local_state;\n", encoding="utf-8")
        (root / "tests").mkdir()
        (root / "tests" / "test_state.c").write_text(
            "static int test_state;\n", encoding="utf-8")
        (root / "infra" / "third_party").mkdir(parents=True)
        (root / "infra" / "third_party" / "vendor.h").write_text(
            "static int vendor_state;\n", encoding="utf-8")
        expected = {
            "domain/state.h": {("counter", 1), ("buffer", 2), ("cursor", 3)},
            "domain/state.c": {("local_state", 1)},
        }
        got = names(root)
        if got != expected:
            print(f"mutable globals self-check failed: scan counted {got}, "
                  f"expected {expected}")
            return False
    return True


def self_check() -> bool:
    for text, expected in SELF_CHECK:
        got = [(name, line) for _, name, line in mutable(text)]
        if got != expected:
            print(f"mutable globals self-check failed: {text!r} counted "
                  f"{got}, expected {expected}")
            return False
    probes = (
        ("two same-named globals on different lines pass",
         {"f.c": {("dup", 3), ("dup", 9)}},
         {"f.c": {("dup", 3), ("dup", 9)}}, 0),
        ("removing one same-named global and adding another at a different "
         "line fails",
         {"f.c": {("dup", 3), ("dup", 9)}},
         {"f.c": {("dup", 3), ("dup", 12)}}, 2),
        ("a removed name is stale",
         {"f.c": {("a", 1), ("b", 2)}}, {"f.c": {("a", 1)}}, 1),
        ("a moved line is stale",
         {"f.c": {("g", 3)}}, {"f.c": {("g", 4)}}, 2),
    )
    for label, baseline, current, expected in probes:
        failures = diff(baseline, current)
        if len(failures) != expected:
            print(f"mutable globals self-check failed: {label}, got "
                  f"{failures}, expected {expected} failures")
            return False
    current = {"f.c": {("dup", 3), ("dup", 9)}, "g.h": {("state", 1)}}
    with tempfile.TemporaryDirectory() as temporary:
        baseline = Path(temporary) / "baseline.txt"
        baseline.write_text(render(current), encoding="utf-8")
        if diff(load(baseline), current):
            print("mutable globals self-check failed: --update output does "
                  "not clear")
            return False
    return tree_check()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()

    if not self_check():
        return 1

    current = names()
    if args.update:
        BASELINE.write_text(render(current), encoding="utf-8")
        print(f"mutable globals baseline updated: {summary(current)}")
        return 0

    failures = diff(load(), current)
    for failure in failures:
        print(failure)
    if failures:
        return 1
    print(f"mutable globals ok ({summary(current)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
