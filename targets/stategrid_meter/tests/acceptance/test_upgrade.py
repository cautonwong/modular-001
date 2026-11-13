from __future__ import annotations

from cases import case

import protocol

ADDRESS = b"\xff" * 6
PASSWORD = b"\x03\xff\xff\xff"
DATA_OFFSET = protocol.DATA_OFFSET

# mbase_block_parameter_1 is the first EEPROM parameter block, and
# UPGRADE_PARA_INFO is its first item, so the 59-byte image_transfer_state_t
# sits at offset 0. A reserved byte and a big-endian checksum follow, then the
# four-byte reset flag and its checksum at 62; mbase_block_parameter_1_bp
# mirrors the whole block at 1024.
IMAGE_STATE = slice(0, 59)
RESET_FLAG = slice(62, 66)
BACKUP = 1024


def _frame(address: bytes, control: int, data: bytes, *, long_length: bool = False) -> bytes:
    length = len(data).to_bytes(2, "little") if long_length else bytes([len(data)])
    body = bytes([0x68]) + address + bytes([0x68, control]) + length
    body += bytes((byte + DATA_OFFSET) & 0xFF for byte in data)
    return body + bytes([protocol.checksum(body), 0x16])


def _run(ctx, requests, *, cycles=None, seed_eeprom=None):
    lines = [
        f"at {index} uart 0 {request.hex()}"
        for index, request in enumerate(requests, 1)
    ]
    return ctx.run(
        cycles=cycles if cycles is not None else len(requests) + 4,
        scenario="\n".join(lines) + "\n",
        seed_eeprom=seed_eeprom,
    )


def _answer(run, index: int) -> tuple[int, bytes]:
    frames = protocol.iter_645_frames(run.uart_tx)
    _, control, raw = protocol.parse_frame_645(frames[index])
    return control, bytes((byte - DATA_OFFSET) & 0xFF for byte in raw)


def _answer_count(run) -> int:
    return len(protocol.iter_645_frames(run.uart_tx))


def _oad_frame(oad: int, payload: bytes) -> bytes:
    return _frame(ADDRESS, 0xFA, oad.to_bytes(4, "little") + payload, long_length=True)


def _image_write(process_id: int, payload: bytes) -> bytes:
    return _frame(ADDRESS, 0x04, process_id.to_bytes(2, "little") + PASSWORD + payload)


@case(
    "ACC-645-007-BLOCKED",
    "the 0xFA long-frame upgrade handler accepts a start frame whose 12-byte "
    "header begins with magic 0xEF87, rejects a wrong magic with error 01, "
    "answers one well-formed data frame with 03, echoes the bitmap OAD and an "
    "unknown OAD, and leaves the upgrade parameter block untouched, so a data "
    "frame after a restart returns 01. Image write, verification, activation "
    "and a persistent status flag are absent from upgrade.c, so the full "
    "645-97 flow remains blocked",
    ("645", "upgrade"),
)
def upgrade_frame_protocol(ctx):
    oad_start = 0x304000FF
    oad_data = 0x304000EE
    oad_bitmap = 0x304000DD
    header = (
        (0xEF87).to_bytes(2, "little")
        + (0x1234).to_bytes(2, "little")
        + (16).to_bytes(2, "little")
        + (2).to_bytes(2, "little")
        + (32).to_bytes(4, "little")
    )
    data_payload = b"\x00\x00" + b"\x10\x00" + bytes(range(16))

    first = _run(
        ctx,
        [
            _oad_frame(oad_start, header),
            _oad_frame(oad_data, data_payload),
            # The gatekeeper de-offsets only len-1 payload bytes of a 0xFA
            # frame, so the extra byte keeps the echoed OAD in the
            # corrected region.
            _oad_frame(oad_bitmap, b"\x11\x00"),
            _frame(ADDRESS, 0xFA, b"\x11\x22\x33\x44\x00\x00", long_length=True),
        ],
        cycles=8,
    )
    assert first.exit_code == 0, f"firmware_sim exited {first.exit_code}"
    assert _answer_count(first) == 4, first.uart_tx.hex()
    assert _answer(first, 0) == (0xFA, oad_start.to_bytes(4, "little") + b"\x00")
    assert _answer(first, 1) == (0xFA, oad_data.to_bytes(4, "little") + b"\x03")
    assert _answer(first, 2) == (0xFA, oad_bitmap.to_bytes(4, "little"))
    assert _answer(first, 3) == (0xFA, b"\x11\x22\x33\x44")
    assert first.eeprom[IMAGE_STATE] == b"\xff" * 59, "the handler must not persist upgrade state"
    assert first.eeprom[BACKUP : BACKUP + 59] == b"\xff" * 59
    assert first.non_ff_flash() == 0

    bad_magic = _run(
        ctx,
        [_oad_frame(oad_start, b"\x00\x00" + header[2:])],
        cycles=4,
    )
    assert bad_magic.exit_code == 0, f"firmware_sim exited {bad_magic.exit_code}"
    assert _answer(bad_magic, 0) == (0xFA, oad_start.to_bytes(4, "little") + b"\x01")

    restart = _run(
        ctx,
        [_oad_frame(oad_data, data_payload)],
        cycles=4,
        seed_eeprom=first.eeprom,
    )
    assert restart.exit_code == 0, f"firmware_sim exited {restart.exit_code}"
    assert _answer(restart, 0) == (0xFA, oad_data.to_bytes(4, "little") + b"\x01")


