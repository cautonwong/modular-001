from __future__ import annotations

from cases import case

# DL/T 698.45 GET_Request for OAD F1 01 02 00, addressed to a meter whose TSA
# is the blank-EEPROM value FF FF FF FF FF FF. HCS covers bytes 1..11, FCS
# covers bytes 1..21, both CRC-16/reflected with poly 0x8408.
GET_REQUEST = bytes.fromhex("6817004305ffffffffffff10c71305012ff101020000c4ee16")


@case(
    "ACC-SCRIPT-001",
    "a scenario-injected 698 request reaches the stack and is answered on RS485",
    ("script", "protocol"),
)
def scenario_uart_request_gets_response(ctx):
    run = ctx.run(
        cycles=3,
        scenario="at 1 uart 0 " + GET_REQUEST.hex() + "\n",
        verbose=True,
    )
    assert run.exit_code == 0, f"simulator exited {run.exit_code}"
    assert run.uart_tx, "the firmware emitted no RS485 bytes"

    frame = run.uart_tx.lstrip(b"\xfe")
    assert frame.startswith(b"\x68"), frame[:8].hex()
    assert frame.endswith(b"\x16"), frame[-4:].hex()
    assert len(frame) >= 12, f"response frame is only {len(frame)} bytes"
    assert 0x85 in frame, f"no GET_Response service byte in {frame.hex()}"
    assert bytes.fromhex("".join(run.uart_tx_hex)) == run.uart_tx, (
        "hex capture disagrees with the raw capture"
    )


@case(
    "ACC-SCRIPT-002",
    "--start-time sets the simulated RTC before the first cycle",
    ("script", "rtc"),
)
def start_time_sets_clock(ctx):
    run = ctx.run(cycles=2, start_time="2024-02-29 12:00:00")
    assert run.exit_code == 0, f"simulator exited {run.exit_code}"
    assert run.cycles, "simulator printed no cycle trace"
    assert run.cycles[0].time == "2024-02-29 12:00:00", run.cycles[0].time
    assert run.cycles[1].time == "2024-02-29 12:00:01", run.cycles[1].time
