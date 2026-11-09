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
    -I "$root/instances/stategrid_meter"
    -I "$root/instances/stategrid_meter/mapper_645"
    -I "$root/packages/dl645"
    -I "$root/packages/dl698"
    -I "$root/packages/drivers"
    -I "$root/packages/drivers/arch"
    -I "$root/packages/drivers/arch/cm0"
    -I "$root/packages/drivers/arch/v85xxp inc"
    -I "$root/packages/drivers/arch/v85xxp inc/Lib_Driver/inc"
    -I "$root/packages/meterbase"
    -I "$root/applications/app_resource_tool"
    -I "$root/applications/app_645_server"
    -I "$root/applications/app_698_server"
    -I "$root/applications/app_meter_base"
)
flags=(-std=gnu11 -mcpu=cortex-m0 -mthumb -ffreestanding -w "${includes[@]}")

files=(
    infra/kernel/utils.c
    infra/kernel/time_link.c
    infra/kernel/nidavellir.c
    infra/kernel/rtc.c
    packages/meterbase/energy.c
    packages/meterbase/touFee.c
    packages/meterbase/relay_control.c
    packages/meterbase/runstatus.c
    applications/app_resource_tool/resource_tool.c
    applications/app_meter_base/app_meter_base_entry.c
    instances/stategrid_meter/stategrid_meter_entry.c
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
exit "$status"
