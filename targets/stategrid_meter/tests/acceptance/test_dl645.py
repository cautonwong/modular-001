from __future__ import annotations

from cases import case
from sim import EEPROM_SIZE

import parameters
import protocol

BLANK_ADDRESS = b"\xff" * 6
WILDCARD_ADDRESS = b"\xaa" * 6
BROADCAST_ADDRESS = b"\x99" * 6

DATA_OFFSET = protocol.DATA_OFFSET

# A blank EEPROM leaves Pass02Err_Cnt at 0xFF, which Cmp_Password() treats as a
# permanent lockout (fr_645.c:557 requires the counter below 3 and does not
# increment it once it is not). Seed the register to 0 so the comparison runs.
# Layout: static EEPROM starts at 17408 and is followed by two 133-byte static
# blocks; Pass02Err_Cnt sits 464 bytes into the persistent section
# (meter_record.h offsets 1313 and 849), so 17408 + 2 * 133 + 464 = 18138
# (app_meter_base_entry.c restores that section on MSG_POWERON).
PASS02_ERR_CNT_OFFSET = 18138


def _frame(address: bytes, control: int, data: bytes) -> bytes:
    body = bytes([0x68]) + address + bytes([0x68, control, len(data)])
    body += bytes((byte + DATA_OFFSET) & 0xFF for byte in data)
    return body + bytes([protocol.checksum(body), 0x16])


def _read(address: bytes, data_id: str) -> bytes:
    return _frame(address, 0x11, protocol.data_id_bytes(data_id))


def _decode(frame: bytes) -> tuple[str, int, bytes]:
    """Validate the frame and return (address, control, un-offset data)."""
    address, control, raw = protocol.parse_frame_645(frame)
    return address, control, bytes((byte - DATA_OFFSET) & 0xFF for byte in raw)


def _run(ctx, requests, *, cycles=None, seed_eeprom=None):
    lines = [
        f"at {index} uart 0 {request.hex()}"
        for index, request in enumerate(requests, 1)
    ]
    return ctx.run(
        cycles=cycles if cycles is not None else len(requests) + 3,
        scenario="\n".join(lines) + "\n",
        seed_eeprom=seed_eeprom,
    )


@case(
    "ACC-645-001",
    "a 645 read of DI 00010000 returns a complete checksummed response whose "
    "address and DI match and whose active-energy value is zero on the blank "
    "meter (ctl_11_read.c Comm_Read, ctl_11_read00.c); a second run seeds the "
    "static meter_address to 01 00 00 00 00 00 and reads at that address",
    ("645", "metering"),
)
def read_total_energy(ctx):
    run = _run(ctx, [_read(BLANK_ADDRESS, "00010000")])
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == 1, f"expected one 645 response: {run.uart_tx.hex()}"

    address, control, data = _decode(frames[0])
    assert address == "ffffffffffff", address
    assert control == 0x91, hex(control)
    assert data[:4] == protocol.data_id_bytes("00010000"), data.hex()
    assert data[4:] == b"\x00\x00\x00\x00", data.hex()

    seeded_address = bytes.fromhex("010000000000")
    image = parameters.set_parameters(
        b"\xff" * EEPROM_SIZE, {"meter_address": seeded_address}
    )
    request = _frame(seeded_address, 0x11, protocol.data_id_bytes("00010000"))
    run = _run(ctx, [request], cycles=5, seed_eeprom=image)
    assert run.exit_code == 0, f"seeded firmware_sim exited {run.exit_code}"
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == 1, f"expected one seeded response: {run.uart_tx.hex()}"

    address, control, data = _decode(frames[0])
    assert address == "000000000001", address
    assert control == 0x91, hex(control)
    assert data[:4] == protocol.data_id_bytes("00010000"), data.hex()
    assert data[4:] == b"\x00\x00\x00\x00", data.hex()


@case(
    "ACC-645-008",
    "645 reads of energy, voltage, current and time objects each decode with "
    "the length declared in 645id_table.c:25-49 and produced by "
    "ctl_11_read00.c, ctl_11_read02.c and ctl_11_read04.c",
    ("645", "object-model"),
)
def read_object_family(ctx):
    expected = {
        "00010000": 4,  # forward active total energy, ctl_11_read00.c
        "02010100": 2,  # voltage, ctl_11_read02.c
        "02020100": 3,  # current, ctl_11_read02.c
        "04000102": 3,  # meter time, 645id_table.c FORMAT_02 length 3
    }
    run = _run(ctx, [_read(BLANK_ADDRESS, data_id) for data_id in expected])
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == len(expected), (
        f"expected {len(expected)} responses: {run.uart_tx.hex()}"
    )

    for frame, (data_id, length) in zip(frames, expected.items()):
        address, control, data = _decode(frame)
        assert address == "ffffffffffff", address
        assert control == 0x91, f"{data_id}: control {control:#04x}"
        assert data[:4] == protocol.data_id_bytes(data_id), data.hex()
        assert len(data) == 4 + length, f"{data_id}: {data.hex()}"


