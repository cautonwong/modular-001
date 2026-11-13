"""Fault events: relay misoperation and clock fault.

Both checks read stored event records through 698 and the relay capture.
"""

from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6


def _frame(apdu: bytes, ctrl: int = 0x43) -> bytes:
    return protocol.build_frame(apdu, sa=BLANK_SA, ca=0x10, ctrl=ctrl)


def _get(piid: int, oad: int) -> bytes:
    return _frame(protocol.get_request(piid, oad))


def _set_time(piid: int, value: str) -> str:
    return _frame(protocol.set_request(piid, 0x40000200, bytes.fromhex(value))).hex()


def _action(piid: int, method: int, param: bytes) -> bytes:
    return _frame(protocol.action_request(piid, RELAY_OI, method, param=param))


def _record_request(piid: int, oad: int, rsd: bytes) -> bytes:
    """GET-Request-Record: 05 | 03 | PIID | OAD | RSD | RCSD-null | time tag."""
    apdu = bytes([0x05, 0x03, piid]) + oad.to_bytes(4, "big") + rsd + b"\x00" + b"\x00"
    return _frame(apdu)


def _apdus(run) -> list[bytes]:
    return [protocol.parse_frame(frame).apdu for frame in protocol.iter_698_frames(run.uart_tx)]


def _record(apdu: bytes) -> tuple[int, bytes]:
    assert apdu[0] == 0x85 and apdu[1] == 0x03, apdu.hex()
    position = 8 + apdu[7] * 5
    if apdu[position] != 0x01:
        raise AssertionError(f"record request failed: {apdu.hex()}")
    return apdu[position + 1], apdu[position + 2 :]


def _one_record(apdu: bytes) -> bytes:
    count, body = _record(apdu)
    assert count == 1, (count, apdu.hex())
    return body


def _relay_actions(run) -> list[tuple[int, str]]:
    return [(int(line.split()[1]), line.split()[6]) for line in run.relay_log]


LAST_N = lambda n: b"\x09" + bytes([n])

# 698 control: OI 0x8000, method 0x81 disconnect / 0x82 connect.
RELAY_OI = 0x8000
TRIP = bytes.fromhex("0205110005f205020111001200001100")
CLOSE = bytes.fromhex("0205110005f205020111011200001100")

# Relay misoperation event: records 0x302B0200, count 0x302B0400, value 0x302B0A00.
SWITCH_ERR = 0x302B0200
SWITCH_ERR_COUNT = 0x302B0400
SWITCH_ERR_VALUE = 0x302B0A00

# RTC fault event: records 0x302E0200, count 0x302E0400, value 0x302E0A00.
RTC_ERR = 0x302E0200
RTC_ERR_COUNT = 0x302E0400
RTC_ERR_VALUE = 0x302E0A00


@case(
    "ACC-EVT-006",
    "after a remote trip, injecting active power while the relay is commanded "
    "off starts the relay-misoperation event, and a remote close ends it: the "
    "event count, the tagged value with its start and end times, the stored "
    "record, and the relay capture all show the mismatch and its recovery",
    ("events", "relay", "698"),
)
def relay_misoperation(ctx):
    scenario = [f"at 2 uart 0 {_action(1, 0x81, TRIP).hex()}"]
    for cycle in range(2, 19):
        scenario.append(f"at {cycle} meter power 500")
    scenario += [
        f"at 10 uart 0 {_get(2, SWITCH_ERR_COUNT).hex()}",
        f"at 16 uart 0 {_get(3, SWITCH_ERR_VALUE).hex()}",
        f"at 18 uart 0 {_action(4, 0x82, CLOSE).hex()}",
        f"at 24 uart 0 {_get(5, SWITCH_ERR_VALUE).hex()}",
        f"at 26 uart 0 {_record_request(6, SWITCH_ERR, LAST_N(1)).hex()}",
    ]
    run = ctx.run(cycles=30, scenario="\n".join(scenario) + "\n")
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    actions = _relay_actions(run)
    assert (2, "disconnect") in actions, actions
    assert (18, "connect") in actions, actions

    apdus = _apdus(run)
    assert len(apdus) == 6, [apdu.hex() for apdu in apdus]

    count = protocol.parse_get_response(apdus[1])
    assert count.oad == SWITCH_ERR_COUNT
    assert count.data.hex() == "120001", count.data.hex()

    active = protocol.parse_get_response(apdus[2])
    assert active.oad == SWITCH_ERR_VALUE
    assert active.data.hex() == "010102020002021c07e5041e173b3900", active.data.hex()

    assert protocol.parse_action_response(apdus[0]).dar == 0, apdus[0].hex()
    assert protocol.parse_action_response(apdus[3]).dar == 0, apdus[3].hex()

    ended = protocol.parse_get_response(apdus[4])
    assert ended.oad == SWITCH_ERR_VALUE
    assert ended.data.hex() == (
        "010102020002021c07e5041e173b39" "1c07e5050100000d"
    ), ended.data.hex()

    record = _one_record(apdus[5])
    assert record.hex() == (
        "06ffffffff1c07e5041e173b39" "1c07e5050100000d" "00000000"
    ), record.hex()


