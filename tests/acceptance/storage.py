"""Byte-level decoders for stategrid_meter EEPROM and flash images.

The offsets below come from firmware code, not from guessing:

* EEPROM block layout: stategrid_meter_entry.c registration order.
* Energy record: legacy_energy_record.h, checked against meter_record.h.
* Parameter offsets: mapper_698.c parameter_item[].
* Event and freeze regions: mapper_698.c record_item[], record_item_2[].
* Checksums: resource_tool.c verify_data/update_verify and do_checksum.

test_storage.py re-derives these offsets from the C sources and fails when the
decoder and the firmware disagree. Images are the flat files firmware_sim
writes: 65535 bytes for EEPROM, 8388607 bytes for flash.
"""

from __future__ import annotations

import hashlib
import struct
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone

EEPROM_SIZE = 65535
FLASH_SIZE = 8388607

# The reset counter is a uint32 in the parameter block, read and written by
# nd_get_index/nd_set_index through the ResetCount parameter entry.
EEPROM_RESET_COUNT_OFFSET = 6006
EEPROM_RESET_COUNT_WIDTH = 4

# EEPROM block layout from stategrid_meter_entry.c: parameter_1, parameter_1_bp,
# parameter, static_eprom, static_eprom_bp, persistent_eprom, event.
EEPROM_PARAMETER_BLOCK_OFFSET = 2048
EEPROM_PARAMETER_BLOCK_SIZE = 15 * 1024
EEPROM_STATIC_BLOCK_OFFSET = 17408
EEPROM_STATIC_BLOCK_SIZE = 133
EEPROM_PERSISTENT_BLOCK_OFFSET = 17674
EEPROM_PERSISTENT_BLOCK_SIZE = 512
EEPROM_EVENT_BLOCK_OFFSET = 18186
EEPROM_EVENT_BLOCK_SIZE = 26 * 1024

# Flash blocks in registration order.
FLASH_EVENT1_BLOCK_OFFSET = 0
FLASH_EVENT1_BLOCK_SIZE = 406 * 4096
FLASH_EVENT2_BLOCK_OFFSET = FLASH_EVENT1_BLOCK_SIZE
FLASH_EVENT2_BLOCK_SIZE = (4 + 20 + 4) * 4096
FLASH_PUBLIC_BLOCK_OFFSET = FLASH_EVENT2_BLOCK_OFFSET + FLASH_EVENT2_BLOCK_SIZE

# The 172-byte persisted energy slice starts at persistent_section_start + 3.
# Field offsets are relative to active_p and match legacy_energy_record.h.
ENERGY_RECORD_OFFSET = EEPROM_PERSISTENT_BLOCK_OFFSET + 3
ENERGY_RECORD_SIZE = 172
ENERGY_TARIFF_SLOTS = 13
ENERGY_ACTIVE_P_OFFSET = 0
ENERGY_ACTIVE_N_OFFSET = 52
ENERGY_ACTIVE_P_DEC_OFFSET = 104
ENERGY_ACTIVE_N_DEC_OFFSET = 130
ENERGY_CHECKSUM_OFFSET = 156
ENERGY_MONTH_DEC_OFFSET = 160
ENERGY_MONTH_OFFSET = 164
ENERGY_USE_CHECKSUM_OFFSET = 168

# Record IDs from domain/events/event_id.h.
HOUR_FREEZE_RECORD_ID = 178
DAY_FREEZE_RECORD_ID = 179
MONTH_FREEZE_RECORD_ID = 181
INSTANT_FREEZE_RECORD_ID = 188

# Log managers live in the EEPROM parameter block. Offsets are absolute.
HOUR_FREEZE_MANAGER_OFFSET = 2144
DAY_FREEZE_MANAGER_OFFSET = 2156
MONTH_FREEZE_MANAGER_OFFSET = 2180
INSTANT_FREEZE_MANAGER_OFFSET = 2252

# Event/freeze record regions come from record_item[] and record_item_2[].
INSTANT_FREEZE_REGION_OFFSET = EEPROM_EVENT_BLOCK_OFFSET + 22620
INSTANT_FREEZE_REGION_SIZE = 670
HOUR_FREEZE_REGION_OFFSET = FLASH_EVENT2_BLOCK_OFFSET
HOUR_FREEZE_REGION_SIZE = 4 * 4096
DAY_FREEZE_REGION_OFFSET = HOUR_FREEZE_REGION_OFFSET + HOUR_FREEZE_REGION_SIZE
DAY_FREEZE_REGION_SIZE = 20 * 4096
MONTH_FREEZE_REGION_OFFSET = DAY_FREEZE_REGION_OFFSET + DAY_FREEZE_REGION_SIZE
MONTH_FREEZE_REGION_SIZE = 4 * 4096

