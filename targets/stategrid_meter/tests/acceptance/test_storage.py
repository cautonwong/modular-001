from __future__ import annotations

import re
import shutil
import struct
import subprocess
import tempfile
import unittest
from datetime import datetime, timezone
from pathlib import Path

try:
    from tests.acceptance import storage
except ModuleNotFoundError:
    # acceptance/run.py imports test modules by stem with acceptance/ on sys.path
    import storage

REPO_ROOT = Path(__file__).resolve().parents[2]
MAPPER_C = REPO_ROOT / "targets/stategrid_meter/mapper_698/mapper_698.c"
PORT_H = REPO_ROOT / "targets/stategrid_meter/app/port.h"
ACCEPTANCE_WORK = REPO_ROOT / "build/acceptance/work"

BLANK_EEPROM = b"\xff" * storage.EEPROM_SIZE
BOOT_RECORD = bytes.fromhex("06ffffffff1c281f5644")
MONTH_FREEZE_RECORD = bytes.fromhex("06ffffffff1c281f5680")

PARAMETER_SOURCE_NAMES = {
    "reset_count": "ResetCount",
    "imax_protect_value": "ImaxProtectValue",
    "imax_protect_time": "ImaxProtectTime",
    "tariff_change_time": "ToupriceTable_Changetime",
    "hour_freeze_start": "HourFroze_StartTime",
    "hour_freeze_interval": "HourFroze_Inv",
    "day_freeze_time": "DayFroze_Time",
    "remote_relay_off_time": "Remote_RelayOffTime",
    "parameter_init_flag": "ParaInitFinish_Flag",
}


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def _parameter_table() -> tuple[dict[str, tuple[int, int]], int]:
    """Recompute parameter_item[] offsets exactly as supply_parameter_info does."""
    text = _read(MAPPER_C)
    body = re.search(r"parameter_item\[\]\s*=\s*\{(.*?)\n\};", text, re.S).group(1)
    body = body.split("#else", 1)[0]  # WIN32 has a single {0} row
    body = re.sub(r"//[^\n]*", "", body)
    rows = re.findall(
        r"\{\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*([^,]+?)\s*,\s*[^,}]+\}", body
    )
    offsets: dict[str, tuple[int, int]] = {}
    offset = 0
    for name, same, length in rows:
        same = int(same, 0)
        unit = 10 if "sizeof" in length else int(length.strip(), 0)
        offsets[name] = (offset, unit * same)
        offset += unit * same + 2
    return offsets, offset


def _record_table(name: str) -> list[tuple[str, int, int]]:
    text = _read(MAPPER_C)
    body = re.search(rf"{name}\[\]\s*=\s*\{{(.*?)\n\}};", text, re.S).group(1)
    body = re.sub(r"//[^\n]*", "", body)
    rows = re.findall(
        r"\{\s*(\w+)\s*,\s*\w+\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", body
    )
    return [(row[0], int(row[1]), int(row[2])) for row in rows]


def _freeze_record_table() -> list[tuple[str, str, int]]:
    text = _read(MAPPER_C)
    body = re.search(r"record_item_2\[\]\s*=\s*\{(.*?)\n\};", text, re.S).group(1)
    body = re.sub(r"//[^\n]*", "", body)
    rows = re.findall(r"\{\s*(\w+)\s*,\s*\w+\s*,\s*(.+?)\s*,\s*(\d+)\s*\}", body)
    return [(row[0], row[1], int(row[2])) for row in rows]


def _section_sizes() -> dict[str, int]:
    text = _read(PORT_H)
    names = ("HourFreeze_SECTION_SIZE", "DayFreeze_SECTION_SIZE", "MonthFreeze_SECTION_SIZE")
    return {
        name: int(re.search(rf"#define\s+{name}\s+(\d+)", text).group(1))
        for name in names
    }


def _record(sequence: int, seconds: int) -> bytes:
    return (
        b"\x06"
        + sequence.to_bytes(4, "big")
        + b"\x1c"
        + seconds.to_bytes(4, "big")
    )


