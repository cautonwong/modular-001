from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6

NEUTRAL_COUNT = 0x30400400
NEUTRAL_THRESHOLD = 0x30400601
NEUTRAL_RATE = 0x30400602
NEUTRAL_DURATION = 0x30400603
NEUTRAL_VALUE = 0x30400700
NEUTRAL_TIME_STATUS = 0x30400A00

OC_COUNT = 0x30050300
OC_THRESHOLD = 0x30050501
OC_DURATION = 0x30050502


def _frame(request: bytes) -> bytes:
    return protocol.build_frame(request, sa=BLANK_SA, ca=0x10, ctrl=0x43)


def _get(piid: int, oad: int) -> str:
    return _frame(protocol.get_request(piid, oad)).hex()


def _set(piid: int, oad: int, data: bytes) -> str:
    return _frame(protocol.set_request(piid, oad, data)).hex()


def _responses(run) -> list[bytes]:
    return [
        protocol.parse_frame(frame).apdu
        for frame in protocol.iter_698_frames(run.uart_tx)
    ]


def _check_sets(responses: list[bytes], count: int) -> None:
    for response in responses[:count]:
        piid, dar = protocol.parse_set_response(response)
        assert dar == 0, f"SET piid {piid} rejected with DAR {dar}"


def _ic_count(response: bytes) -> tuple[int, int]:
    result = protocol.parse_get_response(response)
    data = result.data
    assert data[:3] == b"\x02\x04\x00" and data[3] == 0x12, data.hex()
    return result.oad, int.from_bytes(data[4:6], "big")


def _record_count(response: bytes) -> tuple[int, int]:
    result = protocol.parse_get_response(response)
    data = result.data
    assert data[0] == 0x12, data.hex()
    return result.oad, int.from_bytes(data[1:3], "big")


