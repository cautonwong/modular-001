"""Load-control acceptance rows ACC-LC-002/004/006/008.

Evidence is external only: 698 response APDUs, relay.log, backlight.log and
EEPROM bytes. The two BLOCKED cases pin seams that are absent from the twin;
they pass by asserting the current (limited) behavior, the convention used by
the other *-BLOCKED cases in this suite.
"""

from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6
ZERO = b"\x00"
# 2099-12-31 23:59:59 plus a zero interval. Outside factory mode the control
# actions require a valid time tag; this one is future-dated so it passes
# dl_data_time_check in application/adapters/dl698/fr_698_frame.c.
TIME_TAG = bytes.fromhex("0108330c1f173b3b020000")

TRIP = bytes.fromhex("0205110005f205020111001200001100")
CLOSE = bytes.fromhex("0205110005f205020111011200001100")

STATUS = 0x80000500
ALARM = 0x80000400
BAODIAN = 0x80010200
FACTORY_MODE = 0xFFFF0200


def _frame(apdu: bytes) -> bytes:
    return protocol.build_frame(apdu, sa=BLANK_SA, ca=0x10, ctrl=0x43)


def _get(piid: int, oad: int) -> str:
    return _frame(protocol.get_request(piid, oad)).hex()


def _set(piid: int, oad: int, data: bytes) -> str:
    return _frame(protocol.set_request(piid, oad, data)).hex()


def _action(piid: int, oi: int, method: int, param: bytes | None = None) -> str:
    return _frame(protocol.action_request(piid, oi, method, param=param)).hex()


def _timed_action(piid: int, oi: int, method: int, param: bytes | None = None) -> str:
    apdu = (
        bytes([0x07, 0x01, piid])
        + oi.to_bytes(2, "big")
        + bytes([method, 0])
        + (param or b"")
        + TIME_TAG
    )
    return _frame(apdu).hex()


def _responses(run):
    responses = []
    for frame in protocol.iter_698_frames(run.uart_tx):
        apdu = protocol.parse_frame(frame).apdu
        if apdu[0] == 0x87:
            responses.append(("action", protocol.parse_action_response(apdu)))
        elif apdu[0] == 0x85 and apdu[1] == 0x01:
            responses.append(("get", protocol.parse_get_response(apdu)))
        elif apdu[0] == 0x86:
            piid, dar = protocol.parse_set_response(apdu)
            assert dar == 0, (piid, dar)
        else:
            raise AssertionError(f"unexpected APDU {apdu.hex()}")
    return responses


def _relay(run) -> list[tuple[int, str]]:
    entries = []
    for line in run.relay_log:
        fields = line.split()
        assert fields[0] == "cycle" and fields[2] == "time", line
        assert fields[5] == "action" and fields[6] in ("connect", "disconnect"), line
        entries.append((int(fields[1]), fields[6]))
    return entries


def _backlight(run) -> list[tuple[int, int]]:
    entries = []
    for line in run.backlight_log:
        fields = line.split()
        assert fields[0] == "cycle" and fields[2] == "time", line
        assert fields[5] == "value" and fields[6] in ("0", "1"), line
        entries.append((int(fields[1]), int(fields[6])))
    return entries


