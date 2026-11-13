#!/usr/bin/env python3
"""Ratchet direct accesses to the legacy mbase_control_block_t struct.

The allowlist is the baseline. Counts may only go down; --update rewrites the
baseline from the tree and is the only mode that writes.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
ALLOWLIST = ROOT / "tools" / "mbase_access_allowlist.txt"
SKIP_DIRS = {".git", "third_party", "docs", "tests", "tools"}
SKIP_FILES = {
    "targets/stategrid_meter/persistence/meter_record.c",
    "targets/stategrid_meter/persistence/meter_record.h",
    # Excluded from the VG_DTZY source table and refers to a field that no
    # longer exists in the legacy struct.
    "targets/stategrid_meter/app/metering/legacy/demand.c",
}

ACCESS_PATTERNS = (
    r"\b(?:mbase_cb|mbase_control_block|mbase_block)\s*->\s*[A-Za-z_]\w*",
    r"\bmb\s*\([^()\n]*\)\s*->\s*[A-Za-z_]\w*",
    r"\binstance\s*->\s*mbase_cb\b",
)
BLOCK_ACCESS = r"\bblock\s*->\s*[A-Za-z_]\w*"
BLOCK_DECL = re.compile(r"\bstruct\s+mbase_control_block_t\s*\*\s*block\b")
LAYOUT_RE = re.compile(
    r"offsetof\s*\(\s*struct\s+mbase_control_block_t\s*,"
    r"|sizeof\s*\(\s*struct\s+mbase_control_block_t\s*\)"
)
ALLOW_LINE = re.compile(r"^(\S+)\s+access=(\d+)\s+layout=(\d+)$")

HEADER = (
    "# mbase access ratchet: direct accesses to the legacy mbase_control_block_t layout.",
    "# Counts may only go down. Delete a line when both counts reach zero.",
    "# Format: <repo-relative path> access=<n> layout=<n>",
)


def source_files():
    for path in sorted(ROOT.rglob("*")):
        if not path.is_file() or path.suffix not in (".c", ".h"):
            continue
        rel = path.relative_to(ROOT)
        if any(part in SKIP_DIRS or part.startswith("build") for part in rel.parts[:-1]):
            continue
        name = rel.as_posix()
        if name in SKIP_FILES:
            continue
        yield name, path


def strip_comments(text):
    """Remove C and C++ comments without touching string literals."""
    out = []
    index = 0
    state = "code"
    while index < len(text):
        char = text[index]
        nxt = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if char == "/" and nxt == "*":
                state = "block"
                index += 2
                out.append("  ")
            elif char == "/" and nxt == "/":
                state = "line"
                index += 2
                out.append("  ")
            elif char == '"':
                state = "string"
                out.append(char)
                index += 1
            elif char == "'":
                state = "char"
                out.append(char)
                index += 1
            else:
                out.append(char)
                index += 1
        elif state == "block":
            if char == "*" and nxt == "/":
                state = "code"
                index += 2
            else:
                out.append("\n" if char == "\n" else " ")
                index += 1
        elif state == "line":
            if char == "\n":
                state = "code"
                out.append("\n")
            index += 1
        elif state == "string":
            out.append(char)
            if char == "\\" and index + 1 < len(text):
                out.append(text[index + 1])
                index += 2
                continue
            if char == '"':
                state = "code"
            index += 1
        else:  # char literal
            out.append(char)
            if char == "\\" and index + 1 < len(text):
                out.append(text[index + 1])
                index += 2
                continue
            if char == "'":
                state = "code"
            index += 1
    return "".join(out)


def count_file(path):
    text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
    patterns = list(ACCESS_PATTERNS)
    if BLOCK_DECL.search(text):
        patterns.append(BLOCK_ACCESS)
    access_re = re.compile("|".join(patterns))
    access = 0
    layout = 0
    for line in text.splitlines():
        access += sum(1 for _ in access_re.finditer(line))
        layout += sum(1 for _ in LAYOUT_RE.finditer(line))
    return access, layout


def scan():
    return {name: count_file(path) for name, path in source_files()}


def read_allowlist():
    allowed = {}
    for number, raw in enumerate(ALLOWLIST.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        match = ALLOW_LINE.match(line)
        if not match:
            raise ValueError(f"{ALLOWLIST.relative_to(ROOT)}:{number}: {raw}")
        allowed[match.group(1)] = (int(match.group(2)), int(match.group(3)))
    return allowed


def totals(counts):
    access = sum(a for a, _ in counts.values())
    layout = sum(l for _, l in counts.values())
    return access, layout


def main(argv):
    mode = argv[1] if len(argv) > 1 else ""
    if len(argv) > 2 or mode not in ("", "--update", "--summary"):
        print("usage: check_mbase_access.py [--update|--summary]", file=sys.stderr)
        return 2

    current = scan()
    nonzero = {name: value for name, value in current.items() if value != (0, 0)}
    total_access, total_layout = totals(current)
    file_count = len(nonzero)

    if mode == "--update":
        lines = list(HEADER)
        lines.extend(
            f"{name} access={a} layout={l}"
            for name, (a, l) in sorted(nonzero.items())
        )
        ALLOWLIST.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(
            f"wrote {ALLOWLIST.relative_to(ROOT)}: "
            f"files={file_count} access={total_access} layout={total_layout}"
        )
        return 0

    if mode == "--summary":
        print(f"total access={total_access} layout={total_layout} files={file_count}")
        print("top 10:")
        ranked = sorted(
            nonzero.items(),
            key=lambda item: (-(item[1][0] + item[1][1]), -item[1][0], item[0]),
        )
        for name, (a, l) in ranked[:10]:
            print(f"{name} access={a} layout={l}")
        return 0

    for name, (a, l) in sorted(nonzero.items()):
        print(f"{name} access={a} layout={l}")
    print(f"total access={total_access} layout={total_layout} files={file_count}")

    try:
        allowed = read_allowlist()
    except FileNotFoundError:
        print(f"{ALLOWLIST.relative_to(ROOT)} not found; run with --update")
        return 1
    except ValueError as error:
        print(f"malformed allowlist line: {error}")
        return 1

    failures = []
    for name, (a, l) in sorted(current.items()):
        if (a, l) == (0, 0):
            continue
        if name not in allowed:
            failures.append(f"{name}: missing from allowlist (access={a} layout={l})")
            continue
        was_access, was_layout = allowed[name]
        if a > was_access:
            failures.append(
                f"{name}: access {a} exceeds allowed {was_access} (over by {a - was_access})"
            )
        if l > was_layout:
            failures.append(
                f"{name}: layout {l} exceeds allowed {was_layout} (over by {l - was_layout})"
            )
    for name in sorted(allowed):
        if name not in current:
            failures.append(f"{name}: allowlisted but not found in tree")

    if failures:
        for failure in failures:
            print(failure)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
