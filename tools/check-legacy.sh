#!/usr/bin/env bash
# Compile-check the core legacy target with arm-none-eabi-gcc.
#
# infra/kernel/resource.c is excluded: its enums are declared after the
# structs that use them, so it cannot compile as written upstream.
set -uo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
includes=(
    -I "$root"
    -I "$root/nidavellir/include"
    -I "$root/nidavellir/include/kernel"
    -I "$root/targets/stategrid_meter"
    -I "$root/targets/stategrid_meter/app"
    -I "$root/targets/stategrid_meter/mapper_645"
    -I "$root/application/adapters/dl645"
    -I "$root/application/adapters/dl698"
    -I "$root/infra/boards/stategrid_single_phase"
    -I "$root/infra/soc/v85xxp/v8500"
    -I "$root/infra/third_party/arm/cmsis"
    -I "$root/infra/soc/v85xxp/v8500/vendor"
    -I "$root/infra/soc/v85xxp/v8500/vendor/Lib_Driver/inc"
    -I "$root/domain/metering/legacy"
    -I "$root/domain/tariff/legacy"
    -I "$root/domain/events/legacy"
    -I "$root/domain/freeze/legacy"
    -I "$root/domain/load_control/legacy"
    -I "$root/domain/display/legacy"
    -I "$root/targets/stategrid_meter/tools/app_resource_tool"
    -I "$root/targets/stategrid_meter/app/apps/legacy/app_645_server"
    -I "$root/targets/stategrid_meter/app/apps/legacy/app_698_server"
    -I "$root/targets/stategrid_meter/app/apps"
)
# -fpermissive keeps GCC 14 from promoting the old code's implicit
# declarations to errors; GCC 10 only warned.
flags=(-std=gnu11 -mcpu=cortex-m0 -mthumb -ffreestanding -w -fpermissive "${includes[@]}")

files=(
    infra/kernel/utils.c
    targets/stategrid_meter/app/rtos/time_link.c
    targets/stategrid_meter/app/rtos/nidavellir.c
    infra/kernel/rtc.c
    targets/stategrid_meter/app/metering/legacy/energy.c
    targets/stategrid_meter/app/tariff/legacy/touFee.c
    targets/stategrid_meter/app/load_control/legacy/relay_control.c
    targets/stategrid_meter/app/metering/legacy/runstatus.c
    targets/stategrid_meter/tools/app_resource_tool/resource_tool.c
    targets/stategrid_meter/app/apps/app_meter_base_entry.c
    targets/stategrid_meter/app/metering_energy_adapter.c
    targets/stategrid_meter/app/relay_control_adapter.c
    targets/stategrid_meter/app/stategrid_meter_entry.c
    targets/stategrid_meter/app/tariff_lookup.c
    targets/stategrid_meter/app/tariff_lookup_eeprom.c
)

errors="$(mktemp)"
status=0
for rel in "${files[@]}"; do
    if arm-none-eabi-gcc "${flags[@]}" -c "$root/$rel" -o /dev/null 2>"$errors"; then
        echo "OK   $rel"
    else
        echo "FAIL $rel"
        head -3 "$errors"
        status=1
    fi
done
rm -f "$errors"
if ! python3 "$root/tools/check_components.py"; then
    status=1
fi
if ! python3 "$root/tools/check_platform_boundary.py"; then
    status=1
fi
exit "$status"
