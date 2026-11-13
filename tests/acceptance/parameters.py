"""Write individual parameters into a stategrid_meter EEPROM image.

Layout source of truth: mapper_698.c parameter_item[] and parameter_item_1[],
walked exactly as supply_parameter_info does.  Every entry is `same * len`
data bytes followed by a 2-byte big-endian checksum: 0xA5A5 plus the entry
bytes, mod 0x10000 (resource_tool.c verify_data/update_verify).

Three storage shapes are covered:

* ``parameter``: the 15 KiB block at EEPROM offset 2048.  One copy, no backup.
* ``parameter_1``: the 1 KiB block at offset 0.  nd_set_index mirrors every
  write into the backup block at offset 1024, so both entries get their own
  checksum.
* ``static``: the 133-byte control-block static section at 17408, mirrored at
  17541.  Its checksum is at offset 128 of each copy and covers bytes 1..127
  (do_block_static_checksum).  meter_const lives in both the protocol
  parameter block (big-endian) and this runtime copy (host little-endian),
  so set_parameters writes both.
"""

from __future__ import annotations

from dataclasses import dataclass

try:
    from tests.acceptance import storage
except ModuleNotFoundError:
    import storage

PARAMETER_BASE = storage.EEPROM_PARAMETER_BLOCK_OFFSET
PARAMETER_1_BASE = 0
PARAMETER_1_BACKUP_BASE = 1024
STATIC_BASE = storage.EEPROM_STATIC_BLOCK_OFFSET
STATIC_BACKUP_BASE = STATIC_BASE + storage.EEPROM_STATIC_BLOCK_SIZE
STATIC_CHECKSUM_OFFSET = 128


@dataclass(frozen=True)
class Field:
    """One copy of a value, plus the checksum that protects it.

    ``kind`` overrides the parameter encoding for this copy: the protocol
    block is big-endian, but the runtime static block mirrors a host struct.
    """

    offset: int
    size: int
    entry: int | None = None
    entry_size: int | None = None
    static_block: int | None = None
    kind: str | None = None


@dataclass(frozen=True)
class Parameter:
    kind: str
    fields: tuple[Field, ...]


def _parameter(relative_offset: int, size: int, entry: int, entry_size: int) -> Field:
    return Field(PARAMETER_BASE + relative_offset, size, PARAMETER_BASE + entry, entry_size)


def _static(control_offset: int, size: int, block: int, kind: str | None = None) -> Field:
    return Field(block + control_offset, size, static_block=block, kind=kind)


# kinds: u8, be16, be32 (protocol/A-XDR order), le16/le32 (runtime copies
# written by memcpy from a host value), bytes (raw field).
PARAMETERS: dict[str, Parameter] = {
    "meter_const": Parameter(
        "be32",
        (
            _parameter(6667, 4, 6667, 4),  # Meter_Const1
            _static(16, 4, STATIC_BASE, "le32"),  # runtime copy used by metering
            _static(16, 4, STATIC_BACKUP_BASE, "le32"),
        ),
    ),
    "season_count": Parameter("u8", (_parameter(5670, 1, 5670, 5),)),
    "day_table_count": Parameter("u8", (_parameter(5671, 1, 5670, 5),)),
    "day_period_count": Parameter("u8", (_parameter(5672, 1, 5670, 5),)),
    "fee_count": Parameter("u8", (_parameter(5673, 1, 5670, 5),)),
    "holiday_count": Parameter("u8", (_parameter(5674, 1, 5670, 5),)),
    "season_table_change": Parameter("bytes", (_parameter(5652, 7, 5652, 7),)),
    "day_table_change": Parameter("bytes", (_parameter(5661, 7, 5661, 7),)),
    "weekend": Parameter("u8", (_parameter(5802, 1, 5802, 1),)),
    "weekend_fee": Parameter("u8", (_parameter(5805, 1, 5805, 1),)),
    "hour_freeze_start": Parameter("bytes", (_parameter(7036, 5, 7036, 5),)),
    "hour_freeze_interval": Parameter("u8", (_parameter(7043, 1, 7043, 1),)),
    "day_freeze_time": Parameter("bytes", (_parameter(7046, 2, 7046, 2),)),
    "ec_save_diff": Parameter(
        "le16",
        (
            Field(194, 2, entry=194, entry_size=2),
            Field(PARAMETER_1_BACKUP_BASE + 194, 2, entry=PARAMETER_1_BACKUP_BASE + 194, entry_size=2),
        ),
    ),
    "imax_protect_value": Parameter("be32", (_parameter(6972, 4, 6972, 4),)),
    "imax_protect_time": Parameter("be16", (_parameter(6978, 2, 6978, 2),)),
    "remote_relay_off_time": Parameter("be16", (_parameter(7050, 2, 7050, 2),)),
    "balance_time": Parameter("bytes", (_parameter(6728, 6, 6728, 6),)),
    "meter_address": Parameter(
        "bytes",
        (
            _static(1, 6, STATIC_BASE),  # tsa
            _static(1, 6, STATIC_BACKUP_BASE),
        ),
    ),
}


