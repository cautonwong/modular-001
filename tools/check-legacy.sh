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
    -I "$root/targets/stategrid_meter/mapper_645"
    -I "$root/application/adapters/dl645"
    -I "$root/application/adapters/dl698"
    -I "$root/infra/board"
    -I "$root/infra/soc"
    -I "$root/infra/soc/cm0"
    -I "$root/infra/soc/v85xxp inc"
    -I "$root/infra/soc/v85xxp inc/Lib_Driver/inc"
    -I "$root/domain/meterbase"
    -I "$root/application/apps/app_resource_tool"
    -I "$root/application/apps/app_645_server"
    -I "$root/application/apps/app_698_server"
    -I "$root/application/apps/app_meter_base"
)
flags=(-std=gnu11 -mcpu=cortex-m0 -mthumb -ffreestanding -w "${includes[@]}")

files=(
    infra/kernel/utils.c
    infra/kernel/time_link.c
    infra/kernel/nidavellir.c
    infra/kernel/rtc.c
    domain/meterbase/energy.c
    domain/meterbase/touFee.c
    domain/meterbase/relay_control.c
    domain/meterbase/runstatus.c
    application/apps/app_resource_tool/resource_tool.c
    application/apps/app_meter_base/app_meter_base_entry.c
    targets/stategrid_meter/stategrid_meter_entry.c
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
