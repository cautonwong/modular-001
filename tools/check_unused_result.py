#!/usr/bin/env python3
"""Ratchet discarded results of ND_MUST_USE functions.

A declaration marked ND_MUST_USE carries warn_unused_result, so a silent
discard fails the compiler only where -Wall -Werror is on. Target builds and
deliberate discards escape that check. This scan counts every call of an
annotated function whose result is not consumed, including `(void)` casts and
ND_RECORD_STORAGE_DISCARD wrappers, and freezes the current count per file. A new discard, or a higher count in a
file, fails; delete a discard and run --update to lower the baseline.
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "unused_result_baseline.txt"
ANNOTATION = "ND_MUST_USE"
SOURCES = ("domain", "application", "components", "infra", "targets", "tests")
SKIP_PARTS = {"build", "third_party", ".claude", ".git"}
TYPE_TOKEN = re.compile(r"^[A-Za-z_]\w*$")
STATEMENT_WORDS = {"return", "if", "while", "for", "switch", "sizeof", "case"}
DISCARD_MACROS = ("ND_RECORD_STORAGE_DISCARD",)


def source_files(root: Path):
    for base in SOURCES:
        start = root / base
        if not start.is_dir():
            continue
        for path in sorted(start.rglob("*")):
            if path.suffix not in (".c", ".h"):
                continue
            relative = path.relative_to(root)
            if SKIP_PARTS & set(relative.parts):
                continue
            yield path


def header_name(line: str) -> str | None:
    head = line.split(ANNOTATION, 1)[1].split("(", 1)[0]
    names = re.findall(r"[A-Za-z_]\w*", head)
    return names[-1] if names else None


def annotated_functions(root: Path) -> set[str]:
    names: set[str] = set()
    for path in source_files(root):
        if path.suffix != ".h":
            continue
        lines = path.read_text(errors="replace").splitlines()
        for index, line in enumerate(lines):
            stripped = line.strip()
            if stripped.startswith("#") or ANNOTATION not in stripped:
                continue
            text = stripped
            if ANNOTATION == text or not re.search(r"[A-Za-z_]\w*\s*\(", text.split(ANNOTATION, 1)[1]):
                if index + 1 < len(lines):
                    text += " " + lines[index + 1].strip()
            name = header_name(text)
            if name is not None:
                names.add(name)
    return names


def declaration(prefix: str) -> bool:
    tokens = prefix.replace("*", " ").split()
    return bool(tokens) and not (set(tokens) & STATEMENT_WORDS) and all(
        TYPE_TOKEN.match(token) for token in tokens)


def ends_with_expression(text: str) -> bool:
    stripped = text.rstrip()
    if not stripped:
        return False
    if stripped.split()[-1] == "return":
        return True
    return stripped[-1] in "=,(!&|?:+-*/%<>"


def consumed(prefix: str, previous: str) -> bool:
    stripped = prefix.rstrip()
    if not stripped.strip():
        return ends_with_expression(previous)
    if stripped.endswith("(void)") or any(
            stripped.endswith(macro + "(") for macro in DISCARD_MACROS):
        return False
    # Only the text after the last statement boundary belongs to this call.
    boundary = max(stripped.rfind("{"), stripped.rfind(";"), stripped.rfind("}"))
    if boundary >= 0:
        tail = stripped[boundary + 1:]
        if not tail.strip():
            return False
        stripped = tail
    return any(mark in stripped for mark in
               ("=", "return", "(", ",", "!", "&", "|", "?", ":", "+", "-",
                "*", "/", "%", "<", ">"))


def counts(root: Path = ROOT) -> dict[str, int]:
    names = annotated_functions(root)
    found: dict[str, int] = {}
    if not names:
        return found
    call = re.compile(r"\b(" + "|".join(sorted(map(re.escape, names))) + r")\s*\(")
    for path in source_files(root):
        text = path.read_text(errors="replace")
        lines = text.splitlines()
        previous = ""
        block_comment = False
        for line in lines:
            if line.lstrip().startswith("#"):
                continue
            scan = line
            code = ""
            position = 0
            while position < len(scan):
                if block_comment:
                    end = scan.find("*/", position)
                    if end < 0:
                        position = len(scan)
                        break
                    block_comment = False
                    position = end + 2
                    continue
                start = scan.find("/*", position)
                comment = scan.find("//", position)
                if comment >= 0 and (start < 0 or comment < start):
                    code += scan[position:comment]
                    position = len(scan)
                    break
                if start < 0:
                    code += scan[position:]
                    position = len(scan)
                    break
                code += scan[position:start]
                block_comment = True
                position = start + 2
            for match in call.finditer(code):
                prefix = code[:match.start()]
                if declaration(prefix):
                    continue
                if consumed(prefix, previous):
                    continue
                key = path.relative_to(root).as_posix()
                found[key] = found.get(key, 0) + 1
            if code.strip():
                previous = code
    return found


def load(path: Path = BASELINE) -> dict[str, int]:
    baseline: dict[str, int] = {}
    if not path.is_file():
        return baseline
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        name, number = line.rsplit(None, 1)
        baseline[name] = int(number)
    return baseline


def render(current: dict[str, int]) -> str:
    return "\n".join(["# file discarded_calls",
                      *[f"{path} {n}" for path, n in current.items()]]) + "\n"


def diff(baseline: dict[str, int], current: dict[str, int]) -> list[str]:
    failures = []
    for path, number in sorted(current.items()):
        allowed = baseline.get(path, 0)
        if number > allowed:
            failures.append(f"{path}: {number} discarded results, baseline {allowed}")
        elif number < allowed:
            failures.append(f"{path}: baseline says {allowed}, found {number}; "
                            "run --update")
    for path in sorted(set(baseline) - set(current)):
        failures.append(f"{path}: baseline entry is stale; run --update")
    return failures


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        ports = root / "application" / "ports"
        ports.mkdir(parents=True)
        header = ports / "sample.h"
        header.write_text(
            "#ifndef ND_MUST_USE\n"
            "#define ND_MUST_USE __attribute__((warn_unused_result))\n"
            "#endif\n"
            "ND_MUST_USE int must_use_fn(int value);\n"
            "int plain_fn(int value);\n", encoding="utf-8")
        source = root / "application" / "sample.c"
        source.write_text(
            "// must_use_fn(90);\n"
            "void discard(void) { must_use_fn(1); }\n"
            "void explicit_discard(void) { (void)must_use_fn(2); }\n"
            "int used_return(void) { return must_use_fn(3); }\n"
            "int used_assign(void) { int r = must_use_fn(4); return r; }\n"
            "void used_argument(void) { consume(must_use_fn(5)); }\n"
            "void used_continuation(void) {\n"
            "    consume(\n"
            "        must_use_fn(6));\n"
            "}\n"
            "void discard_again(void) {\n"
            "    must_use_fn(7);\n"
            "}\n"
            "void macro_discard(void) { "
            "ND_RECORD_STORAGE_DISCARD(must_use_fn(8)); }\n"
            "void plain_call(void) { plain_fn(8); }\n"
            "int must_use_fn(int value) { return value; }\n", encoding="utf-8")
        got = counts(root)
        expected = {"application/sample.c": 4}
        if got != expected:
            print(f"unused-result self-check failed: scan counted {got}, "
                  f"expected {expected}")
            return False
        baseline = root / "baseline.txt"
        baseline.write_text(render(got), encoding="utf-8")
        if diff(load(baseline), got):
            print("unused-result self-check failed: clean baseline rejected")
            return False
        original = source.read_text(encoding="utf-8")
        source.write_text(original + "void new_discard(void) { must_use_fn(10); }\n",
                          encoding="utf-8")
        if not any("sample.c" in message for message in diff(load(baseline), counts(root))):
            print("unused-result self-check failed: new discard not rejected")
            return False
        source.write_text(original.replace(
            "void discard(void) { must_use_fn(1); }\n", ""), encoding="utf-8")
        if not any("run --update" in message for message in diff(load(baseline), counts(root))):
            print("unused-result self-check failed: fixed discard not reported")
            return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()

    if not tree_check():
        return 1

    current = counts(args.root)
    baseline = args.root / "tools" / "unused_result_baseline.txt"
    if args.update:
        baseline.write_text(render(current), encoding="utf-8")
        print(f"unused-result baseline updated: {sum(current.values())} discards")
        return 0

    errors = diff(load(baseline), current)
    if args.self_check and not errors:
        print("unused-result self-check ok (discard, void cast, used, "
              "macro discard, declaration and continuation classified)")
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    if errors:
        return 1
    print(f"unused-result ok ({sum(current.values())} discards, "
          f"{len(current)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