@case(
    "ACC-EVT-003",
    "neutral-line current abnormal starts after its debounce and freezes its accumulated time",
    ("events", "neutral-current"),
)
def neutral_current_event(ctx):
    threshold = b"\x05" + (10000).to_bytes(4, "big")
    rate = b"\x12\x13\x88"
    duration = b"\x11\x03"
    scenario = (
        f"at 1 uart 0 {_set(1, NEUTRAL_THRESHOLD, threshold)}\n"
        f"at 2 uart 0 {_set(2, NEUTRAL_RATE, rate)}\n"
        f"at 3 uart 0 {_set(3, NEUTRAL_DURATION, duration)}\n"
        f"at 4 uart 0 {_get(10, NEUTRAL_COUNT)}\n"
        f"at 8 uart 0 {_get(11, NEUTRAL_COUNT)}\n"
        f"at 9 uart 0 {_get(12, NEUTRAL_COUNT)}\n"
        f"at 10 uart 0 {_get(13, NEUTRAL_VALUE)}\n"
        f"at 12 uart 0 {_get(14, NEUTRAL_VALUE)}\n"
        f"at 15 uart 0 {_get(15, NEUTRAL_VALUE)}\n"
        f"at 16 uart 0 {_get(16, NEUTRAL_VALUE)}\n"
        f"at 17 uart 0 {_get(17, NEUTRAL_TIME_STATUS)}\n"
    )
    for cycle in range(5, 11):
        scenario += f"at {cycle} meter current 2\n"
    for cycle in range(11, 17):
        scenario += f"at {cycle} meter current 0\n"

    run = ctx.run(
        cycles=20, scenario=scenario, start_time="2026-11-12 00:00:00"
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = _responses(run)
    assert len(responses) == 11, run.uart_tx.hex()
    _check_sets(responses, 3)

    counts = [_record_count(response) for response in responses[3:6]]
    assert [oad for oad, _ in counts] == [NEUTRAL_COUNT] * 3
    assert [count for _, count in counts] == [0, 0, 1]

    values = []
    for response in responses[6:10]:
        result = protocol.parse_get_response(response)
        assert result.oad == NEUTRAL_VALUE
        data = result.data
        assert data[:8] == b"\x01\x01\x02\x02\x00\x02\x02\x06", data.hex()
        assert data[12] == 0x06, data.hex()
        values.append(int.from_bytes(data[13:17], "big"))
    assert values == [2, 4, 6, 6], values

    result = protocol.parse_get_response(responses[10])
    assert result.oad == NEUTRAL_TIME_STATUS
    assert result.data == (
        b"\x01\x01\x02\x02\x00\x02\x02"
        b"\x1c\x07\xea\x0b\x0c\x00\x00\x09"
        b"\x1c\x07\xea\x0b\x0c\x00\x00\x0f"
    ), result.data.hex()


@case(
    "ACC-EVT-011",
    "over-current and neutral-current events are isolated by their own object catalog",
    ("events", "catalog"),
)
def event_catalog_isolation(ctx):
    duration = b"\x11\x03"
    over_current = b"\x05" + (10000).to_bytes(4, "big")
    neutral = b"\x05" + (10000).to_bytes(4, "big")
    rate = b"\x12\x13\x88"
    zero = b"\x12\x00\x00"
    no_threshold = b"\x05" + (0).to_bytes(4, "big")

    run_a = (
        f"at 1 uart 0 {_set(1, OC_THRESHOLD, over_current)}\n"
        f"at 2 uart 0 {_set(2, OC_DURATION, duration)}\n"
        f"at 3 uart 0 {_set(3, NEUTRAL_RATE, zero)}\n"
        f"at 4 uart 0 {_set(4, NEUTRAL_DURATION, duration)}\n"
    )
    for cycle in range(5, 11):
        run_a += f"at {cycle} meter current 2\n"
    run_a += (
        f"at 12 uart 0 {_get(20, OC_COUNT)}\n"
        f"at 13 uart 0 {_get(21, NEUTRAL_COUNT)}\n"
    )
    first = ctx.run(cycles=15, scenario=run_a)
    assert first.exit_code == 0, f"firmware_sim exited {first.exit_code}"

    responses = _responses(first)
    assert len(responses) == 6, first.uart_tx.hex()
    _check_sets(responses, 4)
    assert _ic_count(responses[4]) == (OC_COUNT, 1)
    assert _record_count(responses[5]) == (NEUTRAL_COUNT, 0)

    run_b = (
        f"at 1 uart 0 {_set(1, OC_THRESHOLD, no_threshold)}\n"
        f"at 2 uart 0 {_set(2, OC_DURATION, duration)}\n"
        f"at 3 uart 0 {_set(3, NEUTRAL_THRESHOLD, neutral)}\n"
        f"at 4 uart 0 {_set(4, NEUTRAL_RATE, rate)}\n"
        f"at 5 uart 0 {_set(5, NEUTRAL_DURATION, duration)}\n"
    )
    for cycle in range(5, 11):
        run_b += f"at {cycle} meter current 2\n"
    run_b += (
        f"at 12 uart 0 {_get(20, OC_COUNT)}\n"
        f"at 13 uart 0 {_get(21, NEUTRAL_COUNT)}\n"
    )
    second = ctx.run(cycles=15, scenario=run_b)
    assert second.exit_code == 0, f"firmware_sim exited {second.exit_code}"

    responses = _responses(second)
    assert len(responses) == 7, second.uart_tx.hex()
    _check_sets(responses, 5)
    assert _ic_count(responses[5]) == (OC_COUNT, 0)
    assert _record_count(responses[6]) == (NEUTRAL_COUNT, 1)


@case(
    "ACC-EVT-015",
    "over-current starts only above its threshold and a zero threshold disables it",
    ("events", "over-current"),
)
def over_current_threshold_boundary(ctx):
    threshold = b"\x05" + (40000).to_bytes(4, "big")
    disabled = b"\x05" + (0).to_bytes(4, "big")
    duration = b"\x11\x01"
    scenario = (
        f"at 1 uart 0 {_set(1, OC_THRESHOLD, threshold)}\n"
        f"at 2 uart 0 {_set(2, OC_DURATION, duration)}\n"
        f"at 12 uart 0 {_get(3, OC_COUNT)}\n"
    )
    for cycle in range(5, 11):
        scenario += f"at {cycle} meter current 4\n"
    for cycle in range(12, 18):
        scenario += f"at {cycle} meter current 5\n"
    scenario += f"at 19 uart 0 {_get(4, OC_COUNT)}\n"
    scenario += f"at 20 uart 0 {_set(5, OC_THRESHOLD, disabled)}\n"
    for cycle in range(21, 27):
        scenario += f"at {cycle} meter current 5\n"
    scenario += f"at 28 uart 0 {_get(6, OC_COUNT)}\n"

    run = ctx.run(cycles=30, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = _responses(run)
    assert len(responses) == 6, run.uart_tx.hex()
    _check_sets(responses, 2)
    assert _ic_count(responses[2]) == (OC_COUNT, 0)
    assert _ic_count(responses[3]) == (OC_COUNT, 1)
    piid, dar = protocol.parse_set_response(responses[4])
    assert dar == 0, f"SET piid {piid} rejected with DAR {dar}"
    assert _ic_count(responses[5]) == (OC_COUNT, 1)