@case(
    "ACC-645-006",
    "a frame with a bad checksum produces no response and the parser "
    "resynchronises to a following valid read in the same UART event "
    "(fr_645_matcher.c, ringbuffer_filter in infra/kernel/fifo.c:216-267)",
    ("645", "robustness"),
)
def malformed_frame_is_ignored(ctx):
    bad = bytearray(_read(BLANK_ADDRESS, "00010000"))
    bad[-2] ^= 0xFF  # corrupt CS
    good = _read(BLANK_ADDRESS, "00010000")

    run = _run(ctx, [bytes(bad) + good], cycles=4)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == 1, f"expected only the valid read to answer: {run.uart_tx.hex()}"

    address, control, data = _decode(frames[0])
    assert address == "ffffffffffff", address
    assert control == 0x91, hex(control)
    assert data[:4] == protocol.data_id_bytes("00010000"), data.hex()


@case(
    "ACC-645-004",
    "wildcard AA*6 and broadcast 99*6 addressing follows the gatekeeper in "
    "fr_645.c:733-796: AA reads the meter but 99 is silent, AA may read the "
    "address (C=13) and an exact address may not",
    ("645", "addressing"),
)
def wildcard_and_broadcast(ctx):
    run = _run(
        ctx,
        [
            _read(BLANK_ADDRESS, "00010000"),
            _read(WILDCARD_ADDRESS, "00010000"),
            _frame(WILDCARD_ADDRESS, 0x13, b""),
        ],
        cycles=6,
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == 3, f"expected three responses: {run.uart_tx.hex()}"

    address, control, data = _decode(frames[0])
    assert (address, control) == ("ffffffffffff", 0x91)
    assert data[4:] == b"\x00\x00\x00\x00", data.hex()

    address, control, data = _decode(frames[1])
    assert (address, control) == ("ffffffffffff", 0x91), (
        "a AA wildcard read answers with the meter TSA"
    )
    assert data[4:] == b"\x00\x00\x00\x00", data.hex()

    address, control, data = _decode(frames[2])
    assert (address, control) == ("ffffffffffff", 0x93)
    assert data == BLANK_ADDRESS, data.hex()

    silent = _run(
        ctx,
        [
            _read(BROADCAST_ADDRESS, "00010000"),
            _frame(
                BROADCAST_ADDRESS,
                0x08,
                bytes([0x50, 0x59, 0x23, 0x30, 0x04, 0x21]),
            ),
            _frame(BLANK_ADDRESS, 0x13, b""),
        ],
        cycles=6,
    )
    assert silent.exit_code == 0, f"firmware_sim exited {silent.exit_code}"
    assert protocol.iter_645_frames(silent.uart_tx) == [], silent.uart_tx.hex()


@case(
    "ACC-645-002",
    "writing the meter address (DI 04000401) after the factory-mode "
    "precondition persists across a restart. Factory mode is required because "
    "metertype 0801 lacks F_Password_En, so security_judge_645 returns 0 and "
    "Comm_Write rejects a plaintext write (ctl_14_write.c:776-797, 863-867; "
    "ctl_03_SafeAuthen.c:279-294)",
    ("645", "persistence"),
)
def write_address_survives_restart(ctx):
    factory = _frame(
        BLANK_ADDRESS,
        0x14,
        protocol.data_id_bytes("A8000100")
        + b"\x02\x00\x00\x00"
        + b"\x00\x00\x00\x00"
        + b"\xfb",
    )
    new_address = bytes([0x12, 0x00, 0x00, 0x00, 0x00, 0x00])
    write = _frame(
        BLANK_ADDRESS,
        0x14,
        protocol.data_id_bytes("04000401")
        + b"\x02\x00\x00\x00"
        + b"\x00\x00\x00\x00"
        + new_address,
    )
    readback = _frame(new_address, 0x11, protocol.data_id_bytes("04000401"))

    first = _run(ctx, [factory, write, readback], cycles=8)
    assert first.exit_code == 0, f"firmware_sim exited {first.exit_code}"
    frames = protocol.iter_645_frames(first.uart_tx)
    assert len(frames) == 3, f"expected three responses: {first.uart_tx.hex()}"

    for frame in frames[:2]:
        _, control, data = _decode(frame)
        assert control == 0x94, hex(control)
        assert data == b"", data.hex()

    address, control, data = _decode(frames[2])
    assert address == "000000000012", address
    assert control == 0x91, hex(control)
    assert data[:4] == protocol.data_id_bytes("04000401"), data.hex()
    assert data[4:] == new_address, data.hex()

    second = _run(ctx, [readback], cycles=4, seed_eeprom=first.eeprom)
    assert second.exit_code == 0, f"restart exited {second.exit_code}"
    frames = protocol.iter_645_frames(second.uart_tx)
    assert len(frames) == 1, f"no read after restart: {second.uart_tx.hex()}"
    address, control, data = _decode(frames[0])
    assert address == "000000000012", address
    assert control == 0x91, hex(control)
    assert data[4:] == new_address, data.hex()


@case(
    "ACC-645-005",
    "event clear (C=1B) rejects a wrong plaintext password with error byte 04 "
    "and accepts the correct one with C=9B and no error byte; Cmp_Password in "
    "fr_645.c:548-585, dl645_event_clear in ctl_other.c:362-450",
    ("645", "security"),
)
def password_checked(ctx):
    wrong = _frame(
        BLANK_ADDRESS,
        0x1B,
        bytes([0x02, 0x00, 0x00, 0x00])
        + b"\x00\x00\x00\x00"
        + b"\xff\xff\xff\xff",
    )
    right = _frame(
        BLANK_ADDRESS,
        0x1B,
        bytes([0x02, 0xFF, 0xFF, 0xFF])
        + b"\x00\x00\x00\x00"
        + b"\xff\xff\xff\xff",
    )

    seed = bytearray([0xFF]) * EEPROM_SIZE
    seed[PASS02_ERR_CNT_OFFSET] = 0

    run = _run(ctx, [wrong], cycles=4, seed_eeprom=bytes(seed))
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == 1, f"expected one error response: {run.uart_tx.hex()}"
    address, control, data = _decode(frames[0])
    assert address == "ffffffffffff", address
    assert control == 0xDB, hex(control)
    assert data == b"\x04", data.hex()

    seed[PASS02_ERR_CNT_OFFSET] = 0
    run = _run(ctx, [right], cycles=6, seed_eeprom=bytes(seed))
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == 1, f"expected one success response: {run.uart_tx.hex()}"
    address, control, data = _decode(frames[0])
    assert address == "ffffffffffff", address
    assert control == 0x9B, hex(control)
    assert data == b"", data.hex()


@case(
    "ACC-645-009",
    "a writable parameter (DI 04000207, step count) accepts a factory-mode "
    "write, reads back with the value in the response, and keeps the value "
    "after a restart (ctl_14_write.c Comm_Write and GetCommTable)",
    ("645", "parameter"),
)
def write_parameter_survives_restart(ctx):
    factory = _frame(
        BLANK_ADDRESS,
        0x14,
        protocol.data_id_bytes("A8000100")
        + b"\x02\x00\x00\x00"
        + b"\x00\x00\x00\x00"
        + b"\xfb",
    )
    write = _frame(
        BLANK_ADDRESS,
        0x14,
        protocol.data_id_bytes("04000207")
        + b"\x02\x00\x00\x00"
        + b"\x00\x00\x00\x00"
        + b"\x01",
    )
    read = _frame(BLANK_ADDRESS, 0x11, protocol.data_id_bytes("04000207"))

    first = _run(ctx, [factory, write, read], cycles=8)
    assert first.exit_code == 0, f"firmware_sim exited {first.exit_code}"
    frames = protocol.iter_645_frames(first.uart_tx)
    assert len(frames) == 3, f"expected three responses: {first.uart_tx.hex()}"

    for frame in frames[:2]:
        _, control, data = _decode(frame)
        assert control == 0x94, hex(control)
        assert data == b"", data.hex()

    _, control, data = _decode(frames[2])
    assert control == 0x91, hex(control)
    assert data[:4] == protocol.data_id_bytes("04000207"), data.hex()
    assert data[4:] == b"\x01", data.hex()

    second = _run(ctx, [read], cycles=4, seed_eeprom=first.eeprom)
    assert second.exit_code == 0, f"restart exited {second.exit_code}"
    frames = protocol.iter_645_frames(second.uart_tx)
    assert len(frames) == 1, f"no read after restart: {second.uart_tx.hex()}"
    _, control, data = _decode(frames[0])
    assert control == 0x91, hex(control)
    assert data[:4] == protocol.data_id_bytes("04000207"), data.hex()
    assert data[4:] == b"\x01", data.hex()
