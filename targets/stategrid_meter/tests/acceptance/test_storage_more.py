"""Acceptance cases for the persisted energy record.

The only evidence is the EEPROM image after a simulated power loss; nothing
here reads a process-internal value.
"""

from __future__ import annotations

from cases import case

import storage


@case(
    "ACC-STO-001",
    "three 32-pulse energy steps followed by a power loss leave 3.00 kWh in "
    "the active-forward slot of the EEPROM energy record with valid checksums, "
    "while the parameter-block copy stays empty",
    ("storage", "energy", "power"),
)
def energy_record_after_power_loss(ctx):
    scenario = "\n".join(
        [
            "at 0 energy clear",
            "at 1 meter const 3200 cal 128",
            "at 3 pulse 32",
            "at 4 pulse 32",
            "at 5 pulse 32",
            "at 7 power off",
            "at 9 power on",
        ]
    ) + "\n"
    run = ctx.run(cycles=12, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    record = storage.energy_record(run.eeprom)
    assert (record.forward[0].integer, record.forward[0].fraction) == (3, 0), (
        record.forward[0]
    )
    assert record.reverse[0].integer == 0, record.reverse[0]
    assert record.energy_checksum_ok, record.energy_checksum
    assert record.use_energy_checksum_ok, record.use_energy_checksum

    parameter_copy = storage.energy_record(run.eeprom, 198)
    assert parameter_copy.forward[0].integer == 0, parameter_copy.forward[0]
    assert parameter_copy.energy_checksum_ok, parameter_copy.energy_checksum
