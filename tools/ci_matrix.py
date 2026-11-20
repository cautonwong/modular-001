#!/usr/bin/env python3
"""Build and verify every target discovered under targets/.

The root ci.sh is the only CI entry point. This module keeps the matrix
explicit while allowing new target directories to be added without editing
the shell script.
"""
from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_idis_firmware import elf_sections, section_totals

ROOT = Path(__file__).resolve().parent.parent

# Linked images select their board transport at compile time. The IRQ/DMA
# lane proves both transports link; the POLL lane must stay free of them.
IRQ_MODE_DEFINES = ("-DND_BOARD_UART_MODE=1", "-DND_BOARD_SPI_MODE=2",
                    "-DND_BOARD_IIC_MODE=0")
TRANSPORT_SYMBOLS = ("nd_board_uart_rx_isr", "nd_board_spi_dma_hw_start")
TRANSPORT_TARGETS = ("idis_ip_meter", "idis_ip_meter_freertos")

# Second host build: warnings are errors and the sanitizers verify runtime
# bounds and undefined behavior. firmware_sim is excluded because it maps a
# custom section at a fixed address and builds vendor sources with -w.
STRICT_FLAGS = (
    "-Wall -Wextra -Werror -Wshadow -Wundef -Wstrict-prototypes "
    "-Wmissing-prototypes -Wvla -Wformat=2 -Wnull-dereference "
    "-Wduplicated-cond -Wlogical-op -Wjump-misses-init "
    "-fsanitize=address,undefined "
    "-fno-sanitize-recover=all -fno-omit-frame-pointer"
)

# Path-sensitive interprocedural analysis over the production libraries only;
# test fixtures are excluded because analyzer taint on fakes is noise.
ANALYZER_FLAGS = "-Wall -Wextra -Werror -fanalyzer"
ANALYZER_TARGETS = [
    "blade_domain", "blade_components", "blade_idis_skeleton", "blade_infra",
    "blade_target_stategrid", "blade_application", "blade_target_host",
]

# Conversion warnings are a ratchet: the build does not fail on them, but the
# checker rejects a new file/flag group or a higher count than the baseline.
CONVERSION_FLAGS = ("-Wconversion -Wsign-conversion -Wcast-qual "
                    "-Wdouble-promotion -Wfloat-conversion -I.")

# The whole-firmware simulation and the IDIS host runner under ASan and UBSan.
# Alignment checks are off because host pointers are eight bytes while the
# legacy layout assumes four; those findings describe the simulator, not the
# target.
SIM_SANITIZER_FLAGS = ("-fsanitize=address,undefined -fno-sanitize=alignment "
                       "-fno-sanitize-recover=all -fno-omit-frame-pointer")


class Matrix:
    def __init__(self, root: Path) -> None:
        self.root = root
        self.failures: list[str] = []

    def run(self, label: str, command: list[str], cwd: Path | None = None) -> bool:
        print(f"[{label}] {' '.join(command)}", flush=True)
        result = subprocess.run(command, cwd=cwd or self.root)
        if result.returncode != 0:
            self.failures.append(label)
            print(f"[{label}] FAILED ({result.returncode})", flush=True)
            return False
        print(f"[{label}] ok", flush=True)
        return True

    def check(self, label: str, command: list[str]) -> None:
        self.run(label, command)


def cmake_dir(path: Path) -> Path | None:
    for name in ("app", "build"):
        if (path / name / "CMakeLists.txt").is_file():
            return path / name
    return None


def target_kind(path: Path) -> str:
    if path.name == "host":
        return "host"
    if cmake_dir(path) is not None:
        return "cmake"
    if list((path / "iar for arm").glob("*.ewp")):
        return "legacy"
    return "skip"


