#!/usr/bin/env python3
"""Build and verify every target discovered under targets/.

The root ci.sh is the only CI entry point. This module keeps the matrix
explicit while allowing new target directories to be added without editing
the shell script.
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"


class Matrix:
    def __init__(self) -> None:
        self.failures: list[str] = []

    def run(self, label: str, command: list[str], cwd: Path | None = None) -> bool:
        print(f"[{label}] {' '.join(command)}", flush=True)
        result = subprocess.run(command, cwd=cwd or ROOT)
        if result.returncode != 0:
            self.failures.append(label)
            print(f"[{label}] FAILED ({result.returncode})", flush=True)
            return False
        print(f"[{label}] ok", flush=True)
        return True

    def check(self, label: str, command: list[str]) -> None:
        self.run(label, command)


def target_kind(path: Path) -> str:
    if path.name == "host":
        return "host"
    if (path / "app" / "CMakeLists.txt").is_file():
        return "cmake"
    if list((path / "iar for arm").glob("*.ewp")):
        return "legacy"
    return "skip"


def configure_build(matrix: Matrix, target: Path) -> Path | None:
    build = BUILD / f"ci-{target.name}"
    if not matrix.run(f"{target.name}:configure",
                      ["cmake", "-S", str(target / "app"), "-B", str(build),
                       "-G", "Ninja",
                       "-DCMAKE_TOOLCHAIN_FILE=" + str(ROOT / "cmake/toolchains/arm-none-eabi.cmake"),
                       "-DFREERTOS_KERNEL_DIR=" + os.environ.get(
                           "FREERTOS_KERNEL_DIR",
                           str(ROOT / "infra/third_party/freertos/FreeRTOS-KernelV11.3.3"))]):
        return None
    if not matrix.run(f"{target.name}:build", ["cmake", "--build", str(build)]):
        return None
    return build


def firmware_elf(build: Path, target: str) -> Path:
    name = "idis_freertos.elf" if target.endswith("_freertos") else "idis.elf"
    return build / name


def run_target_acceptance(matrix: Matrix, target: Path, host_build: Path) -> None:
    acceptance = target / "tests/acceptance"
    if not acceptance.is_dir():
        return
    for script in sorted(acceptance.glob("*.sh")):
        matrix.run(f"{target.name}:accept:{script.stem}",
                   ["bash", str(script), str(host_build / "idis_host_runner")])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    root = args.root.resolve()
    matrix = Matrix()

    print("== host tests ==")
    host_build = BUILD / "ci-host"
    matrix.run("host:configure", ["cmake", "-S", str(root / "tests"),
                                 "-B", str(host_build), "-G", "Ninja"])
    matrix.run("host:build", ["cmake", "--build", str(host_build)])
    matrix.run("host:test", ["ctest", "--test-dir", str(host_build),
                             "--output-on-failure"])

    print("== shared checks ==")
    for label, command in [
        ("check:components", [sys.executable, str(root / "tools/check_components.py")]),
        ("check:domain", [sys.executable, str(root / "tools/check_domain_contracts.py")]),
        ("check:rtos", [sys.executable, str(root / "tools/check_rtos_targets.py")]),
        ("check:ids", [sys.executable, str(root / "tools/check_idis_ids.py")]),
        ("check:audit", [sys.executable, str(root / "tools/check_idis_audit.py"),
                         str(host_build / "idis_host_runner")]),
        ("check:acceptance-matrix", [sys.executable, str(root / "tools/check_acceptance_matrix.py")]),
        ("check:layers", [str(root / "tools/check-layers.sh")]),
        ("check:platform", [sys.executable, str(root / "tools/check_platform_boundary.py")]),
        ("check:meter-record", [sys.executable, str(root / "tools/gen_meter_record.py"), "--check"]),
        ("check:products", [sys.executable, str(root / "tools/product_manifest.py"), "--check-all"]),
    ]:
        matrix.check(label, command)

    targets = sorted(path for path in (root / "targets").iterdir() if path.is_dir())
    print("== target matrix ==")
    for target in targets:
        kind = target_kind(target)
        if kind == "host":
            print(f"TARGET {target.name}: unit=shared build=host acceptance=shared")
            continue
        if kind == "skip":
            print(f"TARGET {target.name}: skipped (no build descriptor)")
            continue
        if kind == "legacy":
            matrix.check(f"{target.name}:legacy", [str(root / "tools/check-legacy.sh")])
            matrix.check(f"{target.name}:arm", [str(root / "tools/check-arm.sh")])
            matrix.check(f"{target.name}:persistence", [str(root / "tools/check-persistence.sh")])
            matrix.check(f"{target.name}:instance", [sys.executable, str(root / "tools/check_instance_boundary.py")])
            matrix.check(f"{target.name}:mbase", [sys.executable, str(root / "tools/check_mbase_access.py")])
            matrix.check(f"{target.name}:link", [sys.executable, str(root / "tools/check-firmware-link.py"),
                                                "--product", "meter_electricity_single_phase_cn"])
            print(f"TARGET {target.name}: build=gnu unit=shared acceptance=firmware_sim+firmware_acceptance")
            continue
        build = configure_build(matrix, target)
        if build is None:
            continue
        elf = firmware_elf(build, target.name)
        matrix.check(f"{target.name}:firmware", [sys.executable, str(root / "tools/check_idis_firmware.py"),
                                                str(elf)])
        run_target_acceptance(matrix, target, host_build)
        print(f"TARGET {target.name}: build=cmake unit=shared acceptance=target")

    if matrix.failures:
        print("== FAILED ==")
        for failure in matrix.failures:
            print(f"FAIL {failure}")
        return 1
    print("== all targets ok ==")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