@case(
    "ACC-LC-002",
    "保电 mode reports state 1, rejects a remote trip with DAR 255 while the "
    "relay stays connected, alarm on/off read back as 04 08 80 / 04 08 00, and "
    "after releasing 保电 the trip succeeds and the relay log disconnects",
    ("load-control", "relay", "698"),
)
def baodian_and_alarm(ctx):
    scenario = (
        "at 0 meter voltage 220\n"
        "at 0 meter current 5\n"
        f"at 1 uart 0 {_action(1, 0x8001, 0x7F, ZERO)}\n"
        f"at 2 uart 0 {_get(2, BAODIAN)}\n"
        f"at 3 uart 0 {_action(3, 0x8000, 0x81, TRIP)}\n"
        f"at 4 uart 0 {_get(4, STATUS)}\n"
        f"at 5 uart 0 {_action(5, 0x8001, 0x80, ZERO)}\n"
        f"at 6 uart 0 {_get(6, BAODIAN)}\n"
        f"at 7 uart 0 {_action(7, 0x8000, 0x7F, ZERO)}\n"
        f"at 8 uart 0 {_get(8, ALARM)}\n"
        f"at 9 uart 0 {_action(9, 0x8000, 0x80, ZERO)}\n"
        f"at 10 uart 0 {_get(10, ALARM)}\n"
        f"at 11 uart 0 {_action(11, 0x8000, 0x81, TRIP)}\n"
        f"at 13 uart 0 {_get(12, STATUS)}\n"
    )
    run = ctx.run(cycles=16, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = _responses(run)
    assert [kind for kind, _ in responses] == [
        "action", "get", "action", "get", "action", "get", "action", "get",
        "action", "get", "action", "get",
    ], [kind for kind, _ in responses]

    assert responses[0][1].dar == 0
    assert responses[1][1].data == bytes.fromhex("1601")
    assert responses[2][1].dar == 255
    assert responses[3][1].data == bytes.fromhex("040800")
    assert responses[4][1].dar == 0
    assert responses[5][1].data == bytes.fromhex("1600")
    assert responses[6][1].dar == 0
    assert responses[7][1].data == bytes.fromhex("040880")
    assert responses[8][1].dar == 0
    assert responses[9][1].data == bytes.fromhex("040800")
    assert responses[10][1].dar == 0
    assert responses[11][1].data == bytes.fromhex("040880")

    disconnects = [cycle for cycle, action in _relay(run) if action == "disconnect"]
    assert disconnects, run.relay_log
    assert min(disconnects) >= 11, run.relay_log


@case(
    "ACC-LC-004",
    "an over-current threshold of 1.000 A and a 1-minute delay, with 2 A "
    "flowing, trip at cycle 4 and only disconnect after the delay; dropping "
    "the current and closing reconnects the relay",
    ("load-control", "protection", "relay"),
)
def overcurrent_protection(ctx):
    scenario = (
        "at 0 meter voltage 220\n"
        "at 0 meter current 2\n"
        f"at 1 uart 0 {_set(1, 0x80000201, bytes.fromhex('05') + (10000).to_bytes(4, 'big'))}\n"
        f"at 2 uart 0 {_set(2, 0x80000202, bytes.fromhex('12') + (1).to_bytes(2, 'big'))}\n"
        f"at 4 uart 0 {_timed_action(3, 0x8000, 0x81, TRIP)}\n"
        f"at 5 uart 0 {_get(4, STATUS)}\n"
        "at 70 meter current 0\n"
        f"at 72 uart 0 {_timed_action(5, 0x8000, 0x82, CLOSE)}\n"
        f"at 75 uart 0 {_get(6, STATUS)}\n"
        f"at 78 uart 0 {_get(7, 0x80000202)}\n"
    )
    run = ctx.run(cycles=82, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = _responses(run)
    assert [kind for kind, _ in responses] == [
        "action", "get", "action", "get", "get",
    ], [kind for kind, _ in responses]
    assert responses[0][1].dar == 0
    assert responses[1][1].data == bytes.fromhex("040880")
    assert responses[2][1].dar == 0
    assert responses[3][1].data == bytes.fromhex("040800")
    assert responses[4][1].data == bytes.fromhex("120001")

    relay = _relay(run)
    disconnects = [cycle for cycle, action in relay if action == "disconnect"]
    connects = [cycle for cycle, action in relay if action == "connect"]
    assert len(disconnects) == 1, relay
    assert 55 <= disconnects[0] - 4 <= 65, relay
    assert connects == [72], relay


@case(
    "ACC-LC-006",
    "after a trip and a close-permit command, holding the local key for three "
    "seconds connects the relay at cycle 9 and the key lights the backlight",
    ("load-control", "key", "backlight"),
)
def key_control(ctx):
    scenario = (
        "at 0 meter voltage 220\n"
        "at 0 meter current 5\n"
        f"at 2 uart 0 {_action(1, 0x8000, 0x81, TRIP)}\n"
        f"at 4 uart 0 {_action(2, 0x8000, 0x82, TRIP)}\n"
        "at 6 key press\n"
        "at 10 key release\n"
        f"at 12 uart 0 {_get(3, STATUS)}\n"
    )
    run = ctx.run(cycles=16, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = _responses(run)
    assert [kind for kind, _ in responses] == ["action", "action", "get"]
    assert responses[0][1].dar == 0 and responses[1][1].dar == 0
    assert responses[2][1].data == bytes.fromhex("040800")

    relay = _relay(run)
    assert (2, "disconnect") in relay, relay
    connects = [cycle for cycle, action in relay if action == "connect"]
    assert connects and connects[0] == 9, relay

    backlight = _backlight(run)
    assert backlight[6] == (6, 0), backlight
    assert all(value == 1 for cycle, value in backlight if cycle >= 7), backlight


@case(
    "ACC-LC-008",
    "after power-on initializes the voltage window and factory mode is exited, "
    "the relay does not close at 300 V or 100 V and closes only after the "
    "voltage returns to 220 V",
    ("load-control", "safety-gate", "relay"),
)
def voltage_gate(ctx):
    scenario = (
        "at 0 meter voltage 220\n"
        "at 0 meter current 5\n"
        "at 1 power off\n"
        "at 3 power on\n"
        "at 4 meter voltage 220\n"
        "at 4 meter current 5\n"
        f"at 5 uart 0 {_set(1, FACTORY_MODE, ZERO)}\n"
        f"at 10 uart 0 {_timed_action(2, 0x8000, 0x81, TRIP)}\n"
        "at 13 meter voltage 300\n"
        f"at 14 uart 0 {_timed_action(3, 0x8000, 0x82, CLOSE)}\n"
        "at 17 meter voltage 100\n"
        f"at 18 uart 0 {_timed_action(4, 0x8000, 0x82, CLOSE)}\n"
        "at 20 meter voltage 220\n"
        f"at 21 uart 0 {_timed_action(5, 0x8000, 0x82, CLOSE)}\n"
        f"at 24 uart 0 {_get(6, STATUS)}\n"
    )
    run = ctx.run(cycles=28, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = _responses(run)
    assert [kind for kind, _ in responses] == [
        "action", "action", "action", "action", "get",
    ], [kind for kind, _ in responses]
    assert all(response.dar == 0 for _, response in responses[:-1])
    assert responses[-1][1].data == bytes.fromhex("040800")

    relay = _relay(run)
    disconnects = [cycle for cycle, action in relay if action == "disconnect"]
    connects = [cycle for cycle, action in relay if action == "connect"]
    assert disconnects and min(disconnects) >= 10, relay
    assert not [cycle for cycle in connects if 13 <= cycle <= 19], relay
    assert connects and connects[-1] >= 20, relay


@case(
    "ACC-LC-008-BLOCKED",
    "outside factory mode an alarm-on state does not gate a remote close: the "
    "close is accepted and the relay connects, so the alarm half of the safety "
    "gate has no firmware path",
    ("load-control", "safety-gate", "alarm"),
)
def alarm_does_not_gate_close(ctx):
    scenario = (
        "at 0 meter voltage 220\n"
        "at 0 meter current 5\n"
        "at 1 power off\n"
        "at 3 power on\n"
        "at 4 meter voltage 220\n"
        "at 4 meter current 5\n"
        f"at 5 uart 0 {_set(1, FACTORY_MODE, ZERO)}\n"
        f"at 10 uart 0 {_timed_action(2, 0x8000, 0x81, TRIP)}\n"
        f"at 12 uart 0 {_timed_action(3, 0x8000, 0x7F, ZERO)}\n"
        f"at 14 uart 0 {_timed_action(4, 0x8000, 0x82, CLOSE)}\n"
        f"at 16 uart 0 {_get(5, STATUS)}\n"
        f"at 17 uart 0 {_get(6, ALARM)}\n"
    )
    run = ctx.run(cycles=20, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = _responses(run)
    assert [kind for kind, _ in responses] == [
        "action", "action", "action", "get", "get",
    ], [kind for kind, _ in responses]
    assert responses[0][1].dar == 0
    assert responses[1][1].dar == 0, "alarm-on must be accepted"
    assert responses[2][1].dar == 0, "close must be accepted"
    assert responses[3][1].data == bytes.fromhex("040800")
    assert responses[4][1].data == bytes.fromhex("040880")

    connects = [cycle for cycle, action in _relay(run) if action == "connect"]
    assert connects and connects[-1] >= 14, run.relay_log


@case(
    "ACC-LC-005",
    "with the relay commanded off but feedback still closed and current "
    "flowing, the misoperation event starts and carries its timestamp",
    ("load-control", "feedback", "relay"),
)
def relay_feedback_misoperation(ctx):
    scenario = (
        "at 0 relay_feedback on\n"
        "at 0 meter voltage 220\n"
        "at 0 meter current 5\n"
        f"at 2 uart 0 {_timed_action(1, 0x8000, 0x81, TRIP)}\n"
        f"at 8 uart 0 {_get(2, 0x302B0400)}\n"
        f"at 9 uart 0 {_get(3, 0x302B0A00)}\n"
    )
    run = ctx.run(cycles=12, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = _responses(run)
    assert [kind for kind, _ in responses] == ["action", "get", "get"], [
        kind for kind, _ in responses
    ]
    assert responses[1][1].data.hex() == "120001"
    value = responses[2][1].data
    assert value.count(b"\x1c") == 1, value.hex()
