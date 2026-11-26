#!/usr/bin/env python3
"""Guard the domain layer against COSEM/DLMS/target leakage."""

import argparse
import pathlib
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "domain"
BASELINE = ROOT / "tools" / "domain_purity_baseline.txt"

BANNED = [
    r"\bnd_obis_t\b",
    r"\bND_OBIS_",
    r"\bclass_id\b",
    r"\battribute_id\b",
    r"\bnd_axdr",
    r'"components/protocol/dlms',
    r'"targets/',
]

COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)

PATH_FINDINGS = [
]

TOKEN_FINDINGS = [
    ("C2.2", "oad", re.compile(r"(?:\b|_)oad(?:_|\b)", re.IGNORECASE)),
    ("C2.1", "report_flag", re.compile(r"report_flag")),
    ("C2.1", "report_mode", re.compile(r"report_mode")),
    ("C2.1", "report_3320", re.compile(r"report[a-z_]*3320", re.IGNORECASE)),
    ("C2.1", "report_3011", re.compile(r"report[a-z_]*3011", re.IGNORECASE)),
    ("C2.1", "report_2015", re.compile(r"report[a-z_]*2015", re.IGNORECASE)),
    ("C3.1", "backlight", re.compile(r"backlight")),
    ("C3.1", "page_id", re.compile(r"page_id")),
    ("C3.2", "key_pressed", re.compile(r"key_pressed")),
    ("C3.2", "full_display", re.compile(r"full_display")),
    ("C6.1", "0x5A", re.compile(r"\b0x5a[uUlL]*\b", re.IGNORECASE)),
    ("C2.8", "nd_mbus_", re.compile(r"\bnd_mbus_")),
    ("C2.5", "nd_push_", re.compile(r"\bnd_push_")),
    ("C2.4", "time_format", re.compile(r"time_format")),
]


def strip_comments(text: str) -> str:
    return COMMENTS.sub(lambda match: "\n" * match.group(0).count("\n"), text)


def banned_errors(root: Path = ROOT) -> list[str]:
    violations = []
    for path in sorted((root / "domain").rglob("*.h")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for pattern in BANNED:
            for match in re.finditer(pattern, text):
                line = text.count("\n", 0, match.start()) + 1
                violations.append(f"{path.relative_to(root)}:{line}: {pattern}")
    return violations


def scan(root: Path = ROOT) -> dict[str, set[tuple[str, str]]]:
    found: dict[str, set[tuple[str, str]]] = {}
    for path in sorted((root / "domain").rglob("*")):
        if path.suffix not in (".h", ".c"):
            continue
        text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        rel = path.relative_to(root).as_posix()
        for finding, pattern in PATH_FINDINGS:
            if pathlib.PurePosixPath(rel).match(pattern):
                found.setdefault(rel, set()).add((finding, "path"))
        for finding, word, pattern in TOKEN_FINDINGS:
            if pattern.search(text):
                found.setdefault(rel, set()).add((finding, word))
    return found


def parse(lines: list[str]) -> dict[str, set[tuple[str, str]]]:
    baseline: dict[str, set[tuple[str, str]]] = {}
    for line in lines:
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        path, finding, word = line.rsplit(None, 2)
        baseline.setdefault(path, set()).add((finding, word))
    return baseline


def load(path: Path = BASELINE) -> dict[str, set[tuple[str, str]]]:
    return parse(path.read_text(encoding="utf-8").splitlines())


def render(current: dict[str, set[tuple[str, str]]]) -> str:
    lines = ["# path finding word"]
    for path in sorted(current):
        for finding, word in sorted(current[path]):
            lines.append(f"{path} {finding} {word}")
    return "\n".join(lines) + "\n"


def summary(current: dict[str, set[tuple[str, str]]]) -> str:
    total = sum(len(found) for found in current.values())
    return f"{total} entries in {len(current)} files"


def diff(baseline: dict[str, set[tuple[str, str]]],
         current: dict[str, set[tuple[str, str]]]) -> list[str]:
    failures = []
    for path in sorted(current):
        for finding, word in sorted(current[path] - baseline.get(path, set())):
            failures.append(f"{path}: new domain purity hit {finding} {word!r}; "
                            "move the protocol/UI concern out of domain, or "
                            "--update if it belongs there")
    for path in sorted(baseline):
        for finding, word in sorted(baseline[path] - current.get(path, set())):
            failures.append(f"{path}: baseline entry {finding} {word!r} has no "
                            "hit; run --update")
    return failures


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root / "domain" / "demo").mkdir(parents=True)
        clean = root / "domain" / "demo" / "clean.h"
        clean.write_text(
            "/* 0x5A, 0xAA and OAD in a comment are not hits */\n"
            "#define DEMO_TOP 0xFFFF\n"
            "#define DEMO_EVENT 0xAA\n"
            "struct demo_fifo { uint16_t push; uint16_t download; uint16_t road; };\n"
            "int nd_clean(int value);\n", encoding="utf-8")
        first = scan(root)
        if first:
            print(f"domain purity self-check failed: clean domain reported {first}")
            return False
        baseline_file = root / "baseline.txt"
        baseline_file.write_text(render(first), encoding="utf-8")
        if diff(load(baseline_file), first):
            print("domain purity self-check failed: empty baseline does not clear")
            return False
        hit = root / "domain" / "demo" / "leak.c"
        hit.write_text(
            "typedef unsigned oad_type;\n"
            "uint32_t oad;\n"
            "int nd_report_flag_matches(int flag) { return flag; }\n"
            "int nd_report_3320_variant(void);\n",
            encoding="utf-8")
        added = diff(load(baseline_file), scan(root))
        if not any("oad" in message for message in added):
            print(f"domain purity self-check failed: oad hit reported {added}")
            return False
        if not any("report_flag" in message for message in added):
            print(f"domain purity self-check failed: report_flag hit reported {added}")
            return False
        if not any("report_3320" in message for message in added):
            print(f"domain purity self-check failed: report_3320 hit reported {added}")
            return False
        module_probe = root / "domain" / "demo" / "module_probe.c"
        module_probe.write_text(
            "int nd_mbus_init(void);\n"
            "int nd_push_init(void);\n"
            "int time_format_probe(void);\n"
            "uint32_t nd_event_oad_view(void);\n", encoding="utf-8")
        module = diff(load(baseline_file), scan(root))
        for label in ("nd_mbus_", "nd_push_", "time_format", "oad"):
            if not any(label in message for message in module):
                print(f"domain purity self-check failed: {label} hit reported {module}")
                return False
        module_probe.unlink()
        baseline_file.write_text(render(scan(root)), encoding="utf-8")
        hit.unlink()
        stale = diff(load(baseline_file), scan(root))
        if not any("no hit" in message for message in stale):
            print(f"domain purity self-check failed: stale entry reported {stale}")
            return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if not tree_check():
        return 1

    current = scan(ROOT)
    if args.update:
        BASELINE.write_text(render(current), encoding="utf-8")

    errors = banned_errors(ROOT) + diff(load(BASELINE), current)
    if args.self_check and not errors:
        print("domain purity self-check ok (sentinel ignored, token and path hits "
              "rejected, stale entry rejected)")
    print(f"domain purity: {summary(current)}")
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    if errors:
        return 1
    print("domain contracts clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
