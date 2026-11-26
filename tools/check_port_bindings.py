#!/usr/bin/env python3
"""Fail when a legacy port bind drifts from the frozen table or reappears.

tools/port_bindings.txt freezes product -> bind symbol -> expected direct
calls -> allowed callers. The evidence is the linked ELF: `objdump -d` gives
the direct bl (ARM) / call (host) edges, each attributed to the enclosing
function header. Indirect and function-pointer calls are not counted, so a
bind reachable only that way shows zero direct calls and fails. That is the
explicit decision from docs/guides/port-binding.md section 8.1.

The table has no rows, so every product takes the no-row path: `objdump -t`
lists the ELF function symbols, and any function symbol whose name starts
with nd_, stategrid_, dl698_ or resource_tool_ and ends in _bind fails.
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_TABLE = ROOT / "tools" / "port_bindings.txt"

SCRATCH_CC = "arm-none-eabi-gcc"


def scratch_objdump() -> str:
    """The cross objdump for the scratch links; --objdump is for the real ELF."""
    explicit = os.environ.get("SCRATCH_OBJDUMP") or os.environ.get("OBJDUMP")
    if explicit:
        return explicit
    found = shutil.which("arm-none-eabi-objdump")
    if found:
        return found
    result = subprocess.run([SCRATCH_CC, "-print-prog-name=objdump"],
                            capture_output=True, text=True, errors="replace")
    path = result.stdout.strip()
    if result.returncode == 0 and Path(path).is_file():
        return path
    return "arm-none-eabi-objdump"

FUNCTION = re.compile(r"^([0-9a-f]+) <([^>]+)>:$")
CALL = re.compile(
    r"^\s*[0-9a-f]+:\s+(blx?|callq?)\s+[0-9a-f]+ <([^>]+)>\s*(?:#.*)?$")


@dataclass(frozen=True)
class Row:
    product: str
    symbol: str
    expected: int
    callers: tuple[str, ...]


def load_table(path: Path) -> list[Row]:
    rows: list[Row] = []
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        if len(fields) != 4:
            raise SystemExit(f"{path}:{number}: expected 4 fields, got {len(fields)}")
        product, symbol, expected, callers = fields
        rows.append(Row(product, symbol, int(expected), tuple(callers.split(","))))
    return rows


def analyze(rows: list[Row], text: str) -> tuple[dict[str, list[str]], list[str]]:
    watched = {row.symbol for row in rows}
    calls = {symbol: [] for symbol in watched}
    mentions = {symbol: [] for symbol in watched}
    current = None
    for line in text.splitlines():
        header = FUNCTION.match(line)
        if header:
            current = header.group(2)
            continue
        call = CALL.match(line)
        if call:
            target = call.group(2)
            if target in calls:
                calls[target].append(current)
            continue
        for symbol in watched:
            if f"<{symbol}>" in line or f"<{symbol}+" in line:
                if current != symbol:
                    mentions[symbol].append(line.strip())
    errors: list[str] = []
    for row in rows:
        found = calls[row.symbol]
        if len(found) != row.expected:
            detail = f"expected {row.expected} direct call(s), found {len(found)}"
            if found:
                detail += " from " + ", ".join(str(caller) for caller in found)
            if not found and mentions[row.symbol]:
                detail += "; indirect/function-pointer calls are not counted"
            errors.append(f"{row.symbol}: {detail}")
        for caller in found:
            if caller not in row.callers:
                errors.append(f"{row.symbol}: caller {caller} not allowed "
                              f"(allowed: {', '.join(row.callers)})")
    return calls, errors


def evaluate(rows: list[Row], text: str) -> list[str]:
    return analyze(rows, text)[1]


def disassemble(objdump: str, elf: Path) -> str:
    result = subprocess.run([objdump, "-d", "--no-show-raw-insn", str(elf)],
                            capture_output=True, text=True, errors="replace")
    if result.returncode != 0:
        raise SystemExit(f"objdump failed for {elf}: {result.stderr.strip()[:400]}")
    return result.stdout


BIND = re.compile(
    r"^(?:(?:nd_|stategrid_|dl698_).+_bind|resource_tool.*_bind)$")


def bind_symbols(objdump: str, elf: Path) -> list[str]:
    result = subprocess.run([objdump, "-t", str(elf)],
                            capture_output=True, text=True, errors="replace")
    if result.returncode != 0:
        raise SystemExit(
            f"objdump -t failed for {elf}: {result.stderr.strip()[:400]}")
    names = set()
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) < 6 or not re.fullmatch(r"[0-9a-fA-F]+", fields[0]):
            continue
        if fields[2] not in ("F", "f"):
            continue
        if BIND.match(fields[-1]):
            names.add(fields[-1])
    return sorted(names)


def compile_scratch(tmp: Path, name: str, sources: dict[str, str]) -> Path:
    paths = []
    for filename, text in sources.items():
        path = tmp / f"{name}-{filename}"
        path.write_text(text, encoding="utf-8")
        paths.append(path)
    elf = tmp / f"{name}.elf"
    result = subprocess.run(
        [SCRATCH_CC, "-std=gnu11", "-mcpu=cortex-m0", "-mthumb",
         "-ffreestanding", "-Os", "-nostartfiles", "-Wl,-e,_start",
         "-o", str(elf), *(str(path) for path in paths)],
        capture_output=True, text=True, errors="replace")
    if result.returncode != 0:
        raise SystemExit("port-bindings self-check scratch compile failed: "
                         + result.stderr.strip()[:400])
    return elf


POSITIVE = """\
Disassembly of section .text:

00000000 <stategrid_platform_bind>:
       0:\tbl\t8 <nd_platform_bind>
       2:\tbx\tlr

00000008 <nd_platform_bind>:
       8:\tbx\tlr
"""

ZERO_CALLS = """\
Disassembly of section .text:

00000000 <stategrid_platform_bind>:
       0:\tbl\t8 <other_bind>
       2:\tbx\tlr

00000008 <nd_platform_bind>:
       8:\tbx\tlr
"""

DUPLICATE_CALLS = """\
Disassembly of section .text:

00000000 <stategrid_platform_bind>:
       0:\tbl\t8 <nd_platform_bind>
       2:\tbl\t8 <nd_platform_bind>
       4:\tbx\tlr

00000008 <nd_platform_bind>:
       8:\tbx\tlr
"""

DISALLOWED_CALLER = """\
Disassembly of section .text:

00000000 <rogue_caller>:
       0:\tbl\t8 <nd_platform_bind>
       2:\tbx\tlr

00000008 <nd_platform_bind>:
       8:\tbx\tlr
"""

DUPLICATE_SOURCES = {
    "bind.c": """\
static void *bound_port;
void nd_platform_bind(const void *port) { bound_port = (void *)port; }
""",
    "caller.c": """\
void nd_platform_bind(const void *port);
static volatile int calls;
void stategrid_platform_bind(const void *port)
{
    nd_platform_bind(port);
    calls++;
    nd_platform_bind(port);
    calls++;
}
void _start(void) { stategrid_platform_bind(0); }
""",
}

INDIRECT_SOURCES = {
    "bind.c": """\
static void *bound_port;
void nd_platform_bind(const void *port) { bound_port = (void *)port; }
""",
    "caller.c": """\
