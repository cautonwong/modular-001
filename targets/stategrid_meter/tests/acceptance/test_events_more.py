from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6

OC_COUNT = 0x30050300
OC_VALUE = 0x30050A00
OC_THRESHOLD = 0x30050501
OC_DURATION = 0x30050502


def _frame(request: bytes) -> bytes:
    return protocol.build_frame(request, sa=BLANK_SA, ca=0x10, ctrl=0x43)


def _get(piid: int, oad: int) -> str:
    return _frame(protocol.get_request(piid, oad)).hex()


def _set(piid: int, oad: int, data: bytes) -> str:
    return _frame(protocol.set_request(piid, oad, data)).hex()


@case(
    "ACC-EVT-002",
    "over-current event is produced after its debounce, lasts the disturbance, and recovers",
    ("events", "over-current"),
)
def over_current_event(ctx):
    threshold = 10000  # 2.0000 A after the /10 adapter scaling
    threshold_data = b"\x05" + threshold.to_bytes(4, "big")
    duration_data = b"\x11\x03"
    scenario = (
        f"at 1 uart 0 {_set(1, OC_THRESHOLD, threshold_data)}\n"
        f"at 2 uart 0 {_set(2, OC_DURATION, duration_data)}\n"
    )
    # debounce 3 needs 4 consecutive true seconds (cycles 5..8) to start.
    for cycle in range(5, 11):
        scenario += f"at {cycle} meter current 2\n"
    # 4 consecutive false seconds (cycles 11..14) end the event.
    for cycle in range(11, 17):
        scenario += f"at {cycle} meter current 0\n"
    scenario += (
        f"at 4 uart 0 {_get(10, OC_COUNT)}\n"
        f"at 8 uart 0 {_get(11, OC_COUNT)}\n"
        f"at 9 uart 0 {_get(12, OC_COUNT)}\n"
        f"at 12 uart 0 {_get(13, OC_VALUE)}\n"
        f"at 15 uart 0 {_get(14, OC_VALUE)}\n"
        f"at 16 uart 0 {_get(15, OC_VALUE)}\n"
    )
    run = ctx.run(cycles=18, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert len(frames) == 8, run.uart_tx.hex()

    responses = [protocol.parse_frame(frame).apdu for frame in frames]
    for response in responses[:2]:
        piid, dar = protocol.parse_set_response(response)
        assert dar == 0, f"SET piid {piid} rejected with DAR {dar}"

    counts = []
    for response in responses[2:5]:
        result = protocol.parse_get_response(response)
        assert result.oad == OC_COUNT
        # structure(4): null, long-unsigned <count>, null, null
        data = result.data
        assert data[:3] == b"\x02\x04\x00" and data[3] == 0x12, data.hex()
        counts.append(int.from_bytes(data[4:6], "big"))
    assert counts == [0, 0, 1], counts

    values = []
    for response in responses[5:]:
        result = protocol.parse_get_response(response)
        assert result.oad == OC_VALUE
        # array(4): null, structure{count, accumulated seconds}, null, null
        data = result.data
        assert data[:5] == b"\x01\x04\x00\x02\x02" and data[10] == 0x06, data.hex()
        values.append(int.from_bytes(data[11:15], "big"))
    assert values == [4, 6, 6], values
