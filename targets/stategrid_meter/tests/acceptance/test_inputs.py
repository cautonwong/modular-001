from __future__ import annotations

from cases import case

import protocol
import storage

BLANK_SA = b"\xff" * 6

# meter_const is pulses per kWh. A 3200 imp/kWh meter therefore advances one
# 0.01 kWh step every 32 pulses. One kWh is 10000 units at 0.0001 kWh
# resolution, so 32 pulses is exactly 100 units.
METER_CONST = 3200
PULSES_PER_STEP = METER_CONST // 100


def _frame(oad: int) -> str:
    return protocol.build_frame(
        protocol.get_request(1, oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
    ).hex()


def _get_values(run) -> list:
    values = []
    for frame in protocol.iter_698_frames(run.uart_tx):
        result = protocol.parse_get_response(protocol.parse_frame(frame).apdu)
        values.extend(result.values())
    return values


@case(
    "ACC-INPUT-PULSE",
    "pulse input is converted to active energy using meter_const and the cal word",
    ("inputs", "metering"),
)
def pulse_energy(ctx):
    seed = ctx.run(cycles=4)
    assert seed.exit_code == 0, f"seed run exited {seed.exit_code}"

    scenario = (
        "at 0 energy clear\n"
        f"at 1 meter const {METER_CONST} cal 128\n"
        f"at 1 pulse {PULSES_PER_STEP}\n"
        f"at 3 uart 0 {_frame(0x00000401)}\n"
        f"at 5 uart 0 {_frame(0x00000201)}\n"
    )
    run = ctx.run(cycles=6, seed_eeprom=seed.eeprom, scenario=scenario)
    assert run.exit_code == 0, f"pulse run exited {run.exit_code}"

    precise, coarse = _get_values(run)
    assert precise.tag == protocol.AXDR_LONG64, precise.tag
    assert precise.value == 100, precise.value
    assert coarse.tag == protocol.AXDR_DOUBLE_LONG, coarse.tag
    assert coarse.value == 1, coarse.value


@case(
    "ACC-INPUT-COVER",
    "cover input writes the mbase status and event_pro records one start record, "
    "revised in place when the cover closes",
    ("inputs", "events"),
)
def cover_event(ctx):
    seed = ctx.run(cycles=4)
    assert seed.exit_code == 0, f"seed run exited {seed.exit_code}"

    get = _frame(0x301B0400)
    scenario = (
        "at 2 cover open\n"
        f"at 8 uart 0 {get}\n"
        "at 10 cover closed\n"
        f"at 14 uart 0 {get}\n"
    )
    run = ctx.run(cycles=16, seed_eeprom=seed.eeprom, scenario=scenario)
    assert run.exit_code == 0, f"cover run exited {run.exit_code}"

    started, ended = _get_values(run)
    assert started.tag == protocol.AXDR_LONG_UNSIGNED, started.tag
    assert started.value == 1, started.value
    assert ended.value == 1, ended.value


@case(
    "ACC-INPUT-POWER",
    "power loss and restore runs the low-power pipeline and returns to the metering pipeline",
    ("inputs", "power"),
)
def power_cycle(ctx):
    run = ctx.run(cycles=8, verbose=True, scenario="at 1 power off\nat 3 power on\n")
    assert run.exit_code == 0, f"power run exited {run.exit_code}"
    assert run.non_ff_eeprom() > 0, "power run wrote no persistent EEPROM"

    pipelines = [hit.pipeline for hit in run.filters]
    assert "pl_lowpower_wakeup" in pipelines, pipelines
    wake = pipelines.index("pl_lowpower_wakeup")
    assert "app_meter_base" in pipelines[:wake], pipelines
    assert "app_meter_base" in pipelines[wake + 1 :], pipelines
    assert "Electrical_measurement" in run.stages, run.stages

    assert run.lcd_log, "no LCD capture lines"
    for line in run.lcd_log:
        fields = line.split()
        assert fields[0] == "cycle", line
        assert fields[2] == "time", line
        assert fields[5] == "fb", line
        assert len(fields[6]) == 320, line


@case(
    "ACC-POWER-001",
    "power loss records the outage and leaves the energy record in persistent EEPROM",
    ("inputs", "power", "persistence"),
)
def power_loss_persists(ctx):
    seed = ctx.run(cycles=4)
    assert seed.exit_code == 0, f"seed run exited {seed.exit_code}"

    scenario = (
        "at 0 energy clear\n"
        f"at 1 meter const {METER_CONST} cal 128\n"
        f"at 1 pulse {PULSES_PER_STEP}\n"
        "at 4 power off\n"
    )
    off = ctx.run(cycles=6, seed_eeprom=seed.eeprom, scenario=scenario)
    assert off.exit_code == 0, f"power off run exited {off.exit_code}"
    record = storage.energy_record(off.eeprom)
    assert record.forward[0].integer == 1, record.forward[0]

    event = _frame(0x30110400)
    run = ctx.run(
        cycles=6,
        seed_eeprom=off.eeprom,
        scenario=f"at 1 uart 0 {event}\n",
    )
    assert run.exit_code == 0, f"restart exited {run.exit_code}"
    count = _get_values(run)[0]
    assert count.value == 1, count.value