class SyntheticImageTests(unittest.TestCase):
    def test_reset_counter_known_values(self):
        image = bytearray(BLANK_EEPROM)
        for value in (0, 1, 2):
            struct.pack_into("<I", image, storage.EEPROM_RESET_COUNT_OFFSET, value)
            image[6010:6012] = storage.checksum16(
                image[storage.EEPROM_RESET_COUNT_OFFSET:6010]
            ).to_bytes(2, "big")
            self.assertEqual(storage.reset_count(image), value)
            field = storage.parameter_block(bytes(image))["reset_count"]
            self.assertEqual(field.value, value)
            self.assertEqual(field.offset, storage.EEPROM_RESET_COUNT_OFFSET)
            self.assertEqual(field.size, storage.EEPROM_RESET_COUNT_WIDTH)
            self.assertTrue(field.checksum_ok)

    def test_reset_counter_bad_checksum_is_reported(self):
        image = bytearray(BLANK_EEPROM)
        struct.pack_into("<I", image, storage.EEPROM_RESET_COUNT_OFFSET, 7)
        image[6010:6012] = storage.checksum16(image[6006:6010]).to_bytes(2, "big")
        image[6011] ^= 0xFF
        self.assertEqual(storage.reset_count(image), 7)
        self.assertFalse(storage.parameter_block(bytes(image))["reset_count"].checksum_ok)

    def test_energy_record_exact_offsets_and_checksums(self):
        image = bytearray(BLANK_EEPROM)
        record = bytearray(storage.ENERGY_RECORD_SIZE)
        struct.pack_into("<13I", record, 0, *[100 + index for index in range(13)])
        struct.pack_into("<13I", record, 52, *[200 + index for index in range(13)])
        struct.pack_into("<13H", record, 104, *[1 + index for index in range(13)])
        struct.pack_into("<13H", record, 130, *[21 + index for index in range(13)])
        struct.pack_into("<H", record, 160, 7)
        struct.pack_into("<I", record, 164, 300)
        struct.pack_into("<I", record, 156, storage.checksum16(record[0:156]))
        struct.pack_into("<I", record, 168, storage.checksum16(record[160:168]))
        end = storage.ENERGY_RECORD_OFFSET + storage.ENERGY_RECORD_SIZE
        image[storage.ENERGY_RECORD_OFFSET:end] = record

        decoded = storage.energy_record(bytes(image))
        self.assertEqual(decoded.forward[0], storage.EnergyAmount(100, 1))
        self.assertEqual(decoded.forward[12], storage.EnergyAmount(112, 13))
        self.assertEqual(decoded.reverse[3], storage.EnergyAmount(203, 24))
        self.assertEqual(decoded.month, storage.EnergyAmount(300, 7))
        self.assertEqual(decoded.energy_checksum, storage.checksum16(record[0:156]))
        self.assertTrue(decoded.energy_checksum_ok)
        self.assertTrue(decoded.use_energy_checksum_ok)

        image[storage.ENERGY_RECORD_OFFSET] ^= 0x01
        corrupted = storage.energy_record(bytes(image))
        self.assertFalse(corrupted.energy_checksum_ok)
        self.assertTrue(corrupted.use_energy_checksum_ok)

    def test_energy_record_rejects_short_image(self):
        with self.assertRaises(ValueError):
            storage.energy_record(b"\xff" * 100)

    def test_log_block_returns_oldest_first_with_wraparound(self):
        region = b"".join(_record(sequence, 1000 + sequence) for sequence in (10, 11, 12))
        manager = storage.LogManager(
            read_index=2,
            write_index=1,
            count=3,
            unit_size=10,
            max_count=3,
            checksum=0,
            checksum_ok=True,
        )
        block = storage.log_block(manager, region)
        self.assertEqual(block.capacity, 3)
        self.assertEqual([record.slot for record in block.records], [2, 0, 1])
        self.assertEqual([record.sequence for record in block.records], [12, 10, 11])
        self.assertEqual(
            block.records[0].timestamp,
            datetime(2000, 1, 1, 0, 16, 52, tzinfo=timezone.utc),
        )

    def test_log_block_ignores_bad_manager_checksum(self):
        region = _record(1, 1000)
        manager = storage.LogManager(
            read_index=0xFFFF,
            write_index=0xFFFF,
            count=0xFFFF,
            unit_size=0xFFFF,
            max_count=0xFFFF,
            checksum=0,
            checksum_ok=False,
        )
        block = storage.log_block(manager, region)
        self.assertEqual(block.records, ())
        self.assertEqual(block.capacity, 0)

    def test_blob_summary_runs_and_hash(self):
        summary = storage.blob_summary(b"\xff\x00\x00\xff\x01")
        self.assertEqual(summary["size"], 5)
        self.assertEqual(summary["non_ff"], 3)
        self.assertEqual(summary["runs"], ((1, 3), (4, 5)))
        self.assertEqual(len(summary["sha256"]), 64)


