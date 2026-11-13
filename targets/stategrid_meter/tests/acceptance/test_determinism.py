from __future__ import annotations

from cases import case


@case(
    "ACC-DETERMINISM-001",
    "two fresh runs of the same scenario produce the same trace and images",
    ("determinism",),
)
def fresh_runs_are_deterministic(ctx):
    first = ctx.run(cycles=65)
    second = ctx.run(cycles=65)

    assert first.exit_code == 0, f"first run exited {first.exit_code}"
    assert second.exit_code == 0, f"second run exited {second.exit_code}"
    assert [(cycle.index, cycle.time, cycle.ms) for cycle in first.cycles] == [
        (cycle.index, cycle.time, cycle.ms) for cycle in second.cycles
    ], "cycle trace differs between identical fresh runs"
    assert first.eeprom == second.eeprom, "EEPROM image differs between identical fresh runs"
    assert first.flash == second.flash, "flash image differs between identical fresh runs"
    assert first.reset_count == second.reset_count == 0
