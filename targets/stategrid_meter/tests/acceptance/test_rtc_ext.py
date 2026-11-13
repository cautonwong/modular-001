from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6


def _read_datetime(ctx, start_time: str, at_cycle: int) -> str:
    request = protocol.build_frame(
        protocol.get_request(1, 0x40000200), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=at_cycle + 2,
        start_time=start_time,
        scenario=f"at {at_cycle} uart 0 {request.hex()}\n",
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert frames, f"no clock response: {run.uart_tx.hex()}"
    data = protocol.parse_get_response(protocol.parse_frame(frames[0]).apdu).data
    assert data[0] == protocol.AXDR_DATE_TIME_S, data.hex()
    year = int.from_bytes(data[1:3], "big")
    month, day, hour, minute, second = data[3:8]
    return f"{year:04d}-{month:02d}-{day:02d} {hour:02d}:{minute:02d}:{second:02d}"


@case(
    "ACC-RTC-002",
    "the calendar crosses month ends and leap day and is readable over 698",
    ("rtc", "calendar"),
)
def calendar_boundaries(ctx):
    assert _read_datetime(ctx, "2024-02-28 23:59:58", 3) == "2024-02-29 00:00:01"
    assert _read_datetime(ctx, "2023-12-31 23:59:58", 3) == "2024-01-01 00:00:01"
