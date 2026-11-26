#!/usr/bin/env python3
"""Verify that the IDIS firmware actually reached the wired services.

The build uses --gc-sections, so compiling a translation unit is not proof
that its code is in the image.  This check fails when a service that the
product loop must run is absent from the linked ELF.
"""
import argparse
import os
import re
import subprocess
import sys
from collections import namedtuple

RAM_BASE = 0x20000000
RAM_BYTES = 64 * 1024
STACK_BYTES = 8 * 1024
ROM_BYTES = 103 * 1024
SHF_ALLOC = 0x2

Section = namedtuple("Section", "offset size vma flags")

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
    "nd_idis_profiles_release_legacy",
    "nd_maintenance_init",
    "nd_maintenance_feed",
    "board_ir_read",
    "nd_idis_profile_table_init",
    "nd_idis_script_execute",
    "nd_idis_script_load",
    "nd_idis_script_save",
    "nd_idis_image_load",
    "nd_idis_image_save",
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
    "board_key_event_take",
    "uart_chl_initialize",
    "ndv_timer_tick",
    "mcu_clear_watchdog",
]

# Link presence alone does not prove a service is driven: the class-7 release
# was linked but no firmware code called it. Each edge must be a direct BL in
# the product's per-second loop. on_second is inlined today, but a compiler
# change must not turn that into a false failure, so it is an allowed caller.
CALL_EDGES = [
    ("board_ir_read", ("main", "idis_app_tick", "on_second")),
    ("nd_maintenance_feed", ("main", "idis_app_tick", "on_second")),
    ("nd_cosem_port_release_legacy", ("maintenance_release", "main", "idis_app_tick")),
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

# The vector table is frozen by startup.c: 16 system entries plus 32 IRQs.
# The first word is the initial stack pointer; every handler entry carries
# the Thumb bit. A table reorder or a missing KEEP shows up here.
VECTOR_WORDS = {
    1: "Reset_Handler",
    2: "Default_Handler", 3: "Default_Handler", 4: "Default_Handler",
    5: "Default_Handler", 6: "Default_Handler", 7: "Default_Handler",
    8: "Default_Handler", 9: "Default_Handler", 10: "Default_Handler",
    11: "SVC_Handler",
    12: "Default_Handler",
    13: "Default_Handler",
    14: "PendSV_Handler",
    15: "SysTick_Handler",
    16: "PMU_IRQHandler",
    17: "RTC_IRQHandler",
    18: "U32K0_IRQHandler",
    19: "U32K1_IRQHandler",
    20: "I2C_IRQHandler",
    21: "SPI1_IRQHandler",
    22: "UART0_IRQHandler",
    23: "UART1_IRQHandler",
    24: "UART2_IRQHandler",
    25: "UART3_IRQHandler",
    26: "UART4_IRQHandler",
    27: "UART5_IRQHandler",
    28: "ISO78160_IRQHandler",
    29: "ISO78161_IRQHandler",
    30: "TMR0_IRQHandler",
    31: "TMR1_IRQHandler",
    32: "TMR2_IRQHandler",
    33: "TMR3_IRQHandler",
    34: "PWM0_IRQHandler",
    35: "PWM1_IRQHandler",
    36: "PWM2_IRQHandler",
    37: "PWM3_IRQHandler",
    38: "DMA_IRQHandler",
    39: "FLASH_IRQHandler",
    40: "ANA_IRQHandler",
    41: "Default_Handler",
    42: "Default_Handler",
    43: "SPI2_IRQHandler",
    44: "Default_Handler",
    45: "Default_Handler",
    46: "Default_Handler",
    47: "Default_Handler",
}


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


def startup_symbols(nm, elf):
    """Linker-defined bounds have no size column in nm -S output."""
    wanted = {"_sidata", "_sdata", "_edata", "_sbss", "_ebss", "_estack"}
    found = {}
    output = subprocess.check_output([nm, elf], text=True)
    for line in output.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] in wanted:
            try:
                found[parts[2]] = int(parts[0], 16)
            except ValueError:
                continue
    return found


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
        flags = int.from_bytes(data[header + 8:header + 12], "little")
        address = int.from_bytes(data[header + 12:header + 16], "little")
        offset = int.from_bytes(data[header + 16:header + 20], "little")
        size = int.from_bytes(data[header + 20:header + 24], "little")
        end = names.find(b"\0", name_offset)
        name = names[name_offset:end].decode()
        sections[name] = Section(offset, size, address, flags)
    return sections


OBJDUMP_SECTION = re.compile(
    r"^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+"
    r"([0-9a-fA-F]+)\s+([0-9a-fA-F]+)")


def load_address(objdump, elf, name):
    """objdump -h is the only tool here that prints a section's LMA."""
    output = subprocess.check_output([objdump, "-h", elf], text=True)
    for line in output.splitlines():
        match = OBJDUMP_SECTION.match(line)
        if match and match.group(1) == name:
            return int(match.group(4), 16)
    return None