def configure_build(matrix: Matrix, target: Path, root: Path,
                    build_root: Path, suffix: str = "",
                    defines: tuple[str, ...] = ()) -> Path | None:
    build = build_root / f"ci-{target.name}{suffix}"
    configure = ["cmake", "-S", str(cmake_dir(target)), "-B", str(build),
                 "-G", "Ninja",
                 "-DCMAKE_TOOLCHAIN_FILE=" + str(root / "cmake/toolchains/arm-none-eabi.cmake"),
                 "-DFREERTOS_KERNEL_DIR=" + os.environ.get(
                     "FREERTOS_KERNEL_DIR",
                     str(root / "infra/third_party/freertos/FreeRTOS-KernelV11.3.3"))]
    configure += list(defines)
    if not matrix.run(f"{target.name}:configure{suffix}", configure):
        return None
    if not matrix.run(f"{target.name}:build{suffix}", ["cmake", "--build", str(build)]):
        return None
    return build


def firmware_elf(build: Path, target: str) -> Path:
    name = "idis_freertos.elf" if target.endswith("_freertos") else "idis.elf"
    return build / name


def check_transport_symbols(matrix: Matrix, label: str, elf: Path,
                            irq: bool) -> None:
    nm = os.environ.get("NM", "arm-none-eabi-nm")
    output = subprocess.check_output([nm, "--defined-only", str(elf)], text=True)
    names = {line.split()[-1] for line in output.splitlines() if line.strip()}
    wrong = [name for name in TRANSPORT_SYMBOLS if (name in names) != irq]
    if wrong:
        print(f"[{label}] transport symbol mismatch: " + ", ".join(wrong),
              flush=True)
        matrix.failures.append(label)
        print(f"[{label}] FAILED", flush=True)
        return
    print(f"[{label}] ok "
          f"({'transport' if irq else 'poll'} symbols as expected)", flush=True)


def print_rom(label: str, elf: Path) -> None:
    vector, text, data, _bss, exidx = section_totals(elf_sections(str(elf)))
    print(f"[{label}] rom={vector + text + data + exidx} bytes", flush=True)


def acceptance_cases(target: Path) -> Path | None:
    acceptance = target / "tests/acceptance"
    if not acceptance.is_dir():
        return None
    cases = acceptance
    pointer = acceptance / "CASES"
    if pointer.is_file():
        cases = (acceptance / pointer.read_text(encoding="utf-8").strip()).resolve()
    return cases if cases.is_dir() else None


