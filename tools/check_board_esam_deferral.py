#!/usr/bin/env python3
"""Fail when a deferred board_esam singleton becomes reachable.

IDIS leaves board_esam on file-scope storage until its S batch lands. The
deferral stands only while no source outside that board_esam.c references
board_esam and no image keeps the object. This guard fails on either.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGETS = ROOT / "targets"
SHARED = ("application", "components", "domain", "infra")
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
REFERENCE = re.compile(r"\bboard_esam\b")
DEFERRED_STORAGE = re.compile(r"\bstatic\s+nd_esam_storage_t\b")
SKIP_PARTS = {"build"}


def strip_comments(text: str) -> str:
    return COMMENTS.sub(lambda match: "\n" * match.group(0).count("\n"), text)


def sources(directory: Path) -> list[Path]:
    return [
        path for path in sorted(directory.rglob("*.c"))
        if SKIP_PARTS.isdisjoint(path.parts)
    ]


def deferred_boards(root: Path = ROOT) -> list[Path]:
    found = []
    for path in sorted((root / "targets").glob("*/board/board_esam.c")):
        text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        if DEFERRED_STORAGE.search(text):
            found.append(path)
    return found


def source_errors(root: Path = ROOT) -> list[str]:
    errors: list[str] = []
    boards = deferred_boards(root)
    for board in boards:
        target = board.parents[1]
        scoped = sources(target)
        for shared in SHARED:
            if (root / shared).is_dir():
                scoped += sources(root / shared)
        for path in scoped:
            if path == board:
                continue
            text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
            if REFERENCE.search(text):
                errors.append(
                    f"{path.relative_to(root)} references board_esam while "
                    f"{board.relative_to(root)} still holds file-scope storage; "
                    "do the board-storage-state.md S batch first")
    return errors


def elf_errors(root: Path, boards: list[Path], nm: str,
               images: list[tuple[str, Path]]) -> list[str]:
    errors: list[str] = []
    deferred = {board.parents[1].name for board in boards}
    for name, image in images:
        if name not in deferred:
            continue
        if not image.is_file():
            errors.append(f"{name}: missing image {image}")
            continue
        output = subprocess.run([nm, str(image)], capture_output=True,
                                text=True, check=False)
        if output.returncode != 0:
            errors.append(f"{name}: {nm} failed on {image}: {output.stderr.strip()}")
            continue
        hit = next(
            (line for line in output.stdout.splitlines()
             if line.split() and REFERENCE.search(line.split()[-1])), None)
        if hit is not None:
            errors.append(
                f"{name}: {image} keeps board_esam ({hit.strip()}); "
                "do the board-storage-state.md S batch first")
    return errors


def nm_hits(nm_text: str) -> bool:
    return any(
        line.split() and REFERENCE.search(line.split()[-1])
        for line in nm_text.splitlines())


def self_check_errors() -> list[str]:
    errors: list[str] = []
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        board = root / "targets" / "demo" / "board" / "board_esam.c"
        board.parent.mkdir(parents=True)
        board.write_text(
            "static nd_esam_storage_t esam_storage;\n"
            "static nd_esam_t *esam;\n", encoding="utf-8")
        clean = root / "targets" / "demo" / "app" / "clean.c"
        clean.parent.mkdir(parents=True)
        clean.write_text("/* board_esam lives elsewhere */\nint noop(void) { return 0; }\n",
                         encoding="utf-8")
        if source_errors(root):
            errors.append("self-check: deferred board with no caller was rejected")
        caller = root / "targets" / "demo" / "app" / "caller.c"
        caller.write_text("void use(void) { (void)board_esam(0); }\n", encoding="utf-8")
        found = source_errors(root)
        if not any("caller.c" in message for message in found):
            errors.append("self-check: caller of a deferred board_esam was not rejected")
        caller.unlink()
        if not nm_hits("00000000 T board_esam\n00000000 T other\n"):
            errors.append("self-check: retained board_esam symbol was not detected")
        if nm_hits("00000000 T other\n"):
            errors.append("self-check: clean image was rejected")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", action="append", default=[], metavar="TARGET=PATH",
                        help="image to scan for a retained board_esam object")
    parser.add_argument("--nm", default="arm-none-eabi-nm")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    images: list[tuple[str, Path]] = []
    for value in args.elf:
        name, _, path = value.partition("=")
        if not name or not path:
            raise SystemExit(f"--elf wants TARGET=PATH, got {value!r}")
        images.append((name, Path(path)))

    boards = deferred_boards()
    errors = source_errors() + elf_errors(ROOT, boards, args.nm, images)
    if args.self_check:
        errors.extend(self_check_errors())

    print(f"deferred boards: {[str(board.relative_to(ROOT)) for board in boards]}")
    if not errors:
        print("board_esam deferral ok")
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
