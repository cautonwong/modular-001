from __future__ import annotations

from cases import case
from sim import EEPROM_SIZE

import parameters
import protocol

# A blank EEPROM leaves the meter TSA at FF FF FF FF FF FF. The protocol
# builders reject that address and the FF password bytes, so 645 requests are
# framed locally the way test_dl645.py does.
BLANK_ADDRESS = b"\xff" * 6

# A blank EEPROM leaves Pass02Err_Cnt at 0xFF, which Cmp_Password() treats as a
# permanent lockout. Seed it to 0 so the plaintext password is compared
# (fr_645.c:557, test_dl645.py:15-22).
PASS02_ERR_CNT_OFFSET = 18138

STATUS_OAD = 0x80000500
SWITCH_OFF_COUNT = 0x301F0400
SWITCH_ON_COUNT = 0x30200400


def _seed(**overrides) -> bytes:
    seed = bytearray([0xFF]) * EEPROM_SIZE
    seed[PASS02_ERR_CNT_OFFSET] = 0
    values = {"remote_relay_off_time": 0}
    values.update(overrides)
    return parameters.set_parameters(bytes(seed), values)


def _698(piid: int, oad: int) -> str:
    return protocol.build_frame(
        protocol.get_request(piid, oad), sa=b"\xff" * 6, ca=0x10, ctrl=0x43
    ).hex()


def _gets(run) -> list[bytes]:
    values = []
    for frame in protocol.iter_698_frames(run.uart_tx):
        apdu = protocol.parse_frame(frame).apdu
        if apdu[0] == 0x85:
            values.append(protocol.parse_get_response(apdu).data)
    return values


def _count(data: bytes) -> int:
    # class 7 event count: long-unsigned <count> (get_current_record_number)
    assert data[0] == 0x12 and len(data) == 3, data.hex()
    return int.from_bytes(data[1:3], "big")


def _frame(address: bytes, control: int, data: bytes) -> bytes:
    # The factory-mode value 0xFB plus the 0x33 offset overflows a byte, which
    # protocol._escape refuses; the firmware only subtracts the offset.
    body = bytes([0x68]) + address + bytes([0x68, control, len(data)])
    body += bytes((byte + protocol.DATA_OFFSET) & 0xFF for byte in data)
    return body + bytes([protocol.checksum(body), 0x16])


def _factory_frame() -> bytes:
    data = (
        protocol.data_id_bytes("A8000100")
        + b"\x02\x00\x00\x00"
        + b"\x00\x00\x00\x00"
        + b"\xfb"
    )
    return _frame(BLANK_ADDRESS, 0x14, data)


def _control(command: int, valid_until: bytes) -> bytes:
    data = (
        b"\x02\xff\xff\xff"
        + b"\x00\x00\x00\x00"
        + bytes([command])
        + b"\x00"
        + valid_until
    )
    return _frame(BLANK_ADDRESS, 0x1C, data)


def _645(run) -> list[bytes]:
    return protocol.iter_645_frames(run.uart_tx)


def _parse_645(frame: bytes) -> tuple[str, int, bytes]:
    address, control, raw = protocol.parse_frame_645(frame)
    return address, control, bytes((byte - protocol.DATA_OFFSET) & 0xFF for byte in raw)