@case(
    "ACC-EVT-007",
    "a backward clock jump across a saved power-down time starts the clock-"
    "fault event, and a later 698 time set ends it: the count stays one, the "
    "value gains an end timestamp, and the stored record repeats the start",
    ("events", "rtc", "698"),
)
def clock_fault_after_backward_jump(ctx):
    run = ctx.run(
        cycles=16,
        start_time="2026-11-12 12:00:10",
        scenario=(
            "at 1 power off\n"
            "at 3 time '2026-11-11 11:00:00'\n"
            "at 5 power on\n"
            f"at 7 uart 0 {_get(1, RTC_ERR_COUNT).hex()}\n"
            f"at 8 uart 0 {_get(2, RTC_ERR_VALUE).hex()}\n"
            f"at 9 uart 0 {_record_request(3, RTC_ERR, LAST_N(1)).hex()}\n"
            f"at 10 uart 0 {_set_time(4, '1c07ea0b0c0c1e00')}\n"
            f"at 12 uart 0 {_get(5, RTC_ERR_COUNT).hex()}\n"
            f"at 13 uart 0 {_get(6, RTC_ERR_VALUE).hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    apdus = _apdus(run)
    assert len(apdus) == 6, [apdu.hex() for apdu in apdus]

    count = protocol.parse_get_response(apdus[0])
    assert count.oad == RTC_ERR_COUNT
    assert count.data.hex() == "120001", count.data.hex()

    started = protocol.parse_get_response(apdus[1]).data
    assert started.count(b"\x1c") == 1, started.hex()

    record = _one_record(apdus[2])
    assert record.startswith(bytes.fromhex("06ffffffff1c")), record.hex()

    piid, dar = protocol.parse_set_response(apdus[3])
    assert (piid, dar) == (4, 0), (piid, dar)
    assert protocol.parse_get_response(apdus[4]).data.hex() == "120001"
    ended = protocol.parse_get_response(apdus[5]).data
    assert ended.count(b"\x1c") == 2, ended.hex()


@case(
    "ACC-EVT-016",
    "a missing metering chip raises the chip-fault event after its debounce "
    "and the event carries a start timestamp",
    ("events", "metering", "fault"),
)
def metering_chip_event(ctx):
    run = ctx.run(
        cycles=304,
        scenario=(
            "at 1 chip ok\n"
            "at 2 chip fault\n"
            f"at 300 uart 0 {_get(1, 0x302F0400).hex()}\n"
            f"at 301 uart 0 {_get(2, 0x302F0A00).hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    apdus = _apdus(run)
    assert len(apdus) == 2, [apdu.hex() for apdu in apdus]
    assert protocol.parse_get_response(apdus[0]).data.hex() == "120001"
    value = protocol.parse_get_response(apdus[1]).data
    assert value.count(b"\x1c") == 1, value.hex()
