from __future__ import annotations

from cases import case

import storage


@case(
    "ACC-FRZ-004",
    "crossing midnight at a month boundary writes the month freeze record",
    ("freeze", "flash"),
)
def month_freeze_record(ctx):
    run = ctx.run(cycles=3600)

    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert run.cycles[0].time == "2021-04-30 23:59:50"
    assert run.cycles[-1].time == "2021-05-01 00:59:49", run.cycles[-1].time

    blocks = storage.freeze_records(run.eeprom, run.flash)
    month = blocks["month"]
    assert month.manager.checksum_ok, "month freeze manager checksum is invalid"
    assert month.manager.count == 1, month.manager.count
    assert len(month.records) == 1, month.records
    record = month.records[0]
    assert record.sequence == 0xFFFFFFFF, record.sequence
    assert record.timestamp is not None, record.raw.hex()
    assert record.timestamp.strftime("%Y-%m-%d %H:%M:%S") == "2021-05-01 00:00:00"
    assert record.raw[0] == 0x06, record.raw.hex()