def _read(image: bytes, field: Field) -> bytes:
    end = field.offset + field.size
    if end > len(image):
        raise ValueError(f"image too short: need {end} bytes, have {len(image)}")
    return image[field.offset:end]


def _encode(value: int | bytes, size: int, kind: str) -> bytes:
    if kind == "bytes":
        if not isinstance(value, (bytes, bytearray)):
            raise ValueError(f"{kind} parameter needs bytes, got {type(value).__name__}")
        if len(value) != size:
            raise ValueError(f"expected {size} bytes, got {len(value)}")
        return bytes(value)
    if isinstance(value, (bytes, bytearray)):
        raise ValueError(f"{kind} parameter needs an int, got {len(value)} bytes")
    order = "little" if kind in ("le16", "le32") else "big"
    try:
        return value.to_bytes(size, order)
    except (AttributeError, OverflowError):
        raise ValueError(f"value {value!r} does not fit in {size} bytes") from None


def _decode(raw: bytes, kind: str) -> int | bytes:
    if kind == "bytes":
        return raw
    return int.from_bytes(raw, "little" if kind in ("le16", "le32") else "big")


def _refresh_checksum(image: bytearray, field: Field) -> None:
    if field.entry is not None:
        data = _read(image, Field(field.entry, field.entry_size))
        trailer = field.entry + field.entry_size
        image[trailer:trailer + 2] = storage.checksum16(data).to_bytes(2, "big")
    elif field.static_block is not None:
        block = field.static_block
        data = _read(image, Field(block + 1, STATIC_CHECKSUM_OFFSET - 1))
        image[block + STATIC_CHECKSUM_OFFSET:block + STATIC_CHECKSUM_OFFSET + 4] = (
            storage.checksum16(data).to_bytes(4, "little")
        )


def set_parameters(image: bytes, values: dict[str, int | bytes]) -> bytes:
    """Return a copy of image with values written, checksums refreshed."""
    result = bytearray(image)
    for name, value in values.items():
        parameter = PARAMETERS.get(name)
        if parameter is None:
            raise KeyError(f"unknown parameter {name!r}")
        for field in parameter.fields:
            raw = _encode(value, field.size, field.kind or parameter.kind)
            end = field.offset + field.size
            if end > len(result):
                raise ValueError(
                    f"image too short for {name}: need {end} bytes, have {len(result)}"
                )
            result[field.offset:end] = raw
            _refresh_checksum(result, field)
    return bytes(result)


def read_parameters(image: bytes) -> dict[str, int | bytes]:
    """Decode every writable parameter from its main copy."""
    return {
        name: _decode(_read(image, parameter.fields[0]), parameter.fields[0].kind or parameter.kind)
        for name, parameter in PARAMETERS.items()
    }


def _checksum_ok(image: bytes, field: Field) -> bool:
    if field.entry is not None:
        data = _read(image, Field(field.entry, field.entry_size))
        trailer = _read(image, Field(field.entry + field.entry_size, 2))
        return storage.checksum_ok(data + trailer)
    if field.static_block is not None:
        block = field.static_block
        stored = int.from_bytes(_read(image, Field(block + STATIC_CHECKSUM_OFFSET, 4)), "little")
        return stored == storage.checksum16(
            _read(image, Field(block + 1, STATIC_CHECKSUM_OFFSET - 1))
        )
    raise AssertionError("field has no checksum domain")


def verify_parameter_checksums(image: bytes) -> dict[str, bool]:
    """Map each writable name to whether every copy's checksum is valid."""
    return {
        name: all(_checksum_ok(image, field) for field in parameter.fields)
        for name, parameter in PARAMETERS.items()
    }