def section_totals(sections):
    def size(name):
        section = sections.get(name)
        return section.size if section else 0

    exidx = sum(section.size for name, section in sections.items()
                if name.startswith(".ARM.exidx"))
    return (size(".isr_vector"), size(".text"), size(".data"),
            size(".bss"), exidx)


def rom_load_span(sections, data_lma):
    """The LMA range objcopy writes to .bin: section sizes plus alignment.

    Summing section sizes misses the alignment padding between sections, and
    the linker's `_sidata + SIZEOF(.data)` ROM assert and the flashed image
    see the span instead. .isr_vector, .text and .ARM.exidx load at their VMA;
    .data loads at _sidata.
    """
    if data_lma is None:
        return None
    ends = [section.vma + section.size for name, section in sections.items()
            if name == ".isr_vector" or name.startswith(".text")
            or name.startswith(".ARM.exidx")]
    data = sections.get(".data")
    if data is not None:
        ends.append(data_lma + data.size)
    if not ends:
        return None
    return max(ends)


def check_ram_sections(sections):
    """Any alloc section mapped into RAM must join the RAM accounting."""
    stray = [f"{name} at 0x{section.vma:08x}"
             for name, section in sections.items()
             if name not in (".data", ".bss")
             and section.flags & SHF_ALLOC
             and RAM_BASE <= section.vma < RAM_BASE + RAM_BYTES]
    if not stray:
        return []
    return ["RAM sections outside the .data/.bss accounting: "
            + ", ".join(stray)
            + "; extend check_idis_firmware.py instead of dropping them"]


def check_startup_symbols(symbols, sections, data_lma):
    """Reset_Handler copies .data and zeroes .bss using these bounds.

    The bounds are anchored to the section VMAs and the .data load address,
    so shifting all of them together cannot pass as a consistent copy.
    """
    failures = []
    missing = [name for name in
               ("_sidata", "_sdata", "_edata", "_sbss", "_ebss", "_estack")
               if name not in symbols]
    if missing:
        return ["missing linker symbols: " + ", ".join(missing)]
    data = sections.get(".data", Section(0, 0, 0, 0))
    bss = sections.get(".bss", Section(0, 0, 0, 0))
    if data_lma is None:
        return ["objdump found no .data load address"]
    anchors = [
        ("_sdata", data.vma, ".data VMA"),
        ("_edata", data.vma + data.size, ".data VMA + size"),
        ("_sbss", bss.vma, ".bss VMA"),
        ("_ebss", bss.vma + bss.size, ".bss VMA + size"),
        ("_sidata", data_lma, ".data LMA"),
    ]
    for name, expected, source in anchors:
        if symbols[name] != expected:
            failures.append(f"{name} is 0x{symbols[name]:08x}, expected "
                            f"0x{expected:08x} ({source})")
    if symbols["_edata"] > symbols["_sbss"]:
        failures.append(f".data ends at 0x{symbols['_edata']:08x} but .bss "
                        f"starts at 0x{symbols['_sbss']:08x}")
    elif symbols["_sbss"] - symbols["_edata"] > 8:
        failures.append(f"alignment gap between .data and .bss is "
                        f"{symbols['_sbss'] - symbols['_edata']} bytes")
    if symbols["_sidata"] >= ROM_BYTES:
        failures.append(f"_sidata 0x{symbols['_sidata']:08x} is outside ROM")
    if symbols["_sidata"] + data.size > ROM_BYTES:
        failures.append("the .data load image runs past the ROM budget")
    if symbols["_estack"] != RAM_BASE + RAM_BYTES:
        failures.append(f"_estack 0x{symbols['_estack']:08x} is not the top "
                        "of RAM")
    if symbols["_ebss"] + STACK_BYTES > symbols["_estack"]:
        failures.append(f"_ebss 0x{symbols['_ebss']:08x} plus the "
                        f"{STACK_BYTES}-byte stack runs past _estack")
    return failures


def check_vector_table(elf, sections, table, lma):
    """The vector table must map and load at address 0: 48 frozen entries."""
    section = sections.get(".isr_vector")
    if section is None:
        return ["missing .isr_vector section"]
    if section.vma != 0:
        return [f".isr_vector VMA is 0x{section.vma:08x}; the vector table "
                "must map at address 0 or the image cannot boot"]
    if lma is None:
        return ["objdump found no .isr_vector load address"]
    if lma != 0:
        return [f".isr_vector LMA is 0x{lma:08x}; the vector table must load "
                "at address 0 or the boot vector reads the wrong data"]
    offset, size = section.offset, section.size
    if size != 4 * 48:
        return [f".isr_vector is {size} bytes, expected 192 "
                "(update VECTOR_WORDS and startup.c together)"]
    data = open(elf, "rb").read()
    words = [int.from_bytes(data[offset + 4 * index:offset + 4 * index + 4],
                            "little") for index in range(48)]
    failures = []
    stack = RAM_BASE + RAM_BYTES
    if words[0] != stack:
        failures.append(f"vector[0] is 0x{words[0]:08x}, "
                        f"expected _estack 0x{stack:08x}")
    if words[0] % 8 != 0:
        failures.append(f"initial SP 0x{words[0]:08x} is not 8-byte aligned")
    if not RAM_BASE <= words[0] <= RAM_BASE + RAM_BYTES:
        failures.append(f"initial SP 0x{words[0]:08x} is outside RAM")
    for index, name in VECTOR_WORDS.items():
        symbol = table.get(name)
        if symbol is None:
            failures.append(f"missing {name} symbol")
            continue
        expected = symbol[0] | 1
        if words[index] != expected:
            failures.append(f"vector[{index}] is 0x{words[index]:08x}, "
                            f"expected {name} 0x{expected:08x}")
        if words[index] & 1 == 0:
            failures.append(f"vector[{index}] has no Thumb bit")
        if words[index] >= ROM_BYTES:
            failures.append(f"vector[{index}] 0x{words[index]:08x} "
                            "is outside ROM")
    return failures


