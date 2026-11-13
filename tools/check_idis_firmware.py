#!/usr/bin/env python3
"""Verify that the IDIS firmware actually reached the wired services.

The build uses --gc-sections, so compiling a translation unit is not proof
that its code is in the image.  This check fails when a service that the
product loop must run is absent from the linked ELF.
"""
import os
import subprocess
import sys

RAM_BYTES = 64 * 1024
STACK_BYTES = 8 * 1024
ROM_BYTES = 100 * 1024

REQUIRED_SYMBOLS = [
    "nd_idis_event_init",
    "nd_idis_event_raise",
    "nd_idis_event_save",
    "nd_idis_pq_init",
    "nd_idis_pq_on_second",
    "nd_idis_mbus_init",
    "nd_idis_mbus_on_second",
    "nd_idis_calendar_init",
    "nd_idis_calendar_tick",
    "nd_push_on_second",
    "nd_idis_average_init",
    "nd_idis_average_on_second",
    "nd_idis_scheduler_on_second_multi",
    "nd_idis_single_action_on_second",
    "nd_idis_monitor_on_second",
    "nd_idis_limiter_on_second",
    "nd_idis_profiles_on_second",
    "nd_idis_profile_table_init",
    "nd_idis_script_execute",
    "nd_idis_script_load",
    "nd_idis_script_save",
    "nd_idis_image_load",
    "nd_idis_image_save",
    "nd_idis_profiles_sync",
    "nd_profile_store_sync",
    "nd_idis_setup_load",
    "nd_idis_setup_save",
    "nd_idis_energy_load",
    "nd_idis_energy_save",
    "nd_ic_demand_load",
    "nd_ic_demand_save",
    "nd_ic_demand_action",
    "nd_ic_extended_load",
    "nd_ic_extended_save",
    "nd_ic_disconnect_set",
    "nd_ic_data_get",
    "nd_ic_mbus_client_get",
    "board_adc_read",
    "board_flash_port",
    "board_measurement_read",
    "read_mcu_rtc",
    "write_mcu_rtc",
    "board_uart_init",
    "board_gpio_init",
    "board_rtc_init",
    "board_key_event_take",
    "uart_chl_initialize",
    "ndv_timer_tick",
    "mcu_clear_watchdog",
]

IRQ_HANDLERS = [
    "PMU_IRQHandler",
    "RTC_IRQHandler",
    "UART0_IRQHandler",
    "UART1_IRQHandler",
    "UART2_IRQHandler",
    "UART3_IRQHandler",
    "UART4_IRQHandler",
    "UART5_IRQHandler",
    "TMR0_IRQHandler",
    "TMR1_IRQHandler",
    "TMR2_IRQHandler",
    "TMR3_IRQHandler",
    "DMA_IRQHandler",
]


def nm_rows(nm, elf):
    output = subprocess.check_output(
        [nm, "-S", "--radix=d", elf], text=True)
    rows = []
    for line in output.splitlines():
        parts = line.split()
        if len(parts) < 4:
            continue
        try:
            address = int(parts[0])
            size = int(parts[1])
        except ValueError:
            continue
        rows.append((parts[3], address, size, parts[2]))
    return rows


def symbol_table(rows):
    return {name: (address, size, kind)
            for name, address, size, kind in rows}


def section_totals(rows):
    text = rodata = data = bss = 0
    for _, _, size, kind in rows:
        if kind in ("T", "t"):
            text += size
        elif kind in ("R", "r"):
            rodata += size
        elif kind in ("D", "d"):
            data += size
        elif kind in ("B", "b"):
            bss += size
    return text, rodata, data, bss


def main():
    if len(sys.argv) != 2:
        print("usage: check_idis_firmware.py <idis.elf>")
        return 2
    nm = os.environ.get("NM", "arm-none-eabi-nm")
    rows = nm_rows(nm, sys.argv[1])
    table = symbol_table(rows)

    missing = [name for name in REQUIRED_SYMBOLS if name not in table]
    if "ndv_timer_init" not in table and "ndv_timer_reset" not in table:
        missing.append("ndv_timer_init/ndv_timer_reset")
    missing += [name for name in IRQ_HANDLERS if name not in table]
    if missing:
        print("missing wired symbols: " + ", ".join(missing))
        return 1

    vector = table.get("vector_table")
    if vector is None or vector[1] != 192:
        print("vector_table missing or not 48 entries")
        return 1

    text, rodata, data, bss = section_totals(rows)
    ram = bss + data + STACK_BYTES
    rom = text + rodata + data
    if ram > RAM_BYTES:
        print(f"RAM overflow: {ram} > {RAM_BYTES}")
        return 1
    if rom > ROM_BYTES:
        print(f"ROM overflow: {rom} > {ROM_BYTES}")
        return 1

    print(f"wired symbols ok ({len(REQUIRED_SYMBOLS)} services, "
          f"{len(IRQ_HANDLERS)} IRQ handlers)")
    print(f"ram={ram} bytes (bss={bss} data={data} stack={STACK_BYTES}) "
          f"rom={rom} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
