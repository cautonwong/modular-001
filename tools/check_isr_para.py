#!/usr/bin/env python3
"""Reject interrupt vector handlers that pass a bare NULL to a callback.

A vector handler has no argument of its own. Passing NULL to a function that
now expects the registered state silently disables the interrupt path: the
callee's NULL guard returns before doing any work. Dispatch through the
registered table instead:

    Isr_Function[IRQ_X].IsrFunctionRegister(Isr_Function[IRQ_X].para)

The scan looks at `*_IRQHandler` bodies in target sources and flags calls whose
only argument is NULL. Comparisons such as `ptr == NULL` and calls with other
arguments do not match.
"""
from __future__ import annotations

import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HANDLER = re.compile(r"\b([A-Za-z_]\w*_IRQHandler)\s*\([^)]*\)\s*\{")
BARE_NULL_CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(\s*NULL\s*\)\s*;")
COMPARISON = re.compile(r"(==|!=)\s*NULL|NULL\s*(==|!=)")

SELF_CHECK = (
    ("void PMU_IRQHandler(void)\n{\n    IO_Interrupt(NULL);\n}\n", 1),
    ("void PMU_IRQHandler(void)\n{\n"
     "    if (ptr == NULL) { return; }\n"
     "    Isr_Function[IRQ_PMU].IsrFunctionRegister(para);\n}\n", 0),
    ("void TMR3_IRQHandler(void)\n{\n    foo(NULL, 1);\n}\n", 0),
    ("void UART0_IRQHandler(void)\n{\n    dispatch(IRQ_UART0);\n}\n", 0),
    ("void not_a_handler(void)\n{\n    foo(NULL);\n}\n", 0),
)


def handlers(text: str) -> list[tuple[str, int, int, int]]:
    found = []
    for match in HANDLER.finditer(text):
        depth = 0
        end = len(text)
        for position in range(match.end() - 1, len(text)):
            if text[position] == "{":
                depth += 1
            elif text[position] == "}":
                depth -= 1
                if depth == 0:
                    end = position
                    break
        body_start = match.end()
        line = text.count("\n", 0, match.start()) + 1
        found.append((match.group(1), line, body_start, end))
    return found


def scan_text(text: str) -> list[tuple[int, str]]:
    findings = []
    for name, line, start, end in handlers(text):
        body = text[start:end]
        for call in BARE_NULL_CALL.finditer(body):
            if COMPARISON.search(call.group(0)):
                continue
            findings.append(
                (line + body.count("\n", 0, call.start()),
                 f"{name} calls {call.group(1)}(NULL)"))
    return findings


def scan(root: Path) -> list[str]:
    errors = []
    for path in sorted(root.rglob("*.c")):
        parts = path.relative_to(root).parts
        if parts[0] != "targets" or "build" in parts:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        for line, message in scan_text(text):
            errors.append(f"{path.relative_to(root)}:{line}: {message}; "
                          f"dispatch through the registered table")
    return errors


def tree_check() -> bool:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root / "targets" / "x").mkdir(parents=True)
        for index, (text, expected) in enumerate(SELF_CHECK):
            path = root / "targets" / "x" / f"case{index}.c"
            path.write_text(text, encoding="utf-8")
            got = len(scan_text(text))
            if got != expected:
                print(f"isr para self-check failed: case {index} got "
                      f"{got} finding(s), expected {expected}")
                return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    if not tree_check():
        return 1
    if args.self_check:
        return 0
    errors = scan(args.root)
    for error in errors:
        print(error)
    if errors:
        return 1
    print("isr para ok (no bare NULL callback in vector handlers)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