def call_edges(objdump, elf):
    output = subprocess.check_output(
        [objdump, "-d", "--no-show-raw-insn", elf], text=True)
    header = re.compile(r"^[0-9a-f]+ <([^>]+)>:$")
    call = re.compile(r"\bbl\s+[0-9a-f]+ <([^>]+)>")
    edges = set()
    caller = None
    for line in output.splitlines():
        match = header.match(line.strip())
        if match:
            caller = match.group(1)
            continue
        match = call.search(line)
        if match and caller is not None:
            edges.add((caller, match.group(1)))
    return edges


def self_check(elf, nm, objdump, sections, table):
    """The reviewer's repro: a consistent-looking shift must be rejected."""
    failures = []
    data_lma = load_address(objdump, elf, ".data")
    vector_lma = load_address(objdump, elf, ".isr_vector")
    moved = dict(sections)
    moved[".isr_vector"] = moved[".isr_vector"]._replace(vma=0x17D1C)
    if not check_vector_table(elf, moved, table, vector_lma):
        failures.append("a .isr_vector mapped at 0x17d1c was accepted")
    if not check_vector_table(elf, sections, table, 0x17D1C):
        failures.append("a .isr_vector loaded at 0x17d1c was accepted")
    symbols = startup_symbols(nm, elf)
    shifted = dict(symbols)
    shifted["_sidata"] += 4
    if not check_startup_symbols(shifted, sections, data_lma):
        failures.append("_sidata + 4 was accepted")
    shifted = dict(symbols)
    for name in ("_sdata", "_edata", "_sbss", "_ebss"):
        shifted[name] += 16
    if not check_startup_symbols(shifted, sections, data_lma):
        failures.append("all four bounds + 16 was accepted")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf")
    parser.add_argument("--self-check", action="store_true",
                        help="reject the moved-table and shifted-bound repros")
    args = parser.parse_args()
    nm = os.environ.get("NM", "arm-none-eabi-nm")
    objdump = os.environ.get("OBJDUMP", "arm-none-eabi-objdump")
    rows = nm_rows(nm, args.elf)
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

    sections = elf_sections(args.elf)
    vector_lma = load_address(objdump, args.elf, ".isr_vector")
    vector_failures = check_vector_table(args.elf, sections, table, vector_lma)
    if vector_failures:
        print("vector table: " + "; ".join(vector_failures))
        return 1
    data_lma = load_address(objdump, args.elf, ".data")
    startup_failures = check_startup_symbols(
        startup_symbols(nm, args.elf), sections, data_lma)
    if startup_failures:
        print("startup symbols: " + "; ".join(startup_failures))
        return 1
    ram_failures = check_ram_sections(sections)
    if ram_failures:
        print("; ".join(ram_failures))
        return 1

    edges = call_edges(objdump, args.elf)
    unwired = [f"{'/'.join(callers)} -> {callee}"
               for callee, callers in CALL_EDGES
               if not any((caller, callee) in edges for caller in callers)]
    if unwired:
        print("missing firmware call edges: " + ", ".join(unwired))
        return 1

    vector, text, data, bss, exidx = section_totals(sections)
    ram = bss + data + STACK_BYTES
    sections_rom = vector + text + data + exidx
    rom = rom_load_span(sections, data_lma)
    if rom is None:
        print("objdump found no ROM load addresses")
        return 1
    if ram > RAM_BYTES:
        print(f"RAM overflow: {ram} > {RAM_BYTES}")
        return 1
    if rom > ROM_BYTES:
        print(f"ROM overflow: {rom} > {ROM_BYTES}")
        return 1

    print(f"wired symbols ok ({len(REQUIRED_SYMBOLS)} services, "
          f"{len(IRQ_HANDLERS)} IRQ handlers)")
    print(f"ram={ram} bytes (bss={bss} data={data} stack={STACK_BYTES}) "
          f"rom={rom} bytes (load span; section sizes {sections_rom}, "
          f"alignment pad {rom - sections_rom}: vector={vector} text={text} "
          f"data={data} exidx={exidx})")

    if args.self_check:
        probes = self_check(args.elf, nm, objdump, sections, table)
        if probes:
            print("self-check: " + "; ".join(probes))
            return 1
        print("self-check ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
