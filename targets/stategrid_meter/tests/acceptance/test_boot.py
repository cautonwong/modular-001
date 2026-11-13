from __future__ import annotations

from cases import case
from sim import EEPROM_SIZE, FLASH_SIZE


@case(
    "ACC-BOOT-001",
    "cold boot on blank media creates the meter images and starts the reset counter",
    ("boot", "smoke"),
)
def cold_boot(ctx):
    run = ctx.run(cycles=3)

    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}: {run.stderr[-200:]}"
    assert run.done_line == "[sim] done cycles=3", f"unexpected completion line: {run.done_line!r}"
    assert len(run.cycles) == 3, f"expected 3 cycle lines, got {len(run.cycles)}"
    assert [cycle.time for cycle in run.cycles] == [
        "2021-04-30 23:59:50",
        "2021-04-30 23:59:51",
        "2021-04-30 23:59:52",
    ], [cycle.time for cycle in run.cycles]
    assert [cycle.ms for cycle in run.cycles] == [0, 1000, 2000]

    assert len(run.eeprom) == EEPROM_SIZE, f"EEPROM is {len(run.eeprom)} bytes"
    assert len(run.flash) == FLASH_SIZE, f"flash is {len(run.flash)} bytes"
    assert run.reset_count == 0, f"first boot reset counter is {run.reset_count}"
    assert run.non_ff_eeprom() > 0, "the first boot persisted nothing"
    assert run.non_ff_flash() == 0, "flash was written before the first freeze"
