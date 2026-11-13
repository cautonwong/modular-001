from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6
COVER_VALUE = 0x301B0A00


def _cover_value(ctx, scenario: str, cycles: int = 15) -> bytes:
    request = protocol.build_frame(
        protocol.get_request(1, COVER_VALUE), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=cycles,
        scenario=scenario + f"at 12 uart 0 {request.hex()}\n",
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert frames, f"no cover event response: {run.uart_tx.hex()}"
    result = protocol.parse_get_response(protocol.parse_frame(frames[0]).apdu)
    assert result.oad == COVER_VALUE
    return result.data


def _event_after_power_cycle(ctx, count_oad: int, value_oad: int) -> tuple[int, bytes]:
    count = protocol.build_frame(
        protocol.get_request(1, count_oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    value = protocol.build_frame(
        protocol.get_request(2, value_oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=10,
        scenario=(
            f"at 1 uart 0 {count.hex()}\n"
            "at 2 power off\n"
            "at 5 power on\n"
            f"at 8 uart 0 {count.hex()}\n"
            f"at 9 uart 0 {value.hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert len(frames) >= 3, run.uart_tx.hex()
    before = protocol.parse_get_response(protocol.parse_frame(frames[0]).apdu).values()[0]
    after = protocol.parse_get_response(protocol.parse_frame(frames[1]).apdu).values()[0]
    current = protocol.parse_get_response(protocol.parse_frame(frames[2]).apdu).data
    return before.value, (after.value, current)


@case(
    "ACC-EVT-004",
    "a power cut and restore adds one power event with a timestamp",
    ("events", "power"),
)
def power_event(ctx):
    before, (after, current) = _event_after_power_cycle(ctx, 0x30110400, 0x30110A00)

    assert before == 0, before
    assert after == 1, after
    assert b"\x1c" in current, current.hex()


def clock_fault_event(ctx):
    before, (after, current) = _event_after_power_cycle(ctx, 0x302E0400, 0x302E0A00)

    assert before == 0, before
    assert after == 0, after
    assert b"\x1c" not in current, current.hex()


@case(
    "ACC-EVT-005",
    "a 698 parameter write creates one programming event with a timestamp",
    ("events", "parameter"),
)
def programming_event(ctx):
    set_frame = protocol.build_frame(
        protocol.set_request(1, 0x40130200, b"\x11\x01"), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    count_frame = protocol.build_frame(
        protocol.get_request(2, 0x30190400), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    value_frame = protocol.build_frame(
        protocol.get_request(3, 0x30190A00), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=8,
        scenario=(
            f"at 1 uart 0 {set_frame.hex()}\n"
            f"at 3 uart 0 {count_frame.hex()}\n"
            f"at 5 uart 0 {value_frame.hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert len(frames) == 3, run.uart_tx.hex()
    piid, dar = protocol.parse_set_response(protocol.parse_frame(frames[0]).apdu)
    assert (piid, dar) == (1, 0), (piid, dar)
    count = protocol.parse_get_response(protocol.parse_frame(frames[1]).apdu).values()[0]
    value = protocol.parse_get_response(protocol.parse_frame(frames[2]).apdu).data
    assert count.value == 1, count.value
    assert b"\x1c" in value, value.hex()


@case(
    "ACC-EVT-001",
    "a cover opening longer than the 3 s debounce creates an event record",
    ("events", "cover"),
)
def cover_open_event(ctx):
    before = _cover_value(ctx, "")
    after = _cover_value(ctx, "at 2 cover open\nat 8 cover closed\n")

    assert b"\x1c" not in before, before.hex()
    assert b"\x1c" in after, after.hex()
    assert after != before


@case(
    "ACC-EVT-008",
    "a cover pulse shorter than the debounce time does not create an event",
    ("events", "debounce"),
)
def cover_debounce(ctx):
    data = _cover_value(ctx, "at 2 cover open\nat 3 cover closed\n")

    assert data == bytes.fromhex("010102020002020000"), data.hex()