def run_target_acceptance(matrix: Matrix, target: Path, host_build: Path,
                          root: Path, build_root: Path, suffix: str = "",
                          runner: Path | None = None) -> None:
    acceptance = target / "tests/acceptance"
    if not acceptance.is_dir():
        return
    cases = acceptance_cases(target)
    if cases is None:
        print(f"[{target.name}:accept{suffix}] CASES points at a missing directory")
        matrix.failures.append(f"{target.name}:accept{suffix}")
        return
    workdir = build_root / "acceptance/work" / (target.name + suffix)
    evidence = build_root / "acceptance/evidence" / (target.name + suffix)
    runner_path = runner if runner is not None else host_build / "idis_host_runner"
    if any(cases.glob("test_*.py")):
        matrix.run(f"{target.name}:accept{suffix}",
                   [sys.executable, str(root / "tests/acceptance/run.py"),
                    "--runner", str(runner_path),
                    "--cases", str(cases),
                    "--workdir", str(workdir),
                    "--evidence-dir", str(evidence)])
    for script in sorted(acceptance.glob("*.sh")):
        matrix.run(f"{target.name}:accept{suffix}:{script.stem}",
                   ["bash", str(script), str(runner_path)])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    root = args.root.resolve()
    matrix = Matrix(root)

    print("== host tests ==")
    build_root = root / "build"
    host_build = build_root / "ci-host"
    matrix.run("host:configure", ["cmake", "-S", str(root / "tests"),
                                 "-B", str(host_build), "-G", "Ninja"])
    matrix.run("host:build", ["cmake", "--build", str(host_build)])
    matrix.run("host:test", ["ctest", "--test-dir", str(host_build),
                             "--output-on-failure"])

    strict_build = build_root / "ci-strict"
    matrix.run("host:strict-configure", ["cmake", "-S", str(root / "tests"),
                                         "-B", str(strict_build), "-G", "Ninja",
                                         "-DCMAKE_BUILD_TYPE=Debug",
                                         "-DBLADE_BUILD_SIM=OFF",
                                         f"-DCMAKE_C_FLAGS={STRICT_FLAGS}",
                                         "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"])
    matrix.run("host:strict-build", ["cmake", "--build", str(strict_build)])
    matrix.run("host:strict-test", ["ctest", "--test-dir", str(strict_build),
                                    "--output-on-failure"])

    analyzer_build = build_root / "ci-analyzer"
    matrix.run("host:analyzer-configure", ["cmake", "-S", str(root / "tests"),
                                           "-B", str(analyzer_build), "-G", "Ninja",
                                           "-DCMAKE_BUILD_TYPE=Debug",
                                           "-DBLADE_BUILD_SIM=OFF",
                                           f"-DCMAKE_C_FLAGS={ANALYZER_FLAGS}"])
    matrix.run("host:analyzer", ["cmake", "--build", str(analyzer_build),
                                 "--target", *ANALYZER_TARGETS])

    conversion_build = build_root / "ci-conversion"
    matrix.run("host:conversion-configure", ["cmake", "-S", str(root / "tests"),
                                             "-B", str(conversion_build), "-G", "Ninja",
                                             "-DCMAKE_BUILD_TYPE=Debug",
                                             "-DBLADE_BUILD_SIM=OFF",
                                             f"-DCMAKE_C_FLAGS={CONVERSION_FLAGS}"])
    conversion_log = build_root / "conversion.log"
    matrix.run("host:conversion-build", ["bash", "-c",
        "cmake --build " + shlex.quote(str(conversion_build))
        + " --clean-first --target " + " ".join(ANALYZER_TARGETS)
        + " > " + shlex.quote(str(conversion_log)) + " 2>&1"])
    matrix.run("check:conversion", [sys.executable,
                                    str(root / "tools/check_conversion.py"),
                                    "--log", str(conversion_log)])

    sim_build = build_root / "ci-sim-san"
    matrix.run("host:sim-san-configure", ["cmake", "-S", str(root / "tests"),
                                          "-B", str(sim_build), "-G", "Ninja",
                                          "-DCMAKE_BUILD_TYPE=Debug",
                                          f"-DCMAKE_C_FLAGS={SIM_SANITIZER_FLAGS}",
                                          "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"])
    matrix.run("host:sim-san-build", ["cmake", "--build", str(sim_build),
                                      "--target", "firmware_sim",
                                      "idis_host_runner"])
    matrix.run("host:sim-san-test", ["ctest", "--test-dir", str(sim_build),
                                     "-R", "firmware_sim|firmware_acceptance",
                                     "--output-on-failure"])
    for name in ("idis_ip_meter", "idis_ip_meter_freertos"):
        run_target_acceptance(matrix, root / "targets" / name, sim_build, root,
                              build_root, suffix="-san",
                              runner=sim_build / "idis_host_runner")
    matrix.run("host:sim-san-fuzz", [sys.executable,
                                     str(root / "tools/fuzz_idis_eeprom.py"),
                                     "--runner", str(sim_build / "idis_host_runner"),
                                     "--workdir", str(build_root / "acceptance/fuzz-idis"),
                                     "--stride", "512"])

    print("== shared checks ==")
    for label, command in [
        ("check:components", [sys.executable, str(root / "tools/check_components.py")]),
        ("check:component-reachability", [sys.executable, str(root / "tools/check_component_reachability.py")]),
        ("check:component-standard", [sys.executable, str(root / "tools/check_component_standard.py")]),
        ("check:board-modes", [str(root / "tools/check-board-modes.sh")]),
        ("check:domain", [sys.executable, str(root / "tools/check_domain_contracts.py")]),
        ("check:public-structs", [sys.executable, str(root / "tools/check_public_structs.py")]),
        ("check:rtos", [sys.executable, str(root / "tools/check_rtos_targets.py")]),
        ("check:rtos-model", [sys.executable, str(root / "tools/check_rtos_model.py")]),
        ("check:ids", [sys.executable, str(root / "tools/check_idis_ids.py")]),
        ("check:id-docs", [sys.executable, str(root / "tools/check_idis_docs.py")]),
        ("check:rom-budget-docs", [sys.executable, str(root / "tools/check_rom_budget_docs.py")]),
        ("check:doc-evidence", [sys.executable, str(root / "tools/check_doc_evidence.py")]),
        ("check:audit", [sys.executable, str(root / "tools/check_idis_audit.py"),
                         str(host_build / "idis_host_runner")]),
        ("check:acceptance-matrix", [sys.executable, str(root / "tools/check_acceptance_matrix.py")]),
        ("check:layers", [str(root / "tools/check-layers.sh")]),
        ("check:target-kernel", [sys.executable, str(root / "tools/check_target_kernel.py")]),
        ("check:idis-twins", [sys.executable, str(root / "tools/check_idis_twins.py")]),
        ("check:platform", [sys.executable, str(root / "tools/check_platform_boundary.py")]),
        ("check:drivers", [sys.executable, str(root / "tools/check_drivers.py")]),
        ("check:meter-record", [sys.executable, str(root / "tools/gen_meter_record.py"), "--check"]),
        ("check:param-tables", [sys.executable, str(root / "tools/check_param_tables.py"),
                                "--source", str(root / "targets/stategrid_meter/mapper_698/mapper_698.c"),
                                "--elf", str(host_build / "firmware_sim")]),
        ("check:ram-budget-counts", [sys.executable, str(root / "tools/check_ram_budget_counts.py")]),
        ("check:dead-code", [sys.executable, str(root / "tools/check_dead_code.py")]),
        ("check:mutable-globals", [sys.executable, str(root / "tools/check_mutable_globals.py")]),
        ("check:products", [sys.executable, str(root / "tools/product_manifest.py"), "--check-all"]),
    ]:
        matrix.check(label, command)

    targets = sorted(path for path in (root / "targets").iterdir() if path.is_dir())
    for target in targets:
        if target_kind(target) in ("skip", "host"):
            continue
        acceptance = target / "tests/acceptance"
        cases = acceptance_cases(target)
        if cases is None:
            print(f"[{target.name}:acceptance-entry] missing tests/acceptance")
            matrix.failures.append(f"{target.name}:acceptance-entry")
        elif not any(cases.glob("test_*.py")) and not any(acceptance.glob("*.sh")):
            print(f"[{target.name}:acceptance-entry] no test cases")
            matrix.failures.append(f"{target.name}:acceptance-entry")
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
        if (kind == "cmake"
                and (target / "board/board_transport.h").is_file()
                and target.name not in TRANSPORT_TARGETS):
            print(f"[{target.name}:transport-lane] add the target to TRANSPORT_TARGETS")
            matrix.failures.append(f"{target.name}:transport-lane")
        build = configure_build(matrix, target, root, build_root)
        if build is None:
            continue
        elf = firmware_elf(build, target.name)
        matrix.check(f"{target.name}:firmware", [sys.executable, str(root / "tools/check_idis_firmware.py"),
                                                "--self-check", str(elf)])
        if target.name in TRANSPORT_TARGETS:
            check_transport_symbols(matrix, f"{target.name}:poll-symbols", elf,
                                    irq=False)
        run_target_acceptance(matrix, target, host_build, root, build_root)

        if target.name in TRANSPORT_TARGETS:
            irq_build = configure_build(matrix, target, root, build_root,
                                        suffix="-irq", defines=IRQ_MODE_DEFINES)
            if irq_build is not None:
                irq_elf = firmware_elf(irq_build, target.name)
                matrix.check(f"{target.name}:firmware-irq",
                             [sys.executable, str(root / "tools/check_idis_firmware.py"),
                              "--self-check", str(irq_elf)])
                check_transport_symbols(matrix, f"{target.name}:irq-symbols",
                                        irq_elf, irq=True)
                print_rom(f"{target.name}:irq-rom", irq_elf)
        print(f"TARGET {target.name}: build=cmake unit=shared acceptance=target")

    # The link evidence is the build tree, so this runs after the target matrix.
    matrix.check("check:product-link",
                 [sys.executable,
                  str(root / "tools/check_product_component_usage.py")])

    if matrix.failures:
        print("== FAILED ==")
        for failure in matrix.failures:
            print(f"FAIL {failure}")
        return 1
    print("== all targets ok ==")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
