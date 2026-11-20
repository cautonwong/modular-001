"""Acceptance case ACC-POWER-002: meter state restored after an AC outage.

The energy half lived in test_inputs.py; the case moved here so one case owns
the whole row and the relay half can share the power-cycle scenario grammar.
Evidence is external only: 645/698 response bytes, EEPROM bytes, relay capture.

The demand half runs in the same case: a five-minute window seeds a maximum,
the outage saves it, and the next power on clears the current value while
keeping the maximum and starting a fresh window at the restore time, observed
through placeholder OAD 0x11000201.
"""

from __future__ import annotations

from cases import case

import parameters
import protocol
import storage

BLANK_ADDRESS = b"\xff" * 6
BLANK_IMAGE = b"\xff" * storage.EEPROM_SIZE
METER_CONST = 3200
PULSES_PER_STEP = METER_CONST // 100

# Persistent EEPROM block layout (meter_record.h RELAY group): cmd, mode, sta,
# one alignment pad, then alarmmin.  sta is 241 bytes into the block and
# relay_do, which storage.py decodes, is byte 260.
RELAY_STA_OFFSET = storage.EEPROM_PERSISTENT_BLOCK_OFFSET + 241

# A blank Pass02Err_Cnt is 0xFF, which Cmp_Password() treats as a permanent
# lockout (fr_645.c:557).  The trip runs in factory mode and skips the compare,
# but seed it as ACC-645-003 does so the precondition is explicit.
PASS02_ERR_CNT_OFFSET = 18138
STATUS_OAD = 0x80000500
CONTROL_VALID_UNTIL = bytes.fromhex("000412300421")  # 2021-04-30 12:04:00


def _get(piid: int, oad: int) -> str:
    return protocol.build_frame(
        protocol.get_request(piid, oad), sa=BLANK_ADDRESS, ca=0x10, ctrl=0x43
    ).hex()


def _frame(control: int, data: bytes) -> bytes:
    # 0xFB + 0x33 overflows a byte, which protocol._escape refuses; the
    # firmware only subtracts the offset, so frame locally (test_dl645.py).
    body = bytes([0x68]) + BLANK_ADDRESS + bytes([0x68, control, len(data)])
    body += bytes((byte + protocol.DATA_OFFSET) & 0xFF for byte in data)
    return body + bytes([protocol.checksum(body), 0x16])


def _factory_frame() -> bytes:
    return _frame(
        0x14,
        protocol.data_id_bytes("A8000100")
        + b"\x02\x00\x00\x00"
        + b"\x00" * 4
        + b"\xfb",
    )


def _trip_frame() -> bytes:
    return _frame(
        0x1C,
        b"\x02\xff\xff\xff"
        + b"\x00" * 4
        + bytes([protocol.CMD_TRIP])
        + b"\x00"
        + CONTROL_VALID_UNTIL,
    )


def _get_values(run):
    values = []
    for frame in protocol.iter_698_frames(run.uart_tx):
        values.extend(protocol.parse_get_response(protocol.parse_frame(frame).apdu).values())
    return values


def _clock(payload: bytes) -> tuple[int, ...]:
    return (payload[0] << 8 | payload[1], payload[2], payload[3],
            payload[4], payload[5], payload[6])


def _clock_text(text: str) -> tuple[int, ...]:
    return (int(text[0:4]), int(text[5:7]), int(text[8:10]),
            int(text[11:13]), int(text[14:16]), int(text[17:19]))


@case(
    "ACC-POWER-002",
    "active energy saved at power loss is restored after the next power on, and "
    "a 645 trip before the outage leaves the relay command off after power on, "
    "observed through 698 OAD 0x80000500 and the persistent EEPROM sta byte",
    ("inputs", "power", "persistence", "relay"),
)
def power_restore(ctx):
    seed = ctx.run(cycles=4)
    assert seed.exit_code == 0, f"seed run exited {seed.exit_code}"

    energy_scenario = (
        "at 0 energy clear\n"
        f"at 1 meter const {METER_CONST} cal 128\n"
        f"at 1 pulse {PULSES_PER_STEP}\n"
        f"at 3 uart 0 {_get(1, 0x00000401)}\n"
        "at 5 power off\n"
        "at 7 power on\n"
        f"at 8 meter const {METER_CONST} cal 128\n"
        f"at 10 uart 0 {_get(2, 0x00000401)}\n"
    )
    run = ctx.run(cycles=12, seed_eeprom=seed.eeprom, scenario=energy_scenario)
    assert run.exit_code == 0, f"power run exited {run.exit_code}"

    before, after = _get_values(run)
    assert before.value == 100, before.value
    assert after.value == 100, after.value

    relay_seed = bytearray(
        parameters.set_parameters(BLANK_IMAGE, {"remote_relay_off_time": 0})
    )
    relay_seed[PASS02_ERR_CNT_OFFSET] = 0
    relay_scenario = (
        f"at 1 uart 0 {_factory_frame().hex()}\n"
        f"at 2 uart 0 {_trip_frame().hex()}\n"
        f"at 4 uart 0 {_get(1, STATUS_OAD)}\n"
        "at 6 power off\n"
        "at 8 power on\n"
        f"at 10 uart 0 {_get(2, STATUS_OAD)}\n"
    )
    relay = ctx.run(
        cycles=13,
        seed_eeprom=bytes(relay_seed),
        scenario=relay_scenario,
        start_time="2021-04-30 12:00:00",
    )
    assert relay.exit_code == 0, f"relay run exited {relay.exit_code}"

    frames = protocol.iter_645_frames(relay.uart_tx)
    assert len(frames) == 2, f"expected two 645 responses: {relay.uart_tx.hex()}"
    controls = [protocol.parse_frame_645(frame)[1] for frame in frames]
    assert controls == [0x94, 0x9C], controls

    states = _get_values(relay)
    assert len(states) == 2, f"expected two 698 responses: {relay.uart_tx.hex()}"
    assert states[0].value == b"\x80", states[0].value
    assert states[1].value == b"\x80", states[1].value

    assert relay.eeprom[RELAY_STA_OFFSET] == 0x02, relay.eeprom[RELAY_STA_OFFSET]
    assert storage.parameter_block(relay.eeprom)["relay_do"].value == 0

    actions = [line.split() for line in relay.relay_log]
    assert any(
        parts[1] == "2" and parts[-1] == "disconnect" for parts in actions
    ), relay.relay_log

    demand_scenario = (
        "at 0 meter power 100\n"
        "at 305 power off\n"
        "at 310 power on\n"
        f"at 315 uart 0 {_get(1, 0x11000201)}\n"
    )
    demand = ctx.run(
        cycles=320,
        scenario=demand_scenario,
        start_time="2024-08-28 16:40:00",
    )
    assert demand.exit_code == 0, f"demand run exited {demand.exit_code}"

    values = _get_values(demand)
    assert len(values) == 1, f"expected one 698 response: {demand.uart_tx.hex()}"
    assert values[0].tag == protocol.AXDR_STRUCTURE, values[0].tag
    current, fee, maximum, max_fee, max_time, start_time = values[0].children
    assert current.value == 0, current.value
    assert fee.value == 0, fee.value
    assert maximum.value == 1000, maximum.value
    assert max_fee.value == 1000, max_fee.value
    assert _clock(max_time.value) == (2024, 8, 28, 16, 45, 0)
    restore = next(cycle.time for cycle in demand.cycles if cycle.index == 310)
    assert _clock(start_time.value) == _clock_text(restore), start_time.value