# Parameter block entries used by acceptance cases. Offsets are absolute;
# each entry is size bytes of data followed by a 2-byte big-endian checksum.
_PARAMETER_FIELDS: dict[str, tuple[int, int, str]] = {
    "reset_count": (EEPROM_RESET_COUNT_OFFSET, 4, "u32"),
    "imax_protect_value": (9020, 4, "u32"),
    "imax_protect_time": (9026, 2, "u16"),
    "tariff_change_time": (9054, 7, "bytes"),
    "hour_freeze_start": (9084, 5, "bytes"),
    "hour_freeze_interval": (9091, 1, "u8"),
    "day_freeze_time": (9094, 2, "bytes"),
    "remote_relay_off_time": (9098, 2, "u16"),
    "parameter_init_flag": (9470, 4, "u32"),
}

# Control block static section, persisted at EEPROM_STATIC_BLOCK_OFFSET.
# Offsets are inside the 1368-byte mbase_control_block_t.
_STATIC_FIELDS: dict[str, tuple[int, int, str]] = {
    "meter_address": (1, 6, "bytes"),
    "meter_number": (7, 6, "bytes"),
    "meter_const": (16, 4, "u32"),
    "relay_ctl_mode": (43, 1, "u8"),
    "overcurrent_threshold": (48, 4, "u32"),
    "zero_line_rate_threshold": (52, 2, "u16"),
    "zero_line_current_threshold": (56, 4, "u32"),
}

# Control block persistent section, persisted at EEPROM_PERSISTENT_BLOCK_OFFSET.
_PERSISTENT_FIELDS: dict[str, tuple[int, int, str]] = {
    "relay_do": (260, 1, "u8"),
}

_EPOCH = datetime(2000, 1, 1, tzinfo=timezone.utc)


def checksum16(data: bytes) -> int:
    """16-bit checksum used by verify_data/update_verify and do_checksum."""
    return (0xA5A5 + sum(data)) & 0xFFFF


def checksum_ok(data: bytes) -> bool:
    """True when the trailing two bytes match checksum16 of the data before them."""
    if len(data) < 2:
        return False
    return int.from_bytes(data[-2:], "big") == checksum16(data[:-2])


def seconds_to_datetime(seconds: int) -> datetime:
    """Convert the firmware's epoch-2000 second counter to a UTC datetime."""
    return _EPOCH + timedelta(seconds=seconds)


def _slice(image: bytes, offset: int, size: int) -> bytes:
    end = offset + size
    if offset < 0 or size < 0 or end > len(image):
        raise ValueError(f"image too short: need {end} bytes, have {len(image)}")
    return image[offset:end]


def _decode(raw: bytes, kind: str) -> int | bytes:
    if kind == "bytes":
        return raw
    return int.from_bytes(raw, "little")


def reset_count(image: bytes) -> int:
    """Decode the persisted reset counter at EEPROM offset 6006."""
    return int.from_bytes(
        _slice(image, EEPROM_RESET_COUNT_OFFSET, EEPROM_RESET_COUNT_WIDTH),
        "little",
    )


def blob_summary(image: bytes) -> dict[str, object]:
    """Stable byte-level summary, including runs of non-0xFF bytes."""
    runs: list[tuple[int, int]] = []
    start: int | None = None
    for index, value in enumerate(image):
        if value != 0xFF:
            if start is None:
                start = index
        elif start is not None:
            runs.append((start, index))
            start = None
    if start is not None:
        runs.append((start, len(image)))
    return {
        "size": len(image),
        "non_ff": sum(end - begin for begin, end in runs),
        "runs": tuple(runs),
        "sha256": hashlib.sha256(image).hexdigest(),
    }


@dataclass(frozen=True)
class EnergyAmount:
    integer: int
    fraction: int


@dataclass(frozen=True)
class EnergyRecord:
    forward: tuple[EnergyAmount, ...]
    reverse: tuple[EnergyAmount, ...]
    month: EnergyAmount
    energy_checksum: int
    use_energy_checksum: int
    energy_checksum_ok: bool
    use_energy_checksum_ok: bool