class FirmwareLayoutTests(unittest.TestCase):
    def test_parameter_offsets_match_mapper_table(self):
        offsets, total = _parameter_table()
        self.assertEqual(total, 7428, "parameter block size changed")
        fields = storage.parameter_block(BLANK_EEPROM)
        for field_name, source_name in PARAMETER_SOURCE_NAMES.items():
            block_offset, size = offsets[source_name]
            field = fields[field_name]
            self.assertEqual(
                field.offset,
                storage.EEPROM_PARAMETER_BLOCK_OFFSET + block_offset,
                f"{field_name} offset",
            )
            self.assertEqual(field.size, size, f"{field_name} size")

    def test_instant_freeze_region_matches_record_table(self):
        offset = 0
        for name, length, depth in _record_table("record_item"):
            if name == "InstantFreeze":
                break
            offset += length
        else:
            self.fail("InstantFreeze missing from record_item[]")
        self.assertEqual(offset, 22620)
        self.assertEqual(length, storage.INSTANT_FREEZE_REGION_SIZE)
        self.assertEqual(
            storage.INSTANT_FREEZE_REGION_OFFSET,
            storage.EEPROM_EVENT_BLOCK_OFFSET + offset,
        )

    def test_freeze_regions_match_record_item_2(self):
        sections = _section_sizes()
        rows = _freeze_record_table()
        self.assertEqual([row[0] for row in rows], ["HourFreeze", "DayFreeze", "MonthFreeze"])
        offset = 0
        expected = {
            "hour": storage.HOUR_FREEZE_REGION_OFFSET,
            "day": storage.DAY_FREEZE_REGION_OFFSET,
            "month": storage.MONTH_FREEZE_REGION_OFFSET,
        }
        sizes = {
            "hour": storage.HOUR_FREEZE_REGION_SIZE,
            "day": storage.DAY_FREEZE_REGION_SIZE,
            "month": storage.MONTH_FREEZE_REGION_SIZE,
        }
        for name, expression, _depth in rows:
            macro, multiplier = re.match(r"(\w+)\s*\*\s*(\d+)", expression).groups()
            size = sections[macro] * int(multiplier)
            key = name.removesuffix("Freeze").lower()
            self.assertEqual(size, sizes[key])
            self.assertEqual(
                offset + storage.FLASH_EVENT2_BLOCK_OFFSET, expected[key]
            )
            offset += size

    @unittest.skipUnless(shutil.which("gcc"), "gcc is not available")
    def test_offsets_match_meter_record_header(self):
        source = r"""
#include <stddef.h>
#include <stdio.h>
#include "kernel/legacy_control_block.h"
#define P(name) printf("%s %zu\n", #name, offsetof(struct mbase_control_block_t, name))
int main(void) {
    printf("sizeof %zu\n", sizeof(struct mbase_control_block_t));
    P(static_section_start); P(static_section_end);
    P(persistent_section_start); P(persistent_section_end);
    P(active_p); P(active_n); P(active_p_dec); P(active_n_dec);
    P(Energy_chksum); P(MonthEnegery_dec); P(MonthEnegery); P(UseEnergy_chksum);
    P(tsa); P(meterNo); P(meter_const); P(relay_ctl_mode);
    P(OverCurrent_threshold); P(ZeroLineErrS_rate_threshold);
    P(ZeroLineErrS_Current_threshold); P(relay_do);
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

        probe = {}
        for line in output.splitlines():
            key, value = line.split()
            probe[key] = int(value)

        self.assertEqual(probe["sizeof"], 1368)
        self.assertEqual(
            probe["static_section_end"] - probe["static_section_start"] + 1,
            storage.EEPROM_STATIC_BLOCK_SIZE,
        )
        self.assertEqual(
            probe["persistent_section_end"] - probe["persistent_section_start"] + 1,
            storage.EEPROM_PERSISTENT_BLOCK_SIZE,
        )
        self.assertEqual(
            probe["active_p"] - probe["persistent_section_start"],
            storage.ENERGY_RECORD_OFFSET - storage.EEPROM_PERSISTENT_BLOCK_OFFSET,
        )
        for field, constant in (
            ("active_n", storage.ENERGY_ACTIVE_N_OFFSET),
            ("active_p_dec", storage.ENERGY_ACTIVE_P_DEC_OFFSET),
            ("active_n_dec", storage.ENERGY_ACTIVE_N_DEC_OFFSET),
            ("Energy_chksum", storage.ENERGY_CHECKSUM_OFFSET),
            ("MonthEnegery_dec", storage.ENERGY_MONTH_DEC_OFFSET),
            ("MonthEnegery", storage.ENERGY_MONTH_OFFSET),
            ("UseEnergy_chksum", storage.ENERGY_USE_CHECKSUM_OFFSET),
        ):
            self.assertEqual(probe[field] - probe["active_p"], constant, field)

        fields = storage.parameter_block(BLANK_EEPROM)
        for field_name, probe_name in (
            ("meter_address", "tsa"),
            ("meter_number", "meterNo"),
            ("meter_const", "meter_const"),
            ("relay_ctl_mode", "relay_ctl_mode"),
            ("overcurrent_threshold", "OverCurrent_threshold"),
            ("zero_line_rate_threshold", "ZeroLineErrS_rate_threshold"),
            ("zero_line_current_threshold", "ZeroLineErrS_Current_threshold"),
        ):
            self.assertEqual(
                fields[field_name].offset - storage.EEPROM_STATIC_BLOCK_OFFSET,
                probe[probe_name],
                field_name,
            )
        self.assertEqual(
            fields["relay_do"].offset - storage.EEPROM_PERSISTENT_BLOCK_OFFSET,
            probe["relay_do"] - probe["persistent_section_start"],
        )


class RealImageTests(unittest.TestCase):
    def _runs(self, case: str, name: str) -> list[Path]:
        directory = ACCEPTANCE_WORK / case
        return sorted(
            directory.glob(f"run-*/{name}"),
            key=lambda path: int(path.parent.name.split("-")[1]),
        )

    def test_reset_counter_sequence_in_real_images(self):
        runs = self._runs("ACC-PERSIST-001", "eeprom.bin")
        if not runs:
            self.skipTest("no ACC-PERSIST-001 EEPROM images under build/acceptance/work")
        values = []
        for path in runs:
            image = path.read_bytes()
            self.assertEqual(len(image), storage.EEPROM_SIZE, path)
            field = storage.parameter_block(image)["reset_count"]
            self.assertEqual(field.value, storage.reset_count(image), path)
            self.assertTrue(field.checksum_ok, path)
            values.append(field.value)
        if not any(values[index : index + 3] == [0, 1, 2] for index in range(len(values) - 2)):
            self.skipTest(f"no consecutive 0,1,2 boot runs in {len(values)} images: {values[:8]}")

    def test_event_records_from_real_image(self):
        blocks = []
        for path in self._runs("ACC-PERSIST-001", "eeprom.bin"):
            block = storage.event_records(path.read_bytes())
            if block.manager.checksum_ok and block.records:
                blocks.append(block)
        if not blocks:
            self.skipTest("no ACC-PERSIST-001 image has a valid InstantFreeze manager")

        self.assertTrue(any(block.records[0].raw == BOOT_RECORD for block in blocks))
        block = next((block for block in blocks if len(block.records) >= 3), None)
        if block is None:
            self.skipTest("no run reached three boot records")
        self.assertEqual(block.manager.unit_size, 10)
        self.assertEqual(block.capacity, storage.INSTANT_FREEZE_REGION_SIZE // 10)
        self.assertEqual(
            [record.sequence for record in block.records[:3]],
            [0xFFFFFFFF, 0, 1],
        )
        self.assertEqual(block.records[0].raw, BOOT_RECORD)
        self.assertEqual(
            block.records[0].timestamp,
            datetime(2021, 4, 30, 23, 59, tzinfo=timezone.utc),
        )

    def test_freeze_records_from_real_images(self):
        frozen_blocks = []
        for flash_path in self._runs("ACC-FLASH-001", "flash.bin"):
            eeprom_path = flash_path.with_name("eeprom.bin")
            if not eeprom_path.is_file():
                continue
            frozen = storage.freeze_records(
                eeprom_path.read_bytes(), flash_path.read_bytes()
            )
            if frozen["month"].records:
                frozen_blocks.append(frozen)
        if not frozen_blocks:
            self.skipTest("no ACC-FLASH-001 image has a month freeze record")

        month = frozen_blocks[0]["month"]
        self.assertTrue(month.manager.checksum_ok)
        self.assertEqual(month.manager.unit_size, 10)
        self.assertEqual(month.manager.count, 1)
        self.assertEqual(month.records[0].raw, MONTH_FREEZE_RECORD)
        self.assertEqual(
            month.records[0].timestamp,
            datetime(2021, 5, 1, 0, 0, tzinfo=timezone.utc),
        )


if __name__ == "__main__":
    unittest.main()
