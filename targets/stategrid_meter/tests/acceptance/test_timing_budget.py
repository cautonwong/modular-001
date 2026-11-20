"""ACC-PERF-001: pass-count timing budget for the stategrid superloop.

The host simulator models one loop pass as one second and has no independent
time source (tests/sim/sim_trace.c injects instance->ms; the firmware never
advances it), so pass counts are the only automatic proxy for scheduling
latency. This case bounds hops, not milliseconds, and makes no wall-clock
assertion: host execution time is not firmware time.

B1 caps input-to-effect latency at one pass. B2 pins per-pass work: the 14
business stages once each in fixed order, one app_meter_base filter, one RTC
message, and at most one extra record filter/message.
"""

from __future__ import annotations

from collections import defaultdict

from cases import case
from sim import CYCLE_RE, FILTER_RE, MESSAGE_RE, STAGE_RE

import protocol

STAGES = (
    "SecTick_Task",
    "Rtc_Prossecc",
    "TouFeePro",
    "Electrical_measurement",
    "Energy_Pluse_Count",
    "Energy_Meter",
    "freeze_pro",
    "event_pro",
    "Key_Display",
    "Display",
    "backlight_pro",
    "do_relay",
    "relay_led_ctl",
    "IOStateCheck",
)
RTC_MESSAGE = (0x0D, 0x16, 0x01)

READ_AT = 10
TRIP_AT = 20
CLOSE_AT = 30
POWER_OFF_AT = 60
POWER_ON_AT = 80
B1_CYCLES = 100
B2_CYCLES = 101

BLANK_ADDRESS = b"\xff" * 6
RELAY_OI = 0x8000
TRIP = bytes.fromhex("0205110005f205020111001200001100")
CLOSE = bytes.fromhex("0205110005f205020111011200001100")


def _frame(control: int, data: bytes) -> bytes:
    body = bytes([0x68]) + BLANK_ADDRESS + bytes([0x68, control, len(data)])
    body += bytes((byte + protocol.DATA_OFFSET) & 0xFF for byte in data)
    return body + bytes([protocol.checksum(body), 0x16])


def _action(piid: int, method: int, param: bytes) -> str:
    return protocol.build_frame(
        protocol.action_request(piid, RELAY_OI, method, param=param),
        sa=BLANK_ADDRESS,
        ca=0x10,
        ctrl=0x43,
    ).hex()


def _passes(run):
    work = defaultdict(lambda: {"stages": [], "filters": [], "messages": []})
    current = None
    for line in run.trace:
        match = CYCLE_RE.match(line)
        if match:
            current = int(match.group(1))
            work[current]
            continue
        match = STAGE_RE.match(line)
        if match:
            work[current]["stages"].append(match.group(1))
            continue
        match = FILTER_RE.match(line)
        if match:
            work[current]["filters"].append(match.group(1))
            continue
        match = MESSAGE_RE.match(line)
        if match:
            work[current]["messages"].append(
                (int(match.group(1), 16), int(match.group(2), 16), int(match.group(3), 16))
            )
    return work


@case(
    "ACC-PERF-001",
    "input to pass-stamped effect stays within one loop pass, and every pass "
    "runs the 14 business stages once each in fixed order with exactly one "
    "app_meter_base filter and one RTC message",
    ("performance", "timing", "metering", "relay", "power"),
)
def timing_budget(ctx):
    run = ctx.run(cycles=B2_CYCLES, verbose=True)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    work = _passes(run)
    assert sorted(work) == list(range(B2_CYCLES)), sorted(work)
    for cycle, item in work.items():
        assert item["stages"] == list(STAGES), (cycle, item["stages"])
        assert item["filters"].count("app_meter_base") == 1, (cycle, item["filters"])
        assert len(item["filters"]) - item["filters"].count("app_meter_base") <= 1, (
            cycle,
            item["filters"],
        )
        rtc = [message for message in item["messages"] if message == RTC_MESSAGE]
        assert len(rtc) == 1, (cycle, item["messages"])
        assert len(item["messages"]) - len(rtc) <= 1, (cycle, item["messages"])

    read = _frame(0x11, protocol.data_id_bytes("00010000"))
    scenario = (
        f"at {READ_AT} uart 0 {read.hex()}\n"
        f"at {TRIP_AT} uart 0 {_action(1, 0x81, TRIP)}\n"
        f"at {CLOSE_AT} uart 0 {_action(2, 0x82, CLOSE)}\n"
        f"at {POWER_OFF_AT} power off\n"
        f"at {POWER_ON_AT} power on\n"
    )
    run = ctx.run(
        cycles=B1_CYCLES,
        verbose=True,
        scenario=scenario,
        start_time="2021-04-30 12:00:00",
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    # The 645 response is not pass-stamped; it only proves the read was served.
    frames = protocol.iter_645_frames(run.uart_tx)
    assert [protocol.parse_frame_645(frame)[1] for frame in frames] == [0x91], (
        f"645 read was not serviced: {run.uart_tx.hex()}"
    )

    actions = [
        (int(parts[1]), parts[-1]) for parts in (line.split() for line in run.relay_log)
    ]

    def effect_cycle(after: int, action: str) -> int:
        return next(cycle for cycle, name in actions if cycle >= after and name == action)

    work = _passes(run)
    live = [cycle for cycle, item in work.items() if len(item["stages"]) == len(STAGES)]
    stopped = [cycle for cycle in work if cycle >= POWER_OFF_AT and cycle not in live]
    resumed = [cycle for cycle in live if cycle > POWER_OFF_AT]
    assert stopped and resumed, (sorted(work), live)

    latency = {
        "698 disconnect": effect_cycle(TRIP_AT, "disconnect") - TRIP_AT,
        "698 connect": effect_cycle(CLOSE_AT, "connect") - CLOSE_AT,
        "power off": min(stopped) - POWER_OFF_AT,
        "power on": min(resumed) - POWER_ON_AT,
        "power-on relay re-assert": effect_cycle(POWER_ON_AT, "connect") - POWER_ON_AT,
    }
    assert max(latency.values()) <= 1, latency
    print(f"ACC-PERF-001 B1 latency passes={latency}")
