#!/usr/bin/env python3
"""Check parameter_item table rows against compiled symbol sizes.

mapper_698.c writes a row in one of two ways. A row whose index falls in the
special ranges copies exactly len bytes from the array, so len must fit. Every
other row copies array[0] bytes from array + 1, so the first byte must fit.

The sizes come from the build's object files, not from the source, so a table
edit that outgrows its initializer fails here instead of reading adjacent
rodata at run time.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)
ROW = re.compile(r"\{([^{}]*)\}")
INT = re.compile(r"^(?:0[xX][0-9a-fA-F]+|\d+)$")
HEX = re.compile(r"^[0-9a-fA-F]+$")


def strip_comments(text: str) -> str:
    return COMMENT.sub(" ", text)


def parse_values(path: Path) -> dict[str, object]:
    """Collect #define values and enum members, in file order."""
    values: dict[str, object] = {}
    text = strip_comments(path.read_text(errors="replace"))
    for match in re.finditer(r"#\s*define\s+(\w+)\s+(\S+)", text):
        values[match.group(1)] = match.group(2)
    for match in re.finditer(r"\benum\b[^{;]*\{(.*?)\}", text, re.S):
        nxt = 0
        for entry in match.group(1).split(","):
            name, has_value, expr = entry.strip().partition("=")
            name = name.strip()
            if not name:
                continue
            if has_value:
                values[name] = expr.strip()
                value = resolve(expr.strip(), values)
                nxt = None if value is None else value + 1
            else:
                values[name] = nxt
                nxt = None if nxt is None else nxt + 1
    return values


def resolve(name: str, values: dict[str, object], seen: tuple[str, ...] = ()) -> int | None:
    if INT.match(name):
        return int(name, 0)
    raw = values.get(name)
    if raw is None or name in seen:
        return None
    raw = str(raw).strip().strip("()")
    if INT.match(raw):
        return int(raw, 0)
    return resolve(raw, values, seen + (name,))


def parse_table(path: Path, values: dict[str, object],
                macros: dict[str, object],
                table_name: str = "parameter_item") -> tuple[list[tuple], set[str]]:
    """Return (index, index value, length, data) rows plus referenced symbols."""
    text = strip_comments(path.read_text(errors="replace"))
    pattern = re.compile(r"\b" + re.escape(table_name) + r"\s*\[[^\]]*\]\s*=")
    rows: list[tuple] = []
    refs: set[str] = set()
    for table in pattern.finditer(text):
        start = text.find("{", table.end())
        depth = 0
        for pos in range(start, len(text)):
            if text[pos] == "{":
                depth += 1
            elif text[pos] == "}":
                depth -= 1
                if depth == 0:
                    break
        for row in ROW.finditer(text[start:pos + 1]):
            fields = [field.strip() for field in row.group(1).split(",")]
            if len(fields) < 4:
                rows.append((None, None, None, None))
                continue
            index, _same, length, data = fields[:4]
            if data == "0":
                rows.append((index, None, None, 0))
                continue
            if not data.startswith("init_"):
                rows.append((index, None, None, data))
                continue
            refs.add(data)
            rows.append((index, resolve(index, values), resolve(length, macros), data))
    return rows, refs


def build_symbols(paths: list[Path]) -> dict[str, dict]:
    symbols: dict[str, dict] = {}
    for path in paths:
        out = subprocess.run(["objdump", "-t", str(path)],
                             capture_output=True, text=True).stdout
        for line in out.splitlines():
            parts = line.split()
            if len(parts) < 6 or not parts[5].startswith("init_"):
                continue
            if parts[2] != "O" or parts[3].startswith("*"):
                continue
            if not HEX.match(parts[0]) or not HEX.match(parts[4]):
                continue
            symbols[parts[5]] = {"size": int(parts[4], 16), "obj": str(path),
                                 "sec": parts[3], "addr": int(parts[0], 16),
                                 "first": None}
    return symbols


def read_first_byte(symbol: dict) -> int | None:
    if symbol["first"] is not None:
        return symbol["first"]
    out = subprocess.run(["objdump", "-s", "-j", symbol["sec"], symbol["obj"]],
                         capture_output=True, text=True).stdout
    for line in out.splitlines():
        parts = line.split("  ", 1)[0].split()
        if len(parts) < 2 or not HEX.match(parts[0]):
            continue
        try:
            data = b"".join(bytes.fromhex(part) for part in parts[1:5])
        except ValueError:
            continue
        base = int(parts[0], 16)
        if base <= symbol["addr"] < base + len(data):
            symbol["first"] = data[symbol["addr"] - base]
            return symbol["first"]
    return None


def special_ranges(values: dict[str, object], override: str | None) -> list[tuple[int, int]] | None:
    if override:
        ranges = []
        for part in override.split(","):
            low, _, high = part.partition(":")
            start, end = resolve(low.strip(), values), resolve(high.strip(), values)
            if start is None or end is None:
                return None
            ranges.append((start, end))
        return ranges
    names = ("MinuteFreeze_oad", "ZeroLineErrS_seq", "__INITFLAG2_Start", "__INITFLAG2_End")
    bounds = [resolve(name, values) for name in names]
    if None in bounds:
        return None
    return [(bounds[0], bounds[1]), (bounds[2], bounds[3])]


