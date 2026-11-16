#!/usr/bin/env python3
"""Run ./ci.sh against a pristine export of HEAD.

The working tree can contain untracked files that mask a missing tracked
file. This exports the committed tree and runs the same CI entry point
there, so the result depends only on what a fresh clone would get. The
FreeRTOS kernel is an explicit external checkout, so the working tree's
path is forwarded when the environment does not name one.
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def export_head(root: Path, dest: Path) -> None:
    archive = subprocess.Popen(
        ["git", "-C", str(root), "archive", "--format=tar", "HEAD"],
        stdout=subprocess.PIPE,
    )
    assert archive.stdout is not None
    extract = subprocess.run(["tar", "-x", "-C", str(dest)], stdin=archive.stdout)
    archive.stdout.close()
    if archive.wait() != 0 or extract.returncode != 0:
        raise SystemExit("failed to export HEAD")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--keep", action="store_true", help="keep the export on success")
    args = parser.parse_args()

    root = args.root.resolve()
    dest = Path(tempfile.mkdtemp(prefix="blade-clean-"))
    keep = args.keep
    try:
        export_head(root, dest)
        print(f"[clean-clone] {dest}", flush=True)
        env = os.environ.copy()
        kernel = root / "infra/third_party/freertos/FreeRTOS-KernelV11.3.3"
        if "FREERTOS_KERNEL_DIR" not in env and kernel.exists():
            env["FREERTOS_KERNEL_DIR"] = str(kernel.resolve())
            print(f"[clean-clone] external FreeRTOS: {env['FREERTOS_KERNEL_DIR']}", flush=True)
        result = subprocess.run(["bash", str(dest / "ci.sh")], cwd=dest, env=env)
        if result.returncode != 0:
            keep = True
            print(f"[clean-clone] FAILED ({result.returncode}), kept {dest}", flush=True)
            return result.returncode
        print("[clean-clone] ok", flush=True)
        return 0
    finally:
        if not keep:
            shutil.rmtree(dest, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
