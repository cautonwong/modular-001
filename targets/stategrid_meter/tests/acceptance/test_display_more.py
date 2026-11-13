"""Display acceptance rows: ACC-DSP-004 (configured display map) and
ACC-LC-007 (relay state shown on the LCD).

The display-list GET response carries A-XDR tag 0x5B, which the scalar decoder
does not support, so the list is compared as raw response bytes. Everything
else is read from UART or lcd.log.
"""

from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6
ZERO = b"\x00"

STATUS = 0x80000500
VOLTAGE = 0x20000201
CURRENT = 0x20010201
AUTO_LIST = 0xF3000200
AUTO_INTERVAL = 0xF3000300
AUTO_COUNT = 0xF3000401


def _frame(apdu: bytes, sa: bytes = BLANK_SA) -> bytes:
    return protocol.build_frame(apdu, sa=sa, ca=0x10, ctrl=0x43)


def _get(piid: int, oad: int, sa: bytes = BLANK_SA) -> str:
    return _frame(protocol.get_request(piid, oad), sa).hex()


def _set(piid: int, oad: int, data: bytes) -> str:
    return _frame(protocol.set_request(piid, oad, data)).hex()


def _action(piid: int, oi: int, method: int, param: bytes | None = None) -> str:
    return _frame(protocol.action_request(piid, oi, method, param=param)).hex()


def _display_entry(oad: int) -> bytes:
    return b"\x02\x02\x5b\x00" + oad.to_bytes(4, "big") + b"\x11\x00"


def _display_list(*oads: int) -> bytes:
    return b"\x01" + bytes([len(oads)]) + b"".join(_display_entry(oad) for oad in oads)


def _apdus(run):
    return [protocol.parse_frame(frame).apdu for frame in protocol.iter_698_frames(run.uart_tx)]


def _get_bytes(apdu: bytes) -> bytes:
    assert apdu[0] == 0x85 and apdu[1] == 0x01, apdu.hex()
    assert apdu[7] == 0x01, apdu.hex()
    return apdu[8:-2]


def _frames(run) -> dict[int, str]:
    frames = {}
    for line in run.lcd_log:
        fields = line.split()
        assert fields[0] == "cycle" and fields[2] == "time", line
        assert fields[5] == "fb" and len(fields[6]) == 320, line
        frames[int(fields[1])] = fields[6]
    return frames


def _relay(run) -> list[tuple[int, str]]:
    entries = []
    for line in run.relay_log:
        fields = line.split()
        assert fields[0] == "cycle" and fields[2] == "time", line
        assert fields[5] == "action" and fields[6] in ("connect", "disconnect"), line
        entries.append((int(fields[1]), fields[6]))
    return entries


@case(
    "ACC-DSP-004",
    "a display list of voltage then current reads back byte-for-byte, the two "
    "OADs return 2200/5000 and 1800/5000, and lcd.log shows a voltage page that "
    "changes with the mapped value while the current page does not",
    ("display", "698", "lcd"),
)
def display_map(ctx):
    def capture(voltage: int) -> tuple[dict[int, str], dict[int, bytes]]:
        scenario = (
            f"at 0 meter voltage {voltage}\n"
            "at 0 meter current 5\n"
            f"at 1 uart 0 {_set(1, AUTO_LIST, _display_list(VOLTAGE, CURRENT))}\n"
            f"at 2 uart 0 {_set(2, AUTO_INTERVAL, bytes.fromhex('120005'))}\n"
            f"at 3 uart 0 {_set(3, AUTO_COUNT, bytes([0x11, 2]))}\n"
            f"at 4 uart 0 {_get(4, AUTO_LIST)}\n"
            f"at 5 uart 0 {_get(5, VOLTAGE)}\n"
            f"at 6 uart 0 {_get(6, CURRENT)}\n"
        )
        run = ctx.run(cycles=26, scenario=scenario)
        assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

        values: dict[int, bytes] = {}
        list_bytes = None
        for apdu in _apdus(run):
            if apdu[0] == 0x86:
                piid, dar = protocol.parse_set_response(apdu)
                assert dar == 0, (piid, dar)
            elif apdu[0] == 0x85:
                oad = int.from_bytes(apdu[3:7], "big")
                data = _get_bytes(apdu)
                if oad == AUTO_LIST:
                    list_bytes = data
                else:
                    values[oad] = data
            else:
                raise AssertionError(f"unexpected APDU {apdu.hex()}")

        assert list_bytes == _display_list(VOLTAGE, CURRENT), list_bytes.hex()
        assert values == {
            VOLTAGE: bytes.fromhex("120898" if voltage == 220 else "120708"),
            CURRENT: bytes.fromhex("0500001388"),
        }, {oad: data.hex() for oad, data in values.items()}
        return _frames(run), values

    high, _ = capture(220)
    low, _ = capture(180)

    assert high[8] != high[12], "voltage and current pages are identical"
    assert high[8] != low[8], "the voltage page did not change with 220 V -> 180 V"
    assert high[12] == low[12], "the current page changed with an unrelated OAD"


@case(
    "ACC-LC-007",
    "a remote trip sets the LCD trip symbol (FB byte 9 bit 0x20) and clears it "
    "after close, matching the 0x80000500 status bytes and relay.log",
    ("load-control", "display", "lcd"),
)
def relay_state_display(ctx):
    scenario = (
        "at 0 meter voltage 220\n"
        "at 0 meter current 5\n"
        f"at 2 uart 0 {_action(1, 0x8000, 0x81, bytes.fromhex('0205110005f205020111001200001100'))}\n"
        f"at 4 uart 0 {_get(2, STATUS)}\n"
        f"at 6 uart 0 {_action(3, 0x8000, 0x82, bytes.fromhex('0205110005f205020111011200001100'))}\n"
        f"at 9 uart 0 {_get(4, STATUS)}\n"
    )
    run = ctx.run(cycles=12, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = []
    for apdu in _apdus(run):
        if apdu[0] == 0x87:
            responses.append(("action", protocol.parse_action_response(apdu)))
        else:
            responses.append(("get", protocol.parse_get_response(apdu)))
    assert [kind for kind, _ in responses] == ["action", "get", "action", "get"]
    assert responses[0][1].dar == 0 and responses[2][1].dar == 0
    assert responses[1][1].data == bytes.fromhex("040880")
    assert responses[3][1].data == bytes.fromhex("040800")

    frames = {cycle: bytes.fromhex(fb) for cycle, fb in _frames(run).items()}
    assert frames[1][9] & 0x20 == 0, frames[1][9]
    assert frames[4][9] & 0x20 == 0x20, frames[4][9]
    assert frames[9][9] & 0x20 == 0, frames[9][9]

    relay = _relay(run)
    assert (2, "disconnect") in relay, relay
    assert any(action == "connect" for cycle, action in relay if cycle >= 6), relay