def energy_record(image: bytes, offset: int = ENERGY_RECORD_OFFSET) -> EnergyRecord:
    """Decode the 172-byte energy slice.

    Default offset is the firmware_sim EEPROM layout. Pass 852 for a raw 1368
    byte control block, or 0 for a 172-byte record.
    """
    raw = _slice(image, offset, ENERGY_RECORD_SIZE)
    forward_integer = struct.unpack_from("<13I", raw, ENERGY_ACTIVE_P_OFFSET)
    reverse_integer = struct.unpack_from("<13I", raw, ENERGY_ACTIVE_N_OFFSET)
    forward_fraction = struct.unpack_from("<13H", raw, ENERGY_ACTIVE_P_DEC_OFFSET)
    reverse_fraction = struct.unpack_from("<13H", raw, ENERGY_ACTIVE_N_DEC_OFFSET)
    month_integer = struct.unpack_from("<I", raw, ENERGY_MONTH_OFFSET)[0]
    month_fraction = struct.unpack_from("<H", raw, ENERGY_MONTH_DEC_OFFSET)[0]
    energy_checksum = struct.unpack_from("<I", raw, ENERGY_CHECKSUM_OFFSET)[0]
    use_energy_checksum = struct.unpack_from("<I", raw, ENERGY_USE_CHECKSUM_OFFSET)[0]
    return EnergyRecord(
        forward=tuple(
            EnergyAmount(integer, fraction)
            for integer, fraction in zip(forward_integer, forward_fraction)
        ),
        reverse=tuple(
            EnergyAmount(integer, fraction)
            for integer, fraction in zip(reverse_integer, reverse_fraction)
        ),
        month=EnergyAmount(month_integer, month_fraction),
        energy_checksum=energy_checksum,
        use_energy_checksum=use_energy_checksum,
        energy_checksum_ok=energy_checksum
        == checksum16(raw[ENERGY_ACTIVE_P_OFFSET:ENERGY_CHECKSUM_OFFSET]),
        use_energy_checksum_ok=use_energy_checksum
        == checksum16(raw[ENERGY_MONTH_DEC_OFFSET:ENERGY_USE_CHECKSUM_OFFSET]),
    )


@dataclass(frozen=True)
class ParameterValue:
    value: int | bytes
    offset: int
    size: int
    block: str
    checksum_ok: bool | None


def parameter_block(image: bytes) -> dict[str, ParameterValue]:
    """Decode the persisted parameter surface of an EEPROM image.

    Includes parameter block entries (per-entry checksum), the control block
    static section (no per-field checksum), and relay_do from the persistent
    section. Fields whose layout could not be anchored in firmware code are
    deliberately absent.
    """
    fields: dict[str, ParameterValue] = {}
    for name, (offset, size, kind) in _PARAMETER_FIELDS.items():
        fields[name] = ParameterValue(
            value=_decode(_slice(image, offset, size), kind),
            offset=offset,
            size=size,
            block="parameter",
            checksum_ok=checksum_ok(_slice(image, offset, size + 2)),
        )
    for name, (control_offset, size, kind) in _STATIC_FIELDS.items():
        offset = EEPROM_STATIC_BLOCK_OFFSET + control_offset
        fields[name] = ParameterValue(
            value=_decode(_slice(image, offset, size), kind),
            offset=offset,
            size=size,
            block="static",
            checksum_ok=None,
        )
    for name, (control_offset, size, kind) in _PERSISTENT_FIELDS.items():
        offset = EEPROM_PERSISTENT_BLOCK_OFFSET + control_offset
        fields[name] = ParameterValue(
            value=_decode(_slice(image, offset, size), kind),
            offset=offset,
            size=size,
            block="persistent",
            checksum_ok=None,
        )
    return fields


@dataclass(frozen=True)
class LogManager:
    read_index: int
    write_index: int
    count: int
    unit_size: int
    max_count: int
    checksum: int
    checksum_ok: bool


@dataclass(frozen=True)
class LogRecord:
    slot: int
    type_tag: int
    sequence: int | None
    timestamp_seconds: int | None
    timestamp: datetime | None
    raw: bytes


@dataclass(frozen=True)
class LogBlock:
    name: str
    record_id: int
    region_offset: int
    region_size: int
    manager: LogManager
    capacity: int
    records: tuple[LogRecord, ...]