@case(
    "ACC-645-003",
    "a 645 trip command (C=1C, N1=1A) drops the relay and records SwitchOffS; "
    "a direct close (N1=1C) picks the relay up and records SwitchOnS",
    ("645", "control", "relay"),
)
def relay_control_645(ctx):
    factory = _factory_frame()
    trip = _control(protocol.CMD_TRIP, bytes.fromhex("000412300421"))
    close = _control(0x1C, bytes.fromhex("000412300421"))

    scenario = (
        f"at 1 uart 0 {factory.hex()}\n"
        f"at 2 uart 0 {trip.hex()}\n"
        f"at 3 uart 0 {_698(1, STATUS_OAD)}\n"
        f"at 4 uart 0 {_698(2, SWITCH_OFF_COUNT)}\n"
        f"at 6 uart 0 {close.hex()}\n"
        f"at 7 uart 0 {_698(3, STATUS_OAD)}\n"
        f"at 8 uart 0 {_698(4, SWITCH_ON_COUNT)}\n"
    )
    run = ctx.run(
        cycles=10,
        seed_eeprom=_seed(),
        scenario=scenario,
        start_time="2021-04-30 12:00:00",
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    frames = _645(run)
    assert len(frames) == 3, f"expected three 645 responses: {run.uart_tx.hex()}"
    controls = []
    for frame in frames:
        address, control, data = _parse_645(frame)
        assert address == "ffffffffffff", address
        controls.append(control)
    assert controls == [0x94, 0x9C, 0x9C], controls

    gets = _gets(run)
    assert len(gets) == 4, f"expected four 698 responses: {run.uart_tx.hex()}"
    assert gets[0] == bytes.fromhex("040880"), gets[0].hex()
    assert _count(gets[1]) == 1, gets[1].hex()
    assert gets[2] == bytes.fromhex("040800"), gets[2].hex()
    assert _count(gets[3]) == 1, gets[3].hex()

    actions = [
        (int(parts[1]), parts[-1]) for parts in (line.split() for line in run.relay_log)
    ]
    # Cycle 0 re-asserts the relay at power-on; the injected commands follow.
    assert actions[-2:] == [(2, "disconnect"), (6, "connect")], run.relay_log


RTC_OAD = 0x40000200
RTC_FAULT_COUNT = 0x302E0400
RTC_FAULT_VALUE = 0x302E0A00


def _time_set(piid: int, value: str) -> str:
    return protocol.build_frame(
        protocol.set_request(piid, RTC_OAD, bytes.fromhex(value)),
        sa=b"\xff" * 6,
        ca=0x10,
        ctrl=0x43,
    ).hex()


def _set_dar(run) -> int:
    for frame in protocol.iter_698_frames(run.uart_tx):
        apdu = protocol.parse_frame(frame).apdu
        if apdu[0] == 0x86:
            return protocol.parse_set_response(apdu)[1]
    raise AssertionError("no SET response")


@case(
    "ACC-RTC-003",
    "a 698 time set updates the clock and the following GET returns the new "
    "date and time",
    ("rtc", "698", "time"),
)
def clock_set_via_698(ctx):
    scenario = (
        f"at 2 uart 0 {_time_set(1, '1c07ea0b0c0c0000')}\n"
        f"at 4 uart 0 {_698(2, RTC_OAD)}\n"
    )
    run = ctx.run(cycles=6, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _set_dar(run) == 0

    data = _gets(run)[0]
    assert data.hex() == "1c07ea0b0c0c0003", data.hex()


@case(
    "ACC-RTC-004",
    "a backward clock jump across the saved power-down time starts the clock "
    "fault event; a later 698 time set ends it and a clean power cycle does "
    "not raise it again",
    ("rtc", "events", "clock-fault"),
)
def clock_fault_and_recovery(ctx):
    scenario = (
        "at 1 power off\n"
        "at 3 time '2026-11-11 11:00:00'\n"
        "at 5 power on\n"
        f"at 7 uart 0 {_698(1, RTC_FAULT_COUNT)}\n"
        f"at 8 uart 0 {_698(2, RTC_FAULT_VALUE)}\n"
        f"at 10 uart 0 {_time_set(3, '1c07ea0b0c0c1e00')}\n"
        f"at 12 uart 0 {_698(4, RTC_FAULT_COUNT)}\n"
        f"at 13 uart 0 {_698(5, RTC_FAULT_VALUE)}\n"
        "at 15 power off\n"
        "at 17 power on\n"
        f"at 19 uart 0 {_698(6, RTC_FAULT_COUNT)}\n"
    )
    run = ctx.run(
        cycles=21, scenario=scenario, start_time="2026-11-12 12:00:10"
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = [
        protocol.parse_frame(frame).apdu for frame in protocol.iter_698_frames(run.uart_tx)
    ]
    assert len(responses) == 6, [apdu.hex() for apdu in responses]
    assert _count(protocol.parse_get_response(responses[0]).data) == 1
    started = protocol.parse_get_response(responses[1]).data
    assert started.count(b"\x1c") == 1, started.hex()

    piid, dar = protocol.parse_set_response(responses[2])
    assert (piid, dar) == (3, 0), (piid, dar)

    assert _count(protocol.parse_get_response(responses[3]).data) == 1
    ended = protocol.parse_get_response(responses[4]).data
    assert ended.count(b"\x1c") == 2, ended.hex()
    assert _count(protocol.parse_get_response(responses[5]).data) == 1
