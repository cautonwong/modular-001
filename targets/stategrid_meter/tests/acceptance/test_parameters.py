from __future__ import annotations

import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

try:
    from tests.acceptance import parameters, protocol, sim, storage
except ModuleNotFoundError:
    import parameters
    import protocol
    import sim
    import storage

REPO_ROOT = Path(__file__).resolve().parents[2]
MAPPER_C = REPO_ROOT / "targets/stategrid_meter/mapper_698/mapper_698.c"
SIM_BINARY = REPO_ROOT / "build" / "firmware_sim"
BOOT_WORK = REPO_ROOT / "build/acceptance/work/ACC-BOOT-001"
BLANK_EEPROM = b"\xff" * storage.EEPROM_SIZE
BLANK_SA = b"\xff" * 6
ENERGY_OAD = 0x00000401
METER_CONST_OAD = 0x41090200

# name -> (parameter_item[] row, sub-index within a `same` entry)
MAIN_SOURCE_NAMES = {
    "meter_const": ("Meter_Const1", 0),
    "season_count": ("SeasonDay_Para", 0),
    "day_table_count": ("SeasonDay_Para", 1),
    "day_period_count": ("SeasonDay_Para", 2),
    "fee_count": ("SeasonDay_Para", 3),
    "holiday_count": ("SeasonDay_Para", 4),
    "season_table_change": ("SeasonTable_Changetime", 0),
    "day_table_change": ("DayTable_Changetime", 0),
    "weekend": ("Weekend", 0),
    "weekend_fee": ("WeekendFee", 0),
    "hour_freeze_start": ("HourFroze_StartTime", 0),
    "hour_freeze_interval": ("HourFroze_Inv", 0),
    "day_freeze_time": ("DayFroze_Time", 0),
    "imax_protect_value": ("ImaxProtectValue", 0),
    "imax_protect_time": ("ImaxProtectTime", 0),
    "remote_relay_off_time": ("Remote_RelayOffTime", 0),
    "balance_time": ("Balance_Time", 0),
}


def _table_offsets(array: str) -> tuple[dict[str, tuple[int, int, int]], int]:
    """Recompute a parameter_item table exactly as supply_parameter_info walks it."""
    text = MAPPER_C.read_text(encoding="utf-8", errors="replace")
    body = re.search(rf"{array}\[\]\s*=\s*\{{(.*?)\n\}};", text, re.S).group(1)
    body = body.split("#else", 1)[0]  # parameter_item[] has a WIN32 stub
    body = re.sub(r"//[^\n]*", "", body)
    rows = re.findall(
        r"\{\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*([^,]+?)\s*,\s*[^,}]+\}", body
    )
    offsets: dict[str, tuple[int, int, int]] = {}
    offset = 0
    for name, same, length in rows:
        same = int(same, 0)
        unit = 10 if "sizeof" in length else int(length.strip(), 0)
        offsets[name] = (offset, unit, unit * same)
        offset += unit * same + 2
    return offsets, offset


def _base_image() -> bytes:
    fixture = sorted(BOOT_WORK.glob("run-*/eeprom.bin"))
    if fixture:
        return fixture[0].read_bytes()
    return BLANK_EEPROM


