from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6

VOLTAGE = 0x20000201
CURRENT = 0x20010201

AUTO_LIST = 0xF3000200
AUTO_INTERVAL = 0xF3000300
AUTO_COUNT = 0xF3000401
KEY_LIST = 0xF3010200
KEY_COUNT = 0xF3010401
BACKLIGHT_KEY_TIME = 0x40070202


def _frame(request: bytes) -> bytes:
    return protocol.build_frame(request, sa=BLANK_SA, ca=0x10, ctrl=0x43)


def _set(piid: int, oad: int, data: bytes) -> str:
    return _frame(protocol.set_request(piid, oad, data)).hex()


def _display_entry(oad: int) -> bytes:
    return b"\x02\x02\x5b\x00" + oad.to_bytes(4, "big") + b"\x11\x00"


def _display_list(*oads: int) -> bytes:
    return (
        b"\x01"
        + bytes([len(oads)])
        + b"".join(_display_entry(oad) for oad in oads)
    )


def _lcd_frames(run) -> dict[int, str]:
    frames = {}
    for line in run.lcd_log:
        fields = line.split()
        assert fields[0] == "cycle" and fields[2] == "time", line
        assert fields[5] == "fb" and len(fields[6]) == 320, line
        frames[int(fields[1])] = fields[6]
    return frames


def _assert_pages(frames: dict[int, str], start: int, end: int, expected: str) -> None:
    for cycle in range(start, end + 1):
        assert frames[cycle] == expected, (cycle, frames[cycle])


def _display_scenario(*oads: int) -> str:
    interval = b"\x12\x00\x05"
    count = bytes([0x11, len(oads)])
    return (
        "at 0 meter voltage 220\n"
        "at 0 meter current 5\n"
        f"at 1 uart 0 {_set(1, AUTO_LIST, _display_list(*oads))}\n"
        f"at 2 uart 0 {_set(2, AUTO_INTERVAL, interval)}\n"
        f"at 3 uart 0 {_set(3, AUTO_COUNT, count)}\n"
    )


@case(
    "ACC-DSP-001",
    "cyclic display follows the configured list and advances every configured interval",
    ("display", "auto-page"),
)
def cyclic_display(ctx):
    run = ctx.run(cycles=26, scenario=_display_scenario(VOLTAGE, CURRENT))
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    frames = _lcd_frames(run)
    assert sorted(frames) == list(range(26)), sorted(frames)

    voltage = frames[6]
    current = frames[10]
    assert voltage != current, "voltage and current pages render the same"

    _assert_pages(frames, 6, 9, voltage)
    _assert_pages(frames, 10, 14, current)
    _assert_pages(frames, 15, 19, voltage)
    _assert_pages(frames, 20, 24, current)
    assert frames[25] == voltage

    reversed_run = ctx.run(cycles=26, scenario=_display_scenario(CURRENT, VOLTAGE))
    assert reversed_run.exit_code == 0, f"firmware_sim exited {reversed_run.exit_code}"

    reversed_frames = _lcd_frames(reversed_run)
    _assert_pages(reversed_frames, 6, 9, current)
    _assert_pages(reversed_frames, 10, 14, voltage)


@case(
    "ACC-DSP-002",
    "each key press advances the key display list by one page",
    ("display", "key-page"),
)
def key_display(ctx):
    interval = b"\x12\x00\x05"
    count = b"\x11\x02"
    scenario = (
        "at 0 meter voltage 220\n"
        "at 0 meter current 5\n"
        f"at 1 uart 0 {_set(1, AUTO_LIST, _display_list(VOLTAGE, CURRENT))}\n"
        f"at 2 uart 0 {_set(2, AUTO_INTERVAL, interval)}\n"
        f"at 3 uart 0 {_set(3, AUTO_COUNT, count)}\n"
        f"at 4 uart 0 {_set(4, KEY_LIST, _display_list(CURRENT, VOLTAGE))}\n"
        f"at 5 uart 0 {_set(5, KEY_COUNT, count)}\n"
        "at 8 key press\n"
        "at 9 key release\n"
        "at 12 key press\n"
        "at 13 key release\n"
    )
    run = ctx.run(cycles=20, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    frames = _lcd_frames(run)
    assert sorted(frames) == list(range(20)), sorted(frames)

    voltage = frames[6]
    current = frames[9]
    assert voltage != current, "voltage and current pages render the same"

    _assert_pages(frames, 6, 8, voltage)
    _assert_pages(frames, 9, 13, current)
    _assert_pages(frames, 14, 19, voltage)


@case(
    "ACC-DSP-003",
    "a key press lights the backlight and it turns off after backlight_key_time seconds",
    ("display", "backlight"),
)
def key_backlight(ctx):
    backlight_time = b"\x12\x00\x05"
    scenario = (
        f"at 1 uart 0 {_set(1, BACKLIGHT_KEY_TIME, backlight_time)}\n"
        "at 8 key press\n"
        "at 9 key release\n"
    )
    run = ctx.run(cycles=20, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    values = []
    for line in run.backlight_log:
        fields = line.split()
        assert fields[0] == "cycle" and fields[2] == "time", line
        assert fields[5] == "value", line
        values.append((int(fields[1]), int(fields[6])))
    assert [cycle for cycle, _ in values] == list(range(20)), values

    on = [cycle for cycle, value in values if value == 1]
    assert on == [9, 10, 11, 12, 13], on
