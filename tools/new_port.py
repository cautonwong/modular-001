#!/usr/bin/env python3
"""Generate a consumer-defined port.

The port is a plain callback table. The consumer takes
`const nd_<capability>_port_t *` in its constructor and stores it in its own
state; there is no bind global and no forwarding wrapper.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PORTS = ROOT / "application" / "ports"
NAME = re.compile(r"^[a-z][a-z0-9_]*$")
ACTION = re.compile(r"^([a-z][a-z0-9_]*)\s*\((.*)\)$")
PARAM = re.compile(r"^(.*?)\s*([A-Za-z_][A-Za-z0-9_]*)\s*(\[\s*\d*\s*\])?$")


def parse_action(text: str) -> tuple[str, list[tuple[str, str]]]:
    match = ACTION.match(text.strip())
    if not match:
        raise SystemExit(f"action {text!r} must look like name(type arg, ...)")
    name = match.group(1)
    params: list[tuple[str, str]] = []
    body = match.group(2).strip()
    if body:
        for raw in body.split(","):
            param = PARAM.match(raw.strip())
            if not param:
                raise SystemExit(f"cannot parse parameter {raw!r} in {name}")
            kind = param.group(1).strip()
            params.append((kind, param.group(2)))
    return name, params


def format_param(kind: str, name: str) -> str:
    kind = kind.rstrip()
    return f"{kind}{name}" if kind.endswith("*") else f"{kind} {name}"


def guard(capability: str) -> str:
    return f"ND_APPLICATION_PORTS_{capability.upper()}_H"


def render_header(capability: str, actions: list[tuple[str, list[tuple[str, str]]]]) -> str:
    lines = [
        f"#ifndef {guard(capability)}",
        f"#define {guard(capability)}",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        f"typedef struct nd_{capability}_port {{",
        "    void *ctx;",
    ]
    for name, params in actions:
        signature = ", ".join(format_param(kind, arg) for kind, arg in params)
        lines.append(f"    int (*{name})(void *ctx"
                     + (f", {signature}" if signature else "") + ");")
    lines += [
        f"}} nd_{capability}_port_t;",
        "",
        "#endif",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capability", required=True,
                        help="snake_case name, for example backlight")
    parser.add_argument("--action", action="append", required=True,
                        help='signature without return type, for example '
                             '"set(bool enable)"')
    args = parser.parse_args()

    capability = args.capability
    if capability.endswith("_port"):
        capability = capability[:-5]
    if not NAME.fullmatch(capability):
        print(f"capability {args.capability!r} is not snake_case", file=sys.stderr)
        return 1
    actions = [parse_action(text) for text in args.action]

    header = PORTS / f"{capability}.h"
    if header.exists():
        print(f"{header.relative_to(ROOT)} already exists", file=sys.stderr)
        return 1

    header.write_text(render_header(capability, actions), encoding="utf-8")
    print(f"created {header.relative_to(ROOT)}")
    print(f"next: take const nd_{capability}_port_t * in the consumer "
          "constructor, write the target adapter, and add a fake-port unit test")
    return 0


if __name__ == "__main__":
    sys.exit(main())
