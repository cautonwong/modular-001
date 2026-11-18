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
ROM_BYTES = 103 * 1024

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


def elf_sections(path):
    data = open(path, "rb").read()
    if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 1:
        raise ValueError(f"{path}: not a 32-bit little-endian ELF")
    shoff = int.from_bytes(data[0x20:0x24], "little")
    shentsize = int.from_bytes(data[0x2E:0x30], "little")
    shnum = int.from_bytes(data[0x30:0x32], "little")
    shstrndx = int.from_bytes(data[0x32:0x34], "little")

    names_header = shoff + shstrndx * shentsize
    names_offset = int.from_bytes(data[names_header + 16:names_header + 20],
                                  "little")
    names_size = int.from_bytes(data[names_header + 20:names_header + 24],
                                "little")
    names = data[names_offset:names_offset + names_size]

    sections = {}
    for index in range(shnum):
        header = shoff + index * shentsize
        name_offset = int.from_bytes(data[header:header + 4], "little")
        size = int.from_bytes(data[header + 20:header + 24], "little")
        end = names.find(b"\0", name_offset)
        name = names[name_offset:end].decode()
        sections[name] = size
    return sections


def section_totals(sections):
    text = sections.get(".text", 0)
    vector = sections.get(".isr_vector", 0)
    data = sections.get(".data", 0)
    bss = sections.get(".bss", 0)
    return vector, text, data, bss


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

    vector, text, data, bss = section_totals(elf_sections(sys.argv[1]))
    ram = bss + data + STACK_BYTES
    rom = vector + text + data
    if ram > RAM_BYTES:
        print(f"RAM overflow: {ram} > {RAM_BYTES}")
        return 1
    if rom > ROM_BYTES:
        print(f"ROM overflow: {rom} > {ROM_BYTES}")
        return 1

    print(f"wired symbols ok ({len(REQUIRED_SYMBOLS)} services, "
          f"{len(IRQ_HANDLERS)} IRQ handlers)")
    print(f"ram={ram} bytes (bss={bss} data={data} stack={STACK_BYTES}) "
          f"rom={rom} bytes (vector={vector} text={text} data={data})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
