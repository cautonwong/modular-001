#!/usr/bin/env python3
"""Generate an IDIS acceptance case from a runner key/value contract.

The case asserts only what the host runner prints. Values are passed with
--require key=value for the first run and --restart-require for the second,
so the persistence pattern is generated instead of retyped.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_DIR = ROOT / "targets" / "idis_ip_meter" / "tests" / "acceptance"
CASE_ID = re.compile(r"^[a-z][a-z0-9_]*\.[a-z][a-z0-9_-]*$")


def pair(text: str) -> tuple[str, str]:
    key, sep, value = text.partition("=")
    if not sep or not key:
        raise SystemExit(f"expected key=value, got {text!r}")
    return key, value


def render(case_id: str, description: str, tags: list[str], seconds: int,
           start_time: str | None, requires: list[tuple[str, str]],
           restart_requires: list[tuple[str, str]]) -> str:
    lines = [
        "from cases import case",
        "",
        "",
        "def require(run, key: str, expected: str | None = None) -> str:",
        "    actual = run.values.get(key)",
        "    assert actual is not None, (",
        "        f\"missing {key} in runner output: {run.stdout[-300:]!r}\")",
        "    if expected is not None:",
        "        assert actual == expected, f\"{key}={actual}, expected {expected}\"",
        "    return actual",
        "",
        "",
        "@case(",
        f'    "{case_id}",',
        f'    "{description}",',
        f'    {tuple(tags)!r},',
        ")",
        f"def {case_id.split('.', 1)[1].replace('-', '_')}(ctx):",
    ]
    start = "" if start_time is None else f', start_time="{start_time}"'
    lines.append(f"    first = ctx.run(seconds={seconds}{start})")
    lines.append('    assert first.exit_code == 0, '
                 'f"runner exited {first.exit_code}: {first.stderr[-200:]}"')
    for key, value in requires:
        lines.append(f'    require(first, "{key}", "{value}")')
    if restart_requires:
        lines += [
            "",
            "    second = ctx.run(seconds=2)",
            '    assert second.exit_code == 0, '
            'f"runner exited {second.exit_code}: {second.stderr[-200:]}"',
        ]
        for key, value in restart_requires:
            lines.append(f'    require(second, "{key}", "{value}")')
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--id", required=True,
                        help="case id, for example idis.battery-low")
    parser.add_argument("--description", required=True)
    parser.add_argument("--tags", default="idis",
                        help="comma separated tags")
    parser.add_argument("--seconds", type=int, default=10)
    parser.add_argument("--start-time", default=None)
    parser.add_argument("--require", action="append", default=[],
                        metavar="KEY=VALUE",
                        help="assertion on the first run, repeatable")
    parser.add_argument("--restart-require", action="append", default=[],
                        metavar="KEY=VALUE",
                        help="assertion on a second run, repeatable")
    parser.add_argument("--dir", type=Path, default=DEFAULT_DIR)
    args = parser.parse_args()

    if not CASE_ID.fullmatch(args.id):
        print(f"case id {args.id!r} must look like product.behavior",
              file=sys.stderr)
        return 1
    slug = args.id.split(".", 1)[1].replace("-", "_")
    path = args.dir / f"test_{slug}.py"
    if path.exists():
        print(f"{path} already exists", file=sys.stderr)
        return 1

    tags = [tag.strip() for tag in args.tags.split(",") if tag.strip()]
    text = render(args.id, args.description, tags, args.seconds,
                  args.start_time, [pair(item) for item in args.require],
                  [pair(item) for item in args.restart_require])
    args.dir.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    print(f"created {path.relative_to(ROOT)}")
    print("next: run python3 tests/acceptance/run.py --runner "
          "build/ci-host/idis_host_runner "
          f"--cases {args.dir.relative_to(ROOT)} "
          f"--workdir /tmp/acc --evidence-dir /tmp/acc-ev "
          f"--select case:{args.id}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
