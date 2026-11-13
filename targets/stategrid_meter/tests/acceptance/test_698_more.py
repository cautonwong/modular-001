"""DL/T 698.45 acceptance rows beyond the basic GET/SET/ACTION cases.

Every check reads an external artifact: response APDUs, UART TX bytes, or
EEPROM bytes. Record responses need their own parser because protocol.py only
parses GET choice 1.
"""

from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6


def _frame(apdu: bytes, ctrl: int = 0x43) -> bytes:
    return protocol.build_frame(apdu, sa=BLANK_SA, ca=0x10, ctrl=ctrl)


def _get(piid: int, oad: int) -> bytes:
    return _frame(protocol.get_request(piid, oad))


def _set(piid: int, oad: int, data: bytes) -> bytes:
    return _frame(protocol.set_request(piid, oad, data))


def _action(piid: int, oi: int, method: int, param: bytes | None = None) -> bytes:
    return _frame(protocol.action_request(piid, oi, method, mode=0, param=param))


def _record_request(piid: int, oad: int, rsd: bytes) -> bytes:
    """GET-Request-Record: 05 | 03 | PIID | OAD | RSD | RCSD-null | time tag."""
    apdu = bytes([0x05, 0x03, piid]) + oad.to_bytes(4, "big") + rsd + b"\x00" + b"\x00"
    return _frame(apdu)


def _apdus(run) -> list[bytes]:
    return [protocol.parse_frame(frame).apdu for frame in protocol.iter_698_frames(run.uart_tx)]


def _get_bytes(apdu: bytes) -> bytes:
    """Return the data bytes of a GET-Response-Normal without decoding them.

    The frozen capture table contains 0x51 (OAD) tags that protocol.py's
    decoder does not know.
    """
    assert apdu[0] == 0x85 and apdu[1] == 0x01, apdu.hex()
    assert apdu[7] == 0x01, apdu.hex()
    return apdu[8:-2]


def _record(apdu: bytes) -> tuple[int, int, bytes]:
    """Return (record count, record bytes) from a GET-Response-Record.

    Layout after the OAD: RCSD echo (count byte plus 5 bytes per entry), the
    success flag, then the record count and records.
    """
    assert apdu[0] == 0x85 and apdu[1] == 0x03, apdu.hex()
    oad = int.from_bytes(apdu[3:7], "big")
    position = 8 + apdu[7] * 5
    if apdu[position] != 0x01:
        raise AssertionError(f"record request for {oad:#010x} failed: {apdu.hex()}")
    return apdu[position + 1], apdu[position + 2 :]


def _one_record(apdu: bytes) -> bytes:
    count, body = _record(apdu)
    assert count == 1, (count, apdu.hex())
    return body


LAST_N = lambda n: b"\x09" + bytes([n])
EVENT_NUMBER = lambda n: b"\x01\x20\x22\x02\x00\x06" + n.to_bytes(4, "big")
FREEZE_TIME = lambda t: b"\x01\x20\x21\x02\x00\x1c" + t
FREEZE_RANGE = lambda a, b: b"\x02\x20\x21\x02\x00\x1c" + a + b"\x1c" + b + b"\x00"