def check_rows(rows: list[tuple], refs: set[str], symbols: dict[str, dict],
               ranges: list[tuple[int, int]],
               direct_all: bool = False) -> tuple[list[str], Counter, Counter]:
    violations: list[str] = []
    stats: Counter = Counter()
    skipped: Counter = Counter()
    for index, index_value, length, data in rows:
        if data is None:
            skipped["incomplete row"] += 1
            continue
        if not data:
            skipped["zero data pointer"] += 1
            continue
        if not data.startswith("init_"):
            skipped["non-init data pointer"] += 1
            continue
        if index_value is None:
            skipped["unresolved index"] += 1
            continue
        if length is None:
            skipped["non-literal length"] += 1
            continue
        symbol = symbols.get(data)
        if direct_all or any(low <= index_value <= high for low, high in ranges):
            stats["direct"] += 1
            if symbol is not None and length > symbol["size"]:
                violations.append(
                    f"direct row {index}: len={length} exceeds {data} size={symbol['size']}")
        else:
            first = None if symbol is None else read_first_byte(symbol)
            if symbol is not None and first is None:
                skipped["no section bytes"] += 1
                continue
            stats["prefix"] += 1
            if symbol is not None and first > symbol["size"] - 1:
                violations.append(
                    f"prefix row {index}: payload={first} exceeds {data} size={symbol['size']}")
    for name in sorted(refs - symbols.keys()):
        violations.append(f"missing symbol {name}: referenced by the table but not in the objects")
    return violations, stats, skipped


def self_test() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        source = Path(tmp) / "fake.c"
        source.write_text(
            "const parameter_item_t parameter_item[] = {\n"
            "{ A_oad, 0x01, 4, init_A },\n"
            "{ B, 0x01, 1, init_B },\n"
            "{ C, 0x01, 2, init_C },\n"
            "{ D, 0x01, 1, init_Missing },\n};\n")
        size_map = Path(tmp) / "sizes.json"
        size_map.write_text(json.dumps({
            "init_A": {"size": 3},
            "init_B": {"size": 3, "first": 4},
            "init_C": {"size": 2, "first": 1},
        }))
        values = {"A_oad": 1, "B": 2, "C": 3, "D": 4}
        rows, refs = parse_table(source, values, {})
        violations, _stats, _skipped = check_rows(rows, refs, json.loads(size_map.read_text()),
                                                  [(0, 1)])
        assert len(violations) == 3, violations
    print("self-test ok")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", help="mapper source holding the parameter_item table")
    parser.add_argument("--objects", help="directory searched recursively for *.o")
    parser.add_argument("--elf", help="linker output used instead of object files")
    parser.add_argument("--ranges", help="comma separated start:end index overrides")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    if not args.source or bool(args.objects) == bool(args.elf):
        parser.error("--source and exactly one of --objects or --elf are required")

    source = Path(args.source)
    target = source.parent.parent
    port = target / "app" / "port.h"
    if not port.is_file():
        print(f"missing {port}; cannot resolve table indices", file=sys.stderr)
        return 1
    values = parse_values(port)
    macros: dict[str, object] = {}
    for header in (target / "app" / "events" / "gs_record_tables.h",
                   source.parent / "factory_cfg.h"):
        if header.is_file():
            macros.update(parse_values(header))
        else:
            print(f"warning: missing {header}", file=sys.stderr)

    ranges = special_ranges(values, args.ranges)
    if ranges is None:
        print("cannot resolve the special index ranges; pass --ranges start:end,...",
              file=sys.stderr)
        return 1

    rows, refs = parse_table(source, values, macros)
    rows1, refs1 = parse_table(source, values, macros, "parameter_item_1")
    paths = [Path(args.elf)] if args.elf else sorted(Path(args.objects).rglob("*.o"))
    symbols = build_symbols(paths)
    violations, stats, skipped = check_rows(rows, refs, symbols, ranges)
    extra, extra_stats, extra_skipped = check_rows(rows1, refs1, symbols, ranges,
                                                   direct_all=True)
    violations += extra
    stats.update(extra_stats)
    skipped.update(extra_skipped)
    total_rows = len(rows) + len(rows1)

    for violation in violations:
        print(violation)
    checked = stats["direct"] + stats["prefix"]
    if not checked:
        print("no table rows were checked", file=sys.stderr)
        return 1
    reasons = ", ".join(f"{key}={value}" for key, value in sorted(skipped.items()) if value)
    summary = (f"rows={total_rows} checked={checked} direct={stats['direct']} "
               f"prefix={stats['prefix']} skipped={sum(skipped.values())}")
    print(summary + (f" ({reasons})" if reasons else ""))
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main())
