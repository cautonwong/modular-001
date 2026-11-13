from __future__ import annotations

from cases import case

# First flash event block in the current layout. A layout change should fail
# this case loudly rather than move the assertion with it.
FLASH_EVENT_BLOCK = 0x1AE000


@case(
    "ACC-FLASH-001",
    "a full hour crosses midnight and writes the first flash event block",
    ("flash", "freeze"),
)
def midnight_freeze_writes_flash(ctx):
    run = ctx.run(cycles=3600)

    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert len(run.cycles) == 3600
    assert run.cycles[0].time == "2021-04-30 23:59:50"
    assert run.cycles[-1].time == "2021-05-01 00:59:49", run.cycles[-1].time
    assert run.reset_count == 0

    positions = [index for index, value in enumerate(run.flash) if value != 0xFF]
    assert positions, "a full hour wrote nothing to flash"
    assert positions[0] == FLASH_EVENT_BLOCK, f"first flash write is at {positions[0]:#x}"
    assert all(FLASH_EVENT_BLOCK <= index < FLASH_EVENT_BLOCK + 32 for index in positions), (
        f"unexpected flash writes at {positions[:8]}"
    )
    assert run.non_ff_eeprom() >= 180, (
        f"an hour persisted only {run.non_ff_eeprom()} EEPROM bytes"
    )
