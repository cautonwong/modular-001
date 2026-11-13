"""Acceptance case for freeze-record placement across flash regions.

The only evidence is the flash region bytes the firmware wrote.
"""

from __future__ import annotations

from cases import case

import parameters
import protocol
import storage

BLANK_IMAGE = b"\xff" * storage.EEPROM_SIZE
PARAMETER_BASE = storage.EEPROM_PARAMETER_BLOCK_OFFSET
METER_CONST = 3200


def _entry(image: bytes, offset: int, data: bytes, size: int) -> bytes:
    assert len(data) <= size, (len(data), size)
    padded = bytes(data) + b"\xff" * (size - len(data))
    image = bytearray(image)
    image[PARAMETER_BASE + offset : PARAMETER_BASE + offset + size] = padded
    image[
        PARAMETER_BASE + offset + size : PARAMETER_BASE + offset + size + 2
    ] = ((0xA5A5 + sum(padded)) & 0xFFFF).to_bytes(2, "big")
    return bytes(image)


def _image(**fields: bytes) -> bytes:
    image = parameters.set_parameters(BLANK_IMAGE, {"meter_const": METER_CONST})
    for offset, data in fields.items():
        image = _entry(image, int(offset), data, len(data))
    return image


@case(
    "ACC-STO-004",
    "with only the hour period enabled, the hour freeze record lands in the "
    "hour flash region at the event-flash-2 base and the day flash region "
    "stays erased",
    ("storage", "freeze", "flash"),
)
def freeze_flash_partition(ctx):
    image = _image(
        **{
            "3413": bytes([1, 0, 1]),
            "3418": bytes([0, 0, 0]),
        }
    )
    run = ctx.run(cycles=15, seed_eeprom=image)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    hour = storage.freeze_records(run.eeprom, run.flash)["hour"]
    assert hour.manager.checksum_ok, hour.manager
    assert hour.manager.count == 1, hour.manager

    hour_region = run.flash[
        storage.HOUR_FREEZE_REGION_OFFSET : storage.HOUR_FREEZE_REGION_OFFSET
        + storage.HOUR_FREEZE_REGION_SIZE
    ]
    assert hour_region[:10] == bytes.fromhex("06ffffffff1c281f5680"), (
        hour_region[:10].hex()
    )

    day_region = run.flash[
        storage.DAY_FREEZE_REGION_OFFSET : storage.DAY_FREEZE_REGION_OFFSET
        + storage.DAY_FREEZE_REGION_SIZE
    ]
    assert day_region == b"\xff" * len(day_region), "day freeze region was written"


BLANK_ADDRESS = b"\xff" * 6
LAST_N = lambda n: b"\x09" + bytes([n])


def _frame(apdu: bytes) -> bytes:
    return protocol.build_frame(apdu, sa=BLANK_ADDRESS, ca=0x10, ctrl=0x43)


def _record_request(piid: int, oad: int, rsd: bytes) -> bytes:
    apdu = bytes([0x05, 0x03, piid]) + oad.to_bytes(4, "big") + rsd + b"\x00\x00"
    return _frame(apdu)


@case(
    "ACC-FRZ-008",
    "a month-freeze record is readable through the 698 record OAD with its "
    "timestamp and sequence fields",
    ("freeze", "698", "record"),
)
def freeze_record_read(ctx):
    request = _record_request(1, 0x50060200, LAST_N(1))
    run = ctx.run(
        cycles=3600,
        scenario=f"at 3550 uart 0 {request.hex()}\n",
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    frames = list(protocol.iter_698_frames(run.uart_tx))
    assert len(frames) == 1, run.uart_tx.hex()
    apdu = protocol.parse_frame(frames[0]).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x03, apdu.hex()
    position = 8 + apdu[7] * 5
    assert apdu[position] == 0x01, apdu.hex()
    count = apdu[position + 1]
    body = apdu[position + 2 :]
    assert count == 1, (count, apdu.hex())
    assert body.startswith(bytes.fromhex("06ffffffff1c07e50501000000")), body.hex()