@case(
    "ACC-UPG-002-BLOCKED",
    "the online upgrade handshake accepts transfer init (CB50) and RS485 "
    "196-byte mode (CB51), mirrors the 59-byte state to the backup block, and "
    "rejects the CAFD data frame with error 01 because it is compiled out "
    "under UPGRADE_MODE_ONLINE. Online file identify (CB53) jumps straight to "
    "activation, writes the reset flag and resets the twin with exit code 3 "
    "before answering. Fragmentation, verification and command activation are "
    "absent, so the row stays blocked",
    ("upgrade", "645"),
)
def online_upgrade_flow(ctx):
    init = _image_write(0xCB50, b"\x01")
    mode = _image_write(0xCB51, b"\x02")
    block_header = (2).to_bytes(4, "big") + (1).to_bytes(4, "big")
    data = _image_write(0xCAFD, block_header + bytes(196))

    handshake = _run(ctx, [init, mode, data], cycles=7)
    assert handshake.exit_code == 0, f"firmware_sim exited {handshake.exit_code}"
    assert _answer_count(handshake) == 3, handshake.uart_tx.hex()
    assert _answer(handshake, 0) == (0x84, b"\x50\xcb\x01")
    assert _answer(handshake, 1) == (0x84, b"\x51\xcb\x02")
    assert _answer(handshake, 2) == (0xC4, b"\x01")
    state = handshake.eeprom[IMAGE_STATE]
    assert state[:4] == b"\x02\x02\xc4\x00", state.hex()
    assert handshake.eeprom[BACKUP : BACKUP + 59] == state
    assert handshake.eeprom[RESET_FLAG] == b"\xff" * 4
    assert handshake.non_ff_flash() == 0

    identify = _run(ctx, [init, mode, _image_write(0xCB53, bytes(range(1, 40)))], cycles=6)
    assert identify.exit_code == 3, "online identify must reset the device"
    assert _answer_count(identify) == 2, identify.uart_tx.hex()
    state = identify.eeprom[IMAGE_STATE]
    assert state[:4] == b"\x09\x02\xc4\x00", state.hex()
    assert state[4:43] == bytes(range(1, 40)), state.hex()
    assert state[55:59] == b"\x8d\x7c\x6b\x5a", state.hex()
    assert identify.eeprom[RESET_FLAG] == b"\x8d\x7c\x6b\x5a", identify.eeprom[RESET_FLAG].hex()
    assert identify.eeprom[BACKUP : BACKUP + 59] == state


@case(
    "ACC-UPG-001-BLOCKED",
    "local upgrade mode 0x82 is accepted by the mode and identify steps and "
    "stores the 39-byte file info plus checksum, but the local branch that "
    "clears the image area is compiled out under UPGRADE_MODE_ONLINE. The "
    "CAFD data frame and the CB55 verification id both answer error 01, no "
    "flash byte is written and no reset flag appears, so the row stays blocked",
    ("upgrade", "645"),
)
def local_upgrade_flow(ctx):
    block_header = (2).to_bytes(4, "big") + (1).to_bytes(4, "big")
    run = _run(
        ctx,
        [
            _image_write(0xCB50, b"\x01"),
            _image_write(0xCB51, b"\x82"),
            _image_write(0xCB53, bytes(range(1, 40))),
            _image_write(0xCAFD, block_header + bytes(196)),
            _image_write(0xCB55, b"\x00\x00"),
        ],
        cycles=9,
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _answer_count(run) == 5, run.uart_tx.hex()
    assert _answer(run, 0) == (0x84, b"\x50\xcb\x01")
    assert _answer(run, 1) == (0x84, b"\x51\xcb\x82")
    assert _answer(run, 2) == (0x84, b"\x53\xcb")
    assert _answer(run, 3) == (0xC4, b"\x01")
    assert _answer(run, 4) == (0xC4, b"\x01")
    state = run.eeprom[IMAGE_STATE]
    assert state[:4] == b"\x03\x82\xc4\x00", state.hex()
    assert state[4:43] == bytes(range(1, 40)), state.hex()
    assert state[51:59] == b"\x00" * 8, state.hex()
    assert run.eeprom[RESET_FLAG] == b"\xff" * 4
    assert run.eeprom[BACKUP : BACKUP + 59] == state
    assert run.non_ff_flash() == 0