typedef void (*bind_fn)(const void *port);
void nd_platform_bind(const void *port);
static volatile bind_fn indirect_bind = nd_platform_bind;
void stategrid_platform_bind(const void *port) { indirect_bind(port); }
void _start(void) { stategrid_platform_bind(0); }
""",
}

EMPTY_PLAIN_SOURCES = {
    "start.c": "void sim_uart_bind(void) {}\nvoid _start(void) {}\n",
}


def empty_bind_sources(bind_name: str) -> dict[str, str]:
    return {"bind.c": f"void {bind_name}(void) {{}}\nvoid _start(void) {{}}\n"}


def self_check() -> int:
    rows = [Row("self-check", "nd_platform_bind", 1, ("stategrid_platform_bind",))]
    if evaluate(rows, POSITIVE):
        print("port-bindings self-check failed: an allowed single call was rejected")
        return 1
    negatives = (
        ("zero direct calls", ZERO_CALLS, "found 0"),
        ("duplicate direct calls", DUPLICATE_CALLS, "found 2"),
        ("disallowed caller", DISALLOWED_CALLER, "not allowed"),
    )
    for label, text, fragment in negatives:
        if not any(fragment in error for error in evaluate(rows, text)):
            print(f"port-bindings self-check failed: {label} was not rejected")
            return 1
    with tempfile.TemporaryDirectory(prefix="port-bindings-self-check-") as name:
        tmp = Path(name)
        objdump = scratch_objdump()
        duplicate = compile_scratch(tmp, "duplicate", DUPLICATE_SOURCES)
        errors = evaluate(rows, disassemble(objdump, duplicate))
        if not any("found 2" in error for error in errors):
            print("port-bindings self-check failed: a scratch link with a "
                  "duplicated bind was not rejected")
            return 1
        indirect = compile_scratch(tmp, "indirect", INDIRECT_SOURCES)
        errors = evaluate(rows, disassemble(objdump, indirect))
        if not any("found 0" in error for error in errors):
            print("port-bindings self-check failed: a scratch link with only "
                  "an indirect call was not rejected")
            return 1
        for name in ("nd_platform_bind", "stategrid_platform_bind",
                     "dl698_parameter_bind", "resource_tool_bind"):
            bind = compile_scratch(tmp, f"empty-{name}",
                                   empty_bind_sources(name))
            if bind_symbols(objdump, bind) != [name]:
                print("port-bindings self-check failed: an empty table with "
                      f"a synthetic {name} symbol was not rejected")
                return 1
        plain = compile_scratch(tmp, "empty-plain", EMPTY_PLAIN_SOURCES)
        if bind_symbols(objdump, plain):
            print("port-bindings self-check failed: a non-prefixed *_bind "
                  "symbol was rejected")
            return 1
    print("port-bindings self-check ok: zero, duplicate, disallowed caller, "
          "scratch duplicated bind, and scratch indirect-only call all fail; "
          "an empty table rejects a synthetic bind for each guarded prefix "
          "and passes a non-prefixed one")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--table", type=Path, default=DEFAULT_TABLE)
    parser.add_argument("--product")
    parser.add_argument("--elf", type=Path,
                        help="linked ELF to inspect (firmware or host sim)")
    parser.add_argument("--objdump", default="objdump")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if args.self_check:
        return self_check()
    if not args.product or not args.elf:
        parser.error("--product and --elf are required")

    if self_check() != 0:
        return 1
    rows = [row for row in load_table(args.table) if row.product == args.product]
    if not rows:
        symbols = bind_symbols(args.objdump, args.elf)
        for name in symbols:
            print(f"port-bindings FAILED: {name}: guarded *_bind function "
                  f"symbol in {args.elf}")
        if symbols:
            return 1
        print(f"port bindings ok (0-bind: no guarded *_bind symbol in {args.elf})")
        return 0

    text = disassemble(args.objdump, args.elf)
    calls, errors = analyze(rows, text)
    for row in rows:
        callers = calls[row.symbol]
        print(f"[port-bindings] {row.symbol}: {len(callers)} direct call(s)"
              f" from {', '.join(str(caller) for caller in callers) or 'nobody'}")
    for error in errors:
        print(f"port-bindings FAILED: {error}")
    if errors:
        return 1
    print(f"port bindings ok ({len(rows)} binds exactly once in {args.elf})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