def _read_manager(image: bytes, offset: int) -> LogManager:
    raw = _slice(image, offset, 12)
    read_index, write_index, count, unit_size, max_count = struct.unpack_from(
        "<5H", raw, 0
    )
    return LogManager(
        read_index=read_index,
        write_index=write_index,
        count=count,
        unit_size=unit_size,
        max_count=max_count,
        checksum=int.from_bytes(raw[10:12], "big"),
        checksum_ok=checksum_ok(raw),
    )


def _parse_record(raw: bytes, slot: int) -> LogRecord:
    # The first byte is the A-XDR type tag for the sequence, normally 0x06.
    # get_record_sequence_add writes 4 big-endian sequence bytes at offset 1.
    # fc_fun_01 writes 0x1C and 4 big-endian epoch-2000 seconds at offset 5.
    sequence = int.from_bytes(raw[1:5], "big") if len(raw) >= 5 else None
    timestamp_seconds = (
        int.from_bytes(raw[6:10], "big")
        if len(raw) >= 10 and raw[5] == 0x1C
        else None
    )
    return LogRecord(
        slot=slot,
        type_tag=raw[0],
        sequence=sequence,
        timestamp_seconds=timestamp_seconds,
        timestamp=(
            seconds_to_datetime(timestamp_seconds)
            if timestamp_seconds is not None
            else None
        ),
        raw=raw,
    )


def log_block(
    manager: LogManager,
    region: bytes,
    *,
    name: str = "",
    record_id: int = 0,
    region_offset: int = 0,
) -> LogBlock:
    """Decode a circular log region in oldest-first order.

    Read starts at r_ptr and wraps at region_size / unit_size. A failed manager
    checksum means the firmware treats the log as blank, so no records decode.
    """
    capacity = len(region) // manager.unit_size if manager.unit_size else 0
    records: tuple[LogRecord, ...] = ()
    if manager.checksum_ok and manager.count and capacity:
        count = min(manager.count, capacity)
        records = tuple(
            _parse_record(
                _slice(
                    region,
                    ((manager.read_index + index) % capacity) * manager.unit_size,
                    manager.unit_size,
                ),
                (manager.read_index + index) % capacity,
            )
            for index in range(count)
        )
    return LogBlock(
        name=name,
        record_id=record_id,
        region_offset=region_offset,
        region_size=len(region),
        manager=manager,
        capacity=capacity,
        records=records,
    )


def event_records(image: bytes) -> LogBlock:
    """Decode the InstantFreeze log. This is the EEPROM event region that
    carries boot records; other event regions have no verified acceptance
    evidence yet."""
    region = _slice(image, INSTANT_FREEZE_REGION_OFFSET, INSTANT_FREEZE_REGION_SIZE)
    return log_block(
        _read_manager(image, INSTANT_FREEZE_MANAGER_OFFSET),
        region,
        name="instant_freeze",
        record_id=INSTANT_FREEZE_RECORD_ID,
        region_offset=INSTANT_FREEZE_REGION_OFFSET,
    )


def freeze_records(eeprom: bytes, flash: bytes) -> dict[str, LogBlock]:
    """Decode hour, day and month freeze logs.

    Managers live in the EEPROM parameter block; records live in event_flash2.
    Minute freeze schemes need MinFrez_Sectors and are not decoded.
    """
    specs = (
        (
            "hour",
            HOUR_FREEZE_RECORD_ID,
            HOUR_FREEZE_MANAGER_OFFSET,
            HOUR_FREEZE_REGION_OFFSET,
            HOUR_FREEZE_REGION_SIZE,
        ),
        (
            "day",
            DAY_FREEZE_RECORD_ID,
            DAY_FREEZE_MANAGER_OFFSET,
            DAY_FREEZE_REGION_OFFSET,
            DAY_FREEZE_REGION_SIZE,
        ),
        (
            "month",
            MONTH_FREEZE_RECORD_ID,
            MONTH_FREEZE_MANAGER_OFFSET,
            MONTH_FREEZE_REGION_OFFSET,
            MONTH_FREEZE_REGION_SIZE,
        ),
    )
    blocks = {}
    for name, record_id, manager_offset, region_offset, region_size in specs:
        blocks[name] = log_block(
            _read_manager(eeprom, manager_offset),
            _slice(flash, region_offset, region_size),
            name=name,
            record_id=record_id,
            region_offset=region_offset,
        )
    return blocks
