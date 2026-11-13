from __future__ import annotations

from datetime import datetime

from cases import case


@case(
    "ACC-RTC-001",
    "one RTC tick per simulated second crosses midnight and survives the hardware resync",
    ("rtc", "time"),
)
def rtc_ticks_cross_midnight(ctx):
    run = ctx.run(cycles=35)

    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert len(run.cycles) == 35, f"expected 35 cycle lines, got {len(run.cycles)}"
    assert run.cycles[0].time == "2021-04-30 23:59:50"
    assert run.cycles[-1].time == "2021-05-01 00:00:24", run.cycles[-1].time

    times = [datetime.strptime(cycle.time, "%Y-%m-%d %H:%M:%S") for cycle in run.cycles]
    deltas = [(later - earlier).total_seconds() for earlier, later in zip(times, times[1:])]
    assert set(deltas) == {1.0}, f"non one-second RTC deltas: {sorted(set(deltas))}"
    assert [cycle.ms for cycle in run.cycles] == [index * 1000 for index in range(35)]

    # Rtc_Prossecc() re-reads the hardware RTC at second 20. An inert RTC stub
    # leaves the clock stuck there, so reaching the next minute proves the path.
    assert "2021-05-01 00:00:20" in [cycle.time for cycle in run.cycles]
    assert times[-1] > times[-2] > datetime(2021, 5, 1, 0, 0, 20)