class LayoutTests(unittest.TestCase):
    def test_main_parameter_offsets_match_mapper_table(self):
        offsets, total = _table_offsets("parameter_item")
        self.assertEqual(total, 7428, "parameter block layout changed")
        for name, (source, index) in MAIN_SOURCE_NAMES.items():
            relative, unit, span = offsets[source]
            field = parameters.PARAMETERS[name].fields[0]
            self.assertEqual(
                field.offset,
                parameters.PARAMETER_BASE + relative + index * unit,
                f"{name} offset",
            )
            self.assertEqual(field.size, span if name == "balance_time" else unit, f"{name} size")
            self.assertEqual(
                field.entry, parameters.PARAMETER_BASE + relative, f"{name} entry"
            )
            self.assertEqual(field.entry_size, span, f"{name} entry size")

    def test_parameter_1_offset_matches_mapper_table(self):
        offsets, _ = _table_offsets("parameter_item_1")
        relative, unit, span = offsets["EC_SaveDiff_E"]
        main, backup = parameters.PARAMETERS["ec_save_diff"].fields
        self.assertEqual((main.offset, main.size), (relative, unit))
        self.assertEqual((main.entry, main.entry_size), (relative, span))
        self.assertEqual(
            (backup.offset, backup.entry),
            (
                parameters.PARAMETER_1_BACKUP_BASE + relative,
                parameters.PARAMETER_1_BACKUP_BASE + relative,
            ),
        )

    @unittest.skipUnless(shutil.which("gcc"), "gcc is not available")
    def test_static_layout_matches_firmware_header(self):
        source = r"""
#include <stddef.h>
#include <stdio.h>
#include "kernel/legacy_control_block.h"
#define P(name) printf("%s %zu\n", #name, offsetof(struct mbase_control_block_t, name))
int main(void) {
    P(static_section_start); P(static_section_end); P(static_chksum);
    P(tsa); P(meter_const);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "probe.c"
            exe = Path(tmp) / "probe"
            src.write_text(source, encoding="utf-8")
            command = [
                "gcc",
                "-std=gnu11",
                "-DWIN32",
                f"-I{REPO_ROOT}",
                f"-I{REPO_ROOT / 'nidavellir/include'}",
                f"-I{REPO_ROOT / 'targets/stategrid_meter'}",
                str(src),
                "-o",
                str(exe),
            ]
            completed = subprocess.run(command, capture_output=True, text=True, check=False)
            if completed.returncode != 0:
                self.fail(f"probe compile failed:\n{completed.stderr}")
            output = subprocess.run(
                [str(exe)], capture_output=True, text=True, check=True
            ).stdout
        probe = {
            key: int(value)
            for key, value in (line.split() for line in output.splitlines())
        }

        self.assertEqual(
            probe["static_section_end"] - probe["static_section_start"] + 1,
            storage.EEPROM_STATIC_BLOCK_SIZE,
        )
        self.assertEqual(probe["static_chksum"], parameters.STATIC_CHECKSUM_OFFSET)
        self.assertEqual(
            parameters.PARAMETERS["meter_address"].fields[0].offset,
            parameters.STATIC_BASE + probe["tsa"],
        )
        self.assertEqual(
            parameters.PARAMETERS["meter_const"].fields[1].offset,
            parameters.STATIC_BASE + probe["meter_const"],
        )


class SyntheticImageTests(unittest.TestCase):
    VALUES = {
        "meter_const": 3200,
        "hour_freeze_interval": 60,
        "season_count": 3,
        "fee_count": 8,
        "imax_protect_value": 300000,
        "ec_save_diff": 200,
        "weekend": 0xFE,
        "meter_address": b"\x11\x11\x11\x11\x11\x11",
    }

    def test_set_read_and_checksums_on_blank_image(self):
        image = parameters.set_parameters(BLANK_EEPROM, self.VALUES)
        self.assertEqual(len(image), storage.EEPROM_SIZE)
        self.assertIsInstance(image, bytes)

        self.assertEqual(image[8715:8719], (3200).to_bytes(4, "big"))
        self.assertEqual(
            image[8719:8721],
            storage.checksum16(image[8715:8719]).to_bytes(2, "big"),
        )
        self.assertEqual(image[9091], 60)
        self.assertEqual(
            image[9092:9094],
            storage.checksum16(image[9091:9092]).to_bytes(2, "big"),
        )
        self.assertEqual(image[7718:7723], bytes((3, 0xFF, 0xFF, 8, 0xFF)))
        self.assertEqual(
            image[7723:7725],
            storage.checksum16(image[7718:7723]).to_bytes(2, "big"),
        )
        self.assertEqual(image[194:196], (200).to_bytes(2, "little"))
        self.assertEqual(
            image[196:198],
            storage.checksum16(image[194:196]).to_bytes(2, "big"),
        )
        self.assertEqual(image[1218:1220], (200).to_bytes(2, "little"))
        self.assertEqual(image[17409:17415], b"\x11" * 6)
        self.assertTrue(
            parameters.verify_parameter_checksums(image)["meter_address"]
        )

        # Entries this call did not touch keep their blank bytes.
        for start, end in ((7700, 7707), (9084, 9089), (17415, 17421), (18186, 18190)):
            self.assertEqual(image[start:end], BLANK_EEPROM[start:end], (start, end))

        fields = storage.parameter_block(image)
        self.assertEqual(fields["meter_const"].value, 3200)
        self.assertEqual(fields["hour_freeze_interval"].value, 60)
        self.assertEqual(fields["meter_address"].value, b"\x11" * 6)

        read = parameters.read_parameters(image)
        for name, value in self.VALUES.items():
            self.assertEqual(read[name], value, name)
        status = parameters.verify_parameter_checksums(image)
        for name in self.VALUES:
            self.assertTrue(status[name], name)
        self.assertFalse(status["day_freeze_time"], "untouched entry looks valid")

    def test_real_fixture_round_trip(self):
        base = _base_image()
        values = {"meter_const": 3200, "hour_freeze_interval": 60}
        image = parameters.set_parameters(base, values)
        read = parameters.read_parameters(image)
        self.assertEqual(read["meter_const"], 3200)
        self.assertEqual(read["hour_freeze_interval"], 60)
        self.assertEqual(storage.parameter_block(image)["meter_const"].value, 3200)
        self.assertTrue(parameters.verify_parameter_checksums(image)["meter_const"])

    def test_corrupt_value_byte_fails_entry_checksum(self):
        image = bytearray(
            parameters.set_parameters(
                BLANK_EEPROM, {"meter_const": 3200, "hour_freeze_interval": 60}
            )
        )
        image[9091] ^= 0x01
        status = parameters.verify_parameter_checksums(bytes(image))
        self.assertFalse(status["hour_freeze_interval"])
        self.assertTrue(status["meter_const"])

    def test_unknown_name_and_bad_values_are_rejected(self):
        with self.assertRaises(KeyError):
            parameters.set_parameters(BLANK_EEPROM, {"not_a_parameter": 1})
        with self.assertRaises(ValueError):
            parameters.set_parameters(BLANK_EEPROM, {"hour_freeze_interval": 256})
        with self.assertRaises(ValueError):
            parameters.set_parameters(BLANK_EEPROM, {"meter_const": b"\x00\x00\x00"})
        with self.assertRaises(ValueError):
            parameters.set_parameters(BLANK_EEPROM, {"meter_address": 5})


class BootReadbackTests(unittest.TestCase):
    @unittest.skipUnless(SIM_BINARY.is_file(), "firmware_sim is not built")
    def test_two_cycle_boot_keeps_seeded_parameters(self):
        seeded = parameters.set_parameters(
            _base_image(), {"meter_const": 3200, "hour_freeze_interval": 60}
        )
        energy = protocol.build_frame(
            protocol.get_request(1, ENERGY_OAD), sa=BLANK_SA, ca=0x10, ctrl=0x43
        )
        constant = protocol.build_frame(
            protocol.get_request(2, METER_CONST_OAD), sa=BLANK_SA, ca=0x10, ctrl=0x43
        )
        scenario = (
            f"at 0 uart 0 {energy.hex()}\n"
            f"at 1 uart 0 {constant.hex()}\n"
        )

        with tempfile.TemporaryDirectory() as tmp:
            harness = sim.SimHarness(SIM_BINARY, REPO_ROOT)
            run = sim.SimContext(harness, Path(tmp)).run(
                cycles=2, seed_eeprom=seeded, scenario=scenario
            )

        self.assertEqual(run.exit_code, 0, run.stderr[-300:])
        self.assertEqual(len(run.cycles), 2)

        responses = {}
        for frame in protocol.iter_698_frames(run.uart_tx):
            result = protocol.parse_get_response(protocol.parse_frame(frame).apdu)
            responses[result.oad] = result.values()
        self.assertIn(ENERGY_OAD, responses, run.uart_tx.hex())
        self.assertEqual(responses[ENERGY_OAD][0].tag, protocol.AXDR_LONG64)
        self.assertIn(METER_CONST_OAD, responses, run.uart_tx.hex())
        self.assertEqual(responses[METER_CONST_OAD][0].value, 3200)

        read = parameters.read_parameters(run.eeprom)
        self.assertEqual(read["meter_const"], 3200)
        self.assertEqual(read["hour_freeze_interval"], 60)
        fields = storage.parameter_block(run.eeprom)
        self.assertEqual(fields["meter_const"].value, 3200)
        self.assertEqual(fields["hour_freeze_interval"].value, 60)
        status = parameters.verify_parameter_checksums(run.eeprom)
        self.assertTrue(status["meter_const"])
        self.assertTrue(status["hour_freeze_interval"])


if __name__ == "__main__":
    unittest.main()
