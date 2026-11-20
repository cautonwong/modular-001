#!/usr/bin/env bash
# Compile-check the core legacy target with arm-none-eabi-gcc.
#
set -uo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
includes=(
    -I "$root"
    -I "$root/components/core/types"
    -I "$root/targets/stategrid_meter"
    -I "$root/targets/stategrid_meter/app"
    -I "$root/targets/stategrid_meter/mapper_645"
    -I "$root/application/adapters/dl645"
    -I "$root/application/adapters/dl698"
    -I "$root/targets/stategrid_meter/board"
    -I "$root/infra/soc/v85xxp/v8500"
    -I "$root/infra/third_party/arm/cmsis"
    -I "$root/infra/soc/v85xxp/v8500/vendor"
    -I "$root/infra/soc/v85xxp/v8500/vendor/Lib_Driver/inc"
    -I "$root/domain/display/legacy"
    -I "$root/targets/stategrid_meter/tools/app_resource_tool"
    -I "$root/targets/stategrid_meter/app/apps"
)
# -w keeps vendor noise down. The explicit -Werror flag still promotes
# implicit declarations, which are defects even in legacy code.
flags=(-std=gnu11 -mcpu=cortex-m0 -mthumb -ffreestanding -w
       -Werror=implicit-function-declaration -fpermissive "${includes[@]}")

files=(
    components/core/types/utils.c
    targets/stategrid_meter/app/uart_timeout_adapter.c
    components/core/types/rtc.c
    targets/stategrid_meter/app/metering/legacy/energy.c
    targets/stategrid_meter/app/tariff/legacy/touFee.c
    targets/stategrid_meter/app/metering/runstatus.c
    targets/stategrid_meter/app/metering/static_checksum.c
    targets/stategrid_meter/tools/app_resource_tool/resource_tool.c
    targets/stategrid_meter/app/apps/app_meter_base_entry.c
    targets/stategrid_meter/app/metering_energy_adapter.c
    targets/stategrid_meter/persistence/legacy_energy_codec.c
    targets/stategrid_meter/persistence/legacy_energy_store.c
    targets/stategrid_meter/app/relay_control_adapter.c
    targets/stategrid_meter/app/record_storage_adapter.c
    application/ports/record_storage.c
    application/ports/resource_access.c
    targets/stategrid_meter/app/display_control_adapter.c
    targets/stategrid_meter/app/stategrid_meter_entry.c
    targets/stategrid_meter/app/stategrid_context.c
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
if ! python3 "$root/tools/check_externs.py"; then
    status=1
fi
exit "$status"