@case(
    "ACC-698-006",
    "a SET split across two link segments is reassembled and applied, and a "
    "GET normal-list response longer than 512 bytes arrives in three link "
    "segments that reassemble to 120 items",
    ("dl698", "segment"),
)
def segmentation(ctx):
    set_apdu = protocol.set_request(1, 0x20150400, bytes.fromhex("042000000005"))
    first = _frame(b"\x00\x00" + set_apdu[:6], ctrl=0x63)
    last = _frame(b"\x01\x40" + set_apdu[6:], ctrl=0x63)
    rx_run = ctx.run(
        cycles=9,
        scenario=(
            f"at 2 uart 0 {first.hex()}\n"
            f"at 3 uart 0 {last.hex()}\n"
            f"at 5 uart 0 {_get(2, 0x20150400).hex()}\n"
        ),
    )
    assert rx_run.exit_code == 0, f"firmware_sim exited {rx_run.exit_code}"
    apdus = _apdus(rx_run)
    assert len(apdus) == 3, [apdu.hex() for apdu in apdus]
    assert apdus[0] == bytes.fromhex("0080"), apdus[0].hex()
    piid, dar = protocol.parse_set_response(apdus[1])
    assert (piid, dar) == (1, 0), (piid, dar)
    value = protocol.parse_get_response(apdus[2])
    assert value.data == bytes.fromhex("042000000005"), value.data.hex()

    count = 120
    payload = bytes([0x05, 0x02, 1, count]) + (0x20150400).to_bytes(4, "big") * count
    request = _frame(payload + b"\x00")
    confirm_one = _frame(b"\x00\x80", ctrl=0x63)
    confirm_two = _frame(b"\x01\x80", ctrl=0x63)
    tx_run = ctx.run(
        cycles=8,
        scenario=(
            f"at 2 uart 0 {request.hex()}\n"
            f"at 3 uart 0 {confirm_one.hex()}\n"
            f"at 4 uart 0 {confirm_two.hex()}\n"
        ),
    )
    assert tx_run.exit_code == 0, f"firmware_sim exited {tx_run.exit_code}"
    frames = [protocol.parse_frame(frame) for frame in protocol.iter_698_frames(tx_run.uart_tx)]
    assert len(frames) == 3, [frame.apdu.hex() for frame in frames]
    assert [frame.ctrl for frame in frames] == [0xE3, 0xE3, 0xE3]
    assert frames[0].apdu[:2] == b"\x00\x00", frames[0].apdu[:2].hex()
    assert frames[1].apdu[:2] == b"\x01\xc0", frames[1].apdu[:2].hex()
    assert frames[2].apdu[:2] == b"\x02\x40", frames[2].apdu[:2].hex()

    body = b"".join(frame.apdu[2:] for frame in frames)
    assert body[:4] == bytes([0x85, 0x02, 1, count]), body[:4].hex()
    offset = 4
    for _ in range(count):
        assert int.from_bytes(body[offset : offset + 4], "big") == 0x20150400
        assert body[offset + 4] == 0x01, body[offset + 4]
        value, used = protocol.axdr_decode(body[offset + 5 :])
        assert value.tag == protocol.AXDR_BIT_STRING and value.value == b"\xff" * 4
        offset += 5 + used
    assert offset == len(body) - 2, (offset, len(body))


@case(
    "ACC-698-011",
    "SET writes through variable, event, parameter, frozen, control, and "
    "other object classes all report success and read back through GET",
    ("dl698", "set"),
)
def set_object_families(ctx):
    writes = [
        (0x20150400, bytes.fromhex("042000000005"), "042000000005"),
        (0x301B0800, bytes.fromhex("1601"), "1601"),
        (0x40120200, bytes.fromhex("04085a"), "04085a"),
        (0xF3000401, bytes.fromhex("1103"), "1103"),
        (0x50060300, bytes.fromhex("010102031200015100000400120001"), "010102031200015100000400120001"),
        (0x80000200, bytes.fromhex("0202060000123412000a"), "0202060000123412000a"),
    ]
    scenario = []
    for index, (oad, data, _) in enumerate(writes):
        scenario.append(f"at {2 * index + 1} uart 0 {_set(index + 1, oad, data).hex()}\n")
        scenario.append(f"at {2 * index + 2} uart 0 {_get(index + 1, oad).hex()}\n")
    run = ctx.run(cycles=14, scenario="".join(scenario))
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    apdus = _apdus(run)
    assert len(apdus) == 2 * len(writes), [apdu.hex() for apdu in apdus]

    for index, (oad, _, expected) in enumerate(writes):
        piid, dar = protocol.parse_set_response(apdus[2 * index])
        assert (piid, dar) == (index + 1, 0), (hex(oad), piid, dar)
        result = apdus[2 * index + 1]
        assert int.from_bytes(result[3:7], "big") == oad, (hex(oad), result.hex())
        assert _get_bytes(result).hex() == expected, (hex(oad), result.hex())


