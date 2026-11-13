#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-/tmp/idis-prep-build}"
FW_BUILD="${2:-${BUILD}-firmware}"

cd "$ROOT"

echo "== host =="
cmake -S tests -B "$BUILD" -G Ninja
cmake --build "$BUILD"
ctest --test-dir "$BUILD" --output-on-failure
for acceptance in targets/idis_ip_meter/tests/acceptance/*.sh; do
    bash "$acceptance" "$BUILD/idis_host_runner"
done
python3 tools/check_idis_audit.py "$BUILD/idis_host_runner"
python3 tools/check_idis_ids.py
python3 tools/check_components.py
python3 tools/check_domain_contracts.py
python3 tools/check_rtos_targets.py
./tools/check-layers.sh

echo "== baremetal firmware =="
cmake -S targets/idis_ip_meter/app -B "$FW_BUILD" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/toolchains/arm-none-eabi.cmake"
cmake --build "$FW_BUILD"
python3 tools/check_idis_firmware.py "$FW_BUILD/idis.elf"

echo "== freertos firmware =="
FREERTOS_BUILD="${FW_BUILD}-freertos"
FREERTOS_KERNEL_DIR="${FREERTOS_KERNEL_DIR:-$ROOT/infra/third_party/freertos/FreeRTOS-KernelV11.3.3}"
cmake -S targets/idis_ip_meter_freertos/app -B "$FREERTOS_BUILD" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/toolchains/arm-none-eabi.cmake" \
    -DFREERTOS_KERNEL_DIR="$FREERTOS_KERNEL_DIR"
cmake --build "$FREERTOS_BUILD"
python3 tools/check_idis_firmware.py "$FREERTOS_BUILD/idis_freertos.elf"