@case(
    "ACC-698-012",
    "ACTION methods on variable, event, parameter, frozen, and control "
    "objects return success and their external effects are visible",
    ("dl698", "action"),
)
def action_object_families(ctx):
    alarm_on = _action(2, 0x8000, 0x7F, b"\x00")
    alarm_off = _action(4, 0x8000, 0x80, b"\x00")
    energy_clear = _action(9, 0x4300, 3, b"\x00")
    run = ctx.run(
        cycles=12,
        scenario=(
            f"at 2 uart 0 {_action(1, 0x2015, 127, bytes.fromhex('042000000001')).hex()}\n"
            f"at 3 uart 0 {alarm_on.hex()}\n"
            f"at 4 uart 0 {_get(3, 0x80000400).hex()}\n"
            f"at 5 uart 0 {alarm_off.hex()}\n"
            f"at 6 uart 0 {_get(5, 0x80000400).hex()}\n"
            f"at 7 uart 0 {_action(6, 0x5000, 3, bytes.fromhex('120000')).hex()}\n"
            f"at 9 uart 0 {_record_request(7, 0x50000200, LAST_N(1)).hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    apdus = _apdus(run)
    assert len(apdus) == 7, [apdu.hex() for apdu in apdus]

    for index in (0, 1, 3, 5):
        result = protocol.parse_action_response(apdus[index])
        assert result.dar == 0, (index, result.dar, result.error)
    assert protocol.parse_get_response(apdus[2]).data == bytes.fromhex("040880")
    assert protocol.parse_get_response(apdus[4]).data == bytes.fromhex("040800")

    freeze = _one_record(apdus[6])
    assert freeze[0] == 0x06 and freeze[5] == 0x1C, freeze.hex()

    energy_run = ctx.run(
        cycles=8,
        scenario=(
            "at 0 energy clear\n"
            "at 1 meter const 3200 cal 128\n"
            "at 1 pulse 32\n"
            f"at 3 uart 0 {_get(1, 0x00000401).hex()}\n"
            f"at 4 uart 0 {energy_clear.hex()}\n"
            f"at 5 uart 0 {_get(3, 0x00000401).hex()}\n"
        ),
    )
    assert energy_run.exit_code == 0, f"firmware_sim exited {energy_run.exit_code}"
    energy_apdus = _apdus(energy_run)
    assert protocol.parse_get_response(energy_apdus[0]).values()[0].value == 100
    result = protocol.parse_action_response(energy_apdus[1])
    assert result.dar == 0, (result.dar, result.error)
    assert protocol.parse_get_response(energy_apdus[2]).values()[0].value == 0

    event_reset = _action(2, 0x301B, 1, b"\x0f\x00")
    event_run = ctx.run(
        cycles=12,
        scenario=(
            "at 2 cover open\n"
            "at 6 cover closed\n"
            f"at 7 uart 0 {_record_request(1, 0x301B0200, LAST_N(1)).hex()}\n"
            f"at 8 uart 0 {event_reset.hex()}\n"
            f"at 9 uart 0 {_record_request(3, 0x301B0200, LAST_N(1)).hex()}\n"
        ),
    )
    assert event_run.exit_code == 0, f"firmware_sim exited {event_run.exit_code}"
    event_apdus = _apdus(event_run)
    assert _record(event_apdus[0])[0] == 1, event_apdus[0].hex()
    result = protocol.parse_action_response(event_apdus[1])
    assert result.dar == 0, (result.dar, result.error)
    assert _record(event_apdus[2])[0] == 0, event_apdus[2].hex()


@case(
    "ACC-698-013",
    "record selection by last-N, by event number, by time, and by time "
    "range returns the stored event and month-freeze records or an empty list",
    ("dl698", "record"),
)
def record_selection(ctx):
    event_run = ctx.run(
        cycles=18,
        scenario=(
            "at 2 cover open\n"
            "at 8 cover closed\n"
            f"at 12 uart 0 {_record_request(1, 0x301B0200, LAST_N(1)).hex()}\n"
            f"at 13 uart 0 {_record_request(2, 0x301B0200, LAST_N(2)).hex()}\n"
            f"at 14 uart 0 {_record_request(3, 0x301B0200, EVENT_NUMBER(0xFFFFFFFF)).hex()}\n"
            f"at 15 uart 0 {_record_request(4, 0x301B0200, EVENT_NUMBER(0)).hex()}\n"
        ),
    )
    assert event_run.exit_code == 0, f"firmware_sim exited {event_run.exit_code}"
    apdus = _apdus(event_run)
    assert len(apdus) == 4, [apdu.hex() for apdu in apdus]
    body = _one_record(apdus[0])
    assert body.startswith(bytes.fromhex("06ffffffff1c")), body.hex()
    assert body.count(b"\x1c") == 2, body.hex()
    assert _record(apdus[1])[0] == 0
    assert _one_record(apdus[2]) == body
    assert _record(apdus[3])[0] == 0

    month_run = ctx.run(
        cycles=3600,
        scenario=(
            f"at 3550 uart 0 {_record_request(1, 0x50060200, LAST_N(1)).hex()}\n"
            f"at 3551 uart 0 {_record_request(2, 0x50060200, LAST_N(2)).hex()}\n"
            f"at 3552 uart 0 {_record_request(3, 0x50060200, FREEZE_TIME(bytes.fromhex('07e50501000000'))).hex()}\n"
            f"at 3553 uart 0 {_record_request(4, 0x50060200, FREEZE_RANGE(bytes.fromhex('07e50501000000'), bytes.fromhex('07e50501000001'))).hex()}\n"
        ),
    )
    assert month_run.exit_code == 0, f"firmware_sim exited {month_run.exit_code}"
    month_apdus = _apdus(month_run)
    assert len(month_apdus) == 4, [apdu.hex() for apdu in month_apdus]
    month = _one_record(month_apdus[0])
    assert month.startswith(bytes.fromhex("06ffffffff1c07e50501000000")), month.hex()
    assert _record(month_apdus[1])[0] == 0
    assert _one_record(month_apdus[2]) == month
    assert _one_record(month_apdus[3]) == month


@case(
    "ACC-698-008",
    "event records and month-freeze records are readable through 698 "
    "record OADs with their own timestamps and sequence fields",
    ("dl698", "record"),
)
def event_and_freeze_reads(ctx):
    event_run = ctx.run(
        cycles=18,
        scenario=(
            "at 2 cover open\n"
            "at 8 cover closed\n"
            f"at 12 uart 0 {_record_request(1, 0x301B0200, LAST_N(1)).hex()}\n"
        ),
    )
    assert event_run.exit_code == 0, f"firmware_sim exited {event_run.exit_code}"
    event = _one_record(_apdus(event_run)[0])
    assert event.startswith(bytes.fromhex("06ffffffff1c")), event.hex()
    assert event.count(b"\x1c") == 2, event.hex()

    month_run = ctx.run(
        cycles=3600,
        scenario=(
            f"at 3550 uart 0 {_record_request(1, 0x50060200, LAST_N(1)).hex()}\n"
        ),
    )
    assert month_run.exit_code == 0, f"firmware_sim exited {month_run.exit_code}"
    month = _one_record(_apdus(month_run)[0])
    assert month.startswith(bytes.fromhex("06ffffffff1c07e50501000000")), month.hex()
