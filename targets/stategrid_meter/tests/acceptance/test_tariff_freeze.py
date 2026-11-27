"""Acceptance cases for tariff selection and period freeze.

Every assertion reads a byte the firmware wrote to EEPROM/flash, or a 698
response captured from UART. Fixed start times and fixed seeds keep the runs
deterministic; pulses are injected one cycle before the fee they belong to
because the meter chip accumulates them at the start of the next cycle.
"""

from __future__ import annotations

from cases import case

import parameters
import protocol
import storage

BLANK_ADDRESS = b"\xff" * 6
BLANK_IMAGE = b"\xff" * storage.EEPROM_SIZE
PARAMETER_BASE = storage.EEPROM_PARAMETER_BLOCK_OFFSET
FACTORY_MODE = b"\xfb"
METER_CONST = 3200

BILL_MANAGER_OFFSET = PARAMETER_BASE + 120
BILL_REGION_OFFSET = storage.EEPROM_EVENT_BLOCK_OFFSET + 18280
BILL_REGION_SIZE = 3380


def _frame(apdu: bytes) -> bytes:
    return protocol.build_frame(apdu, sa=BLANK_ADDRESS, ca=0x10, ctrl=0x43)


def _get(piid: int, oad: int) -> bytes:
    return _frame(protocol.get_request(piid, oad))


def _set(piid: int, oad: int, data: bytes) -> bytes:
    return _frame(protocol.set_request(piid, oad, data))


def _write_645(data_id: str, value: bytes) -> bytes:
    data = (
        protocol.data_id_bytes(data_id)
        + b"\x00\x00\x00\x00"
        + b"\x00\x00\x00\x00"
        + value
    )
    body = (
        bytes([0x68])
        + BLANK_ADDRESS
        + bytes([0x68, protocol.WRITE_DATA, len(data)])
        + bytes((byte + protocol.DATA_OFFSET) & 0xFF for byte in data)
    )
    return body + bytes([protocol.checksum(body), 0x16])


def _entry(image: bytes, offset: int, data: bytes, size: int) -> bytes:
    assert len(data) <= size, (len(data), size)
    padded = bytes(data) + b"\xff" * (size - len(data))
    image = bytearray(image)
    image[PARAMETER_BASE + offset : PARAMETER_BASE + offset + size] = padded
    image[
        PARAMETER_BASE + offset + size : PARAMETER_BASE + offset + size + 2
    ] = ((0xA5A5 + sum(padded)) & 0xFFFF).to_bytes(2, "big")
    return bytes(image)


def _image(**fields: bytes) -> bytes:
    image = parameters.set_parameters(BLANK_IMAGE, {"meter_const": METER_CONST})
    for offset, data in fields.items():
        image = _entry(image, int(offset), data, len(data))
    return image


def _counts(seasons: int, tables: int, periods: int, fees: int, holidays: int) -> bytes:
    values = (seasons, tables, periods, fees, holidays)
    return bytes([0x01, 0x05, *sum([[0x11, value] for value in values], [])])


def _season_table(rows) -> bytes:
    out = bytes([0x01, len(rows)])
    for month, day, table in rows:
        out += bytes([0x02, 0x03, 0x11, month, 0x11, day, 0x11, table])
    return out


def _day_table(rows) -> bytes:
    out = bytes([0x01, len(rows)])
    for hour, minute, fee in rows:
        out += bytes([0x02, 0x03, 0x11, hour, 0x11, minute, 0x11, fee])
    return out


def _get_values(run) -> list:
    values = []
    for frame in protocol.iter_698_frames(run.uart_tx):
        apdu = protocol.parse_frame(frame).apdu
        if apdu[0] == 0x85:
            values.append(protocol.parse_get_response(apdu).values()[0])
    return values


def _set_dars(run) -> list[int]:
    dars = []
    for frame in protocol.iter_698_frames(run.uart_tx):
        apdu = protocol.parse_frame(frame).apdu
        if apdu[0] == 0x86:
            dars.append(protocol.parse_set_response(apdu)[1])
    return dars


def _energy_slots(value) -> list[int]:
    return [child.value for child in value.children]


@case(
    "ACC-TAR-001",
    "a season-table boundary selects day table 2 on 2021-05-01: the pulse "
    "before midnight lands in fee slot 1 and the pulse after it in slot 2, "
    "while the total is the sum of both",
    ("tariff", "energy", "698"),
)
def season_table_switch(ctx):
    scenario = "\n".join(
        [
            "at 0 energy clear",
            f"at 0 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
            f"at 1 uart 0 {_set(1, 0x400C0200, _counts(2, 2, 1, 2, 0)).hex()}",
            f"at 2 uart 0 {_set(2, 0x40140200, _season_table([(1, 1, 1), (5, 1, 2)])).hex()}",
            f"at 3 uart 0 {_set(3, 0x40160201, _day_table([(0, 0, 1)])).hex()}",
            f"at 4 uart 0 {_set(4, 0x40160202, _day_table([(0, 0, 2)])).hex()}",
            "at 5 meter const 3200 cal 128",
            "at 6 pulse 32",
            "at 12 pulse 32",
            f"at 14 uart 0 {_get(10, 0x00000400).hex()}",
            f"at 15 uart 0 {_get(11, 0x00000401).hex()}",
        ]
    ) + "\n"
    run = ctx.run(cycles=17, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _set_dars(run) == [0, 0, 0, 0], _set_dars(run)

    array, total = _get_values(run)
    assert _energy_slots(array) == [200, 100, 100], _energy_slots(array)
    assert total.value == 200, total.value


@case(
    "ACC-TAR-002",
    "a day-table period boundary at 00:15 moves the next pulse from fee 1 to "
    "fee 2, so OAD 0x00000402 holds 100 and the total holds 200",
    ("tariff", "energy", "698"),
)
def day_period_switch(ctx):
    scenario = "\n".join(
        [
            "at 0 energy clear",
            f"at 0 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
            f"at 1 uart 0 {_set(1, 0x400C0200, _counts(1, 1, 2, 2, 0)).hex()}",
            f"at 2 uart 0 {_set(2, 0x40140200, _season_table([(1, 1, 1)])).hex()}",
            f"at 3 uart 0 {_set(3, 0x40160201, _day_table([(0, 0, 1), (0, 15, 2)])).hex()}",
            "at 4 meter const 3200 cal 128",
            "at 12 pulse 32",
            "at 20 time '2021-05-01 00:14:50'",
            "at 31 pulse 32",
            f"at 33 uart 0 {_get(10, 0x00000400).hex()}",
            f"at 34 uart 0 {_get(11, 0x00000401).hex()}",
            f"at 35 uart 0 {_get(12, 0x00000402).hex()}",
        ]
    ) + "\n"
    run = ctx.run(cycles=38, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _set_dars(run) == [0, 0, 0], _set_dars(run)

    array, total, fee2 = _get_values(run)
    assert _energy_slots(array) == [200, 100, 100], _energy_slots(array)
    assert total.value == 200, total.value
    assert fee2.value == 100, fee2.value


WEEKEND_REST_MASK = bytes([0x04, 0x08, 0xFD])
WEEKEND_REST_TABLE = bytes([0x11, 0x03])
HOLIDAY_2021_05_01 = bytes(
    [0x02, 0x02, 0x1A, 0x07, 0xE5, 0x05, 0x01, 0x06, 0x11, 0x02])


def _holiday_tariff_setup(holiday_count: int) -> list[str]:
    return [
        "at 0 energy clear",
        f"at 0 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
        f"at 1 uart 0 {_set(1, 0x400C0200, _counts(1, 3, 1, 3, holiday_count)).hex()}",
        f"at 2 uart 0 {_set(2, 0x40140200, _season_table([(1, 1, 1)])).hex()}",
        f"at 3 uart 0 {_set(3, 0x40160201, _day_table([(0, 0, 1)])).hex()}",
        f"at 4 uart 0 {_set(4, 0x40160202, _day_table([(0, 0, 2)])).hex()}",
        f"at 5 uart 0 {_set(5, 0x40160203, _day_table([(0, 0, 3)])).hex()}",
        f"at 6 uart 0 {_set(6, 0x40120200, WEEKEND_REST_MASK).hex()}",
        f"at 7 uart 0 {_set(7, 0x40130200, WEEKEND_REST_TABLE).hex()}",
    ]


def _holiday_tariff_scenario() -> str:
    return "\n".join(
        _holiday_tariff_setup(1)
        + [
            f"at 8 uart 0 {_set(8, 0x40110201, HOLIDAY_2021_05_01).hex()}",
            "at 10 meter const 3200 cal 128",
            "at 16 pulse 32",
            "at 22 pulse 32",
            f"at 26 uart 0 {_get(10, 0x00000400).hex()}",
            f"at 27 uart 0 {_get(11, 0x40110201).hex()}",
        ]
    ) + "\n"


@case(
    "ACC-TAR-003",
    "the workday mask selects rest-day table 3 on Saturday 2021-05-01, and a "
    "698 SET 0x4011 holiday for that date reads back as 2021-05-01 table 2 "
    "and moves both pulses from fee 1 to fee 2",
    ("tariff", "energy", "holiday", "698"),
)
def holiday_and_rest_day(ctx):
    rest = "\n".join(
        _holiday_tariff_setup(0)
        + [
            "at 10 meter const 3200 cal 128",
            "at 16 pulse 32",
            "at 22 pulse 32",
            f"at 26 uart 0 {_get(10, 0x00000400).hex()}",
        ]
    ) + "\n"
    run = ctx.run(cycles=28, scenario=rest)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _set_dars(run) == [0, 0, 0, 0, 0, 0, 0], _set_dars(run)
    slots = _energy_slots(_get_values(run)[0])
    assert slots == [200, 0, 0, 200], slots

    run = ctx.run(cycles=30, scenario=_holiday_tariff_scenario())
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _set_dars(run) == [0, 0, 0, 0, 0, 0, 0, 0], _set_dars(run)
    array, holiday = _get_values(run)
    slots = _energy_slots(array)
    assert slots == [200, 0, 200, 0], slots
    assert holiday.value == (bytes([0x07, 0xE5, 0x05, 0x01, 0x06]), 2), holiday.value


@case(
    "ACC-TAR-006",
    "645 factory entry plus four 698 tariff SETs are readable through GET "
    "0x400C0200/0x40140200/0x4016020x and return the same tables after a "
    "restart on the written EEPROM image",
    ("tariff", "698", "persistence"),
)
def tariff_parameters_survive_restart(ctx):
    image = BLANK_IMAGE
    write_scenario = "\n".join(
        [
            "at 0 energy clear",
            f"at 0 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
            f"at 1 uart 0 {_set(1, 0x400C0200, _counts(2, 2, 1, 2, 0)).hex()}",
            f"at 2 uart 0 {_set(2, 0x40140200, _season_table([(1, 1, 1), (5, 1, 2)])).hex()}",
            f"at 3 uart 0 {_set(3, 0x40160201, _day_table([(0, 0, 1)])).hex()}",
            f"at 4 uart 0 {_set(4, 0x40160202, _day_table([(0, 0, 2)])).hex()}",
            f"at 6 uart 0 {_get(10, 0x400C0200).hex()}",
            f"at 7 uart 0 {_get(11, 0x40140200).hex()}",
            f"at 8 uart 0 {_get(12, 0x40160201).hex()}",
            f"at 9 uart 0 {_get(13, 0x40160202).hex()}",
        ]
    ) + "\n"
    written = ctx.run(cycles=11, scenario=write_scenario, seed_eeprom=image)
    assert written.exit_code == 0, f"firmware_sim exited {written.exit_code}"
    assert _set_dars(written) == [0, 0, 0, 0], _set_dars(written)

    read_scenario = "\n".join(
        [
            f"at 2 uart 0 {_get(1, 0x400C0200).hex()}",
            f"at 3 uart 0 {_get(2, 0x40140200).hex()}",
            f"at 4 uart 0 {_get(3, 0x40160201).hex()}",
            f"at 5 uart 0 {_get(4, 0x40160202).hex()}",
        ]
    ) + "\n"
    restarted = ctx.run(
        cycles=7,
        scenario=read_scenario,
        seed_eeprom=written.eeprom,
        seed_flash=written.flash,
    )
    assert restarted.exit_code == 0, f"firmware_sim exited {restarted.exit_code}"

    expected = [
        (2, 2, 1, 2, 0),
        ((1, 1, 1), (5, 1, 2)),
        ((0, 0, 1),),
        ((0, 0, 2),),
    ]
    assert [value.value for value in _get_values(written)] == expected
    assert [value.value for value in _get_values(restarted)] == expected


@case(
    "ACC-TAR-009",
    "a day-table boundary starting at 00:15 keeps the 00:14:59 pulse in fee 1 "
    "and the 00:15:00 pulse in fee 2, and a 2021-05-01 holiday table overrides "
    "rest-day table 3 for that Saturday",
    ("tariff", "boundary", "holiday", "698"),
)
def tou_boundary_and_holiday_priority(ctx):
    boundary = "\n".join(
        [
            "at 0 energy clear",
            f"at 0 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
            f"at 1 uart 0 {_set(1, 0x400C0200, _counts(1, 1, 2, 2, 0)).hex()}",
            f"at 2 uart 0 {_set(2, 0x40140200, _season_table([(1, 1, 1)])).hex()}",
            f"at 3 uart 0 {_set(3, 0x40160201, _day_table([(0, 0, 1), (0, 15, 2)])).hex()}",
            "at 4 meter const 3200 cal 128",
            "at 20 time '2021-05-01 00:14:50'",
            "at 28 pulse 32",
            "at 29 pulse 32",
            f"at 33 uart 0 {_get(10, 0x00000400).hex()}",
        ]
    ) + "\n"
    run = ctx.run(cycles=35, scenario=boundary)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _set_dars(run) == [0, 0, 0], _set_dars(run)
    assert run.cycles[29].time == "2021-05-01 00:14:59", run.cycles[29].time
    assert run.cycles[30].time == "2021-05-01 00:15:00", run.cycles[30].time
    array = _get_values(run)[0]
    assert _energy_slots(array) == [200, 100, 100], _energy_slots(array)

    holiday = ctx.run(cycles=30, scenario=_holiday_tariff_scenario())
    assert holiday.exit_code == 0, f"firmware_sim exited {holiday.exit_code}"
    assert _set_dars(holiday) == [0, 0, 0, 0, 0, 0, 0, 0], _set_dars(holiday)
    array, value = _get_values(holiday)
    slots = _energy_slots(array)
    assert slots == [200, 0, 200, 0], slots
    assert value.value == (bytes([0x07, 0xE5, 0x05, 0x01, 0x06]), 2), value.value


@case(
    "ACC-FRZ-002",
    "with minute scheme 1 set to a one-minute period, the 00:00:00 boundary "
    "writes one 10-byte record to the first flash sector and updates the "
    "scheme-1 log manager in the parameter block",
    ("freeze", "minute", "flash"),
)
def minute_freeze_scheme(ctx):
    image = _image(
        **{
            "3387": bytes([1, 0, 1] + [0, 0, 0] * 7),
            "3463": bytes([0, 1] + [0, 0] * 14),
            "5111": bytes([0, 1, 1, 1, 1, 1, 1, 1, 1]),
        }
    )
    run = ctx.run(cycles=15, seed_eeprom=image)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    manager = storage._read_manager(run.eeprom, PARAMETER_BASE + 0)
    assert manager.checksum_ok, manager
    assert manager.count == 1, manager
    assert manager.unit_size == 10, manager
    assert manager.max_count == 1, manager

    block = storage.log_block(manager, run.flash[0:4096], name="minute1")
    assert len(block.records) == 1, block.records
    record = block.records[0]
    assert record.sequence == 0, record.sequence
    assert record.timestamp is not None, record.raw.hex()
    assert record.timestamp.strftime("%Y-%m-%d %H:%M:%S") == "2021-05-01 00:00:00"
    assert record.raw == bytes.fromhex("06000000001c281f5680"), record.raw.hex()


@case(
    "ACC-FRZ-003",
    "with only the hour period enabled, crossing midnight writes one hour "
    "freeze record at 00:00:00 through the hour manager at parameter offset 96",
    ("freeze", "hour", "flash"),
)
def hour_freeze_record(ctx):
    image = _image(
        **{
            "3413": bytes([1, 0, 1]),
            "3418": bytes([0, 0, 0]),
        }
    )
    run = ctx.run(cycles=15, seed_eeprom=image)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    hour = storage.freeze_records(run.eeprom, run.flash)["hour"]
    assert hour.manager.checksum_ok, hour.manager
    assert hour.manager.count == 1, hour.manager
    assert hour.manager.unit_size == 10, hour.manager
    assert len(hour.records) == 1, hour.records
    record = hour.records[0]
    assert record.sequence == 0xFFFFFFFF, record.sequence
    assert record.timestamp is not None, record.raw.hex()
    assert record.timestamp.strftime("%Y-%m-%d %H:%M:%S") == "2021-05-01 00:00:00"
    assert record.raw == bytes.fromhex("06ffffffff1c281f5680"), record.raw.hex()


@case(
    "ACC-FRZ-005",
    "a bill schedule of day 1 hour 0 writes one bill freeze record at "
    "2021-05-01 00:00:00 to the bill manager and advances LastBillingTime to "
    "the settled date",
    ("freeze", "bill", "flash"),
)
def bill_freeze_record(ctx):
    image = _image(
        **{
            "3423": bytes([1, 0, 1]),
            "6728": bytes([1, 0, 0x63, 0, 0x63, 0]),
            "3917": bytes([21, 4, 1, 0, 0, 0]),
        }
    )
    run = ctx.run(cycles=15, seed_eeprom=image)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    manager = storage._read_manager(run.eeprom, BILL_MANAGER_OFFSET)
    assert manager.checksum_ok, manager
    assert manager.count == 1, manager
    assert manager.unit_size == 10, manager
    assert manager.max_count == 12, manager

    region = run.eeprom[BILL_REGION_OFFSET : BILL_REGION_OFFSET + BILL_REGION_SIZE]
    block = storage.log_block(manager, region, name="bill")
    assert len(block.records) == 1, block.records
    record = block.records[0]
    assert record.timestamp is not None, record.raw.hex()
    assert record.timestamp.strftime("%Y-%m-%d %H:%M:%S") == "2021-05-01 00:00:00"
    assert record.raw == bytes.fromhex("06ffffffff1c281f5680"), record.raw.hex()
    assert (
        run.eeprom[PARAMETER_BASE + 3917 : PARAMETER_BASE + 3923]
        == bytes([0, 0, 0, 1, 5, 21])
    ), run.eeprom[PARAMETER_BASE + 3917 : PARAMETER_BASE + 3923].hex()


@case(
    "ACC-FRZ-006",
    "ACTION 0x5000 method 3 with a 2-minute delay appends a second instant "
    "freeze record at 23:59:00 to the EEPROM instant log",
    ("freeze", "instant", "698"),
)
def instant_freeze_action(ctx):
    action = _frame(protocol.action_request(1, 0x5000, 3, param=b"\x12\x00\x02"))
    run = ctx.run(cycles=10, scenario=f"at 2 uart 0 {action.hex()}\n")
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    block = storage.event_records(run.eeprom)
    assert block.manager.checksum_ok, block.manager
    assert block.manager.count == 2, block.manager
    assert len(block.records) == 2, block.records

    boot, delayed = block.records
    assert boot.raw == bytes.fromhex("06ffffffff1c281f5644"), boot.raw.hex()
    assert delayed.sequence == 0, delayed.sequence
    assert delayed.timestamp is not None, delayed.raw.hex()
    assert delayed.timestamp.strftime("%Y-%m-%d %H:%M:%S") == "2021-04-30 23:59:00"
    assert delayed.raw == bytes.fromhex("06000000001c281f5644"), delayed.raw.hex()


@case(
    "ACC-FRZ-007",
    "a power loss from 2021-04-28 18:00 with the last day freeze on 2021-04-27 "
    "backfills records for 2021-04-29 and 2021-04-30 on power-on",
    ("freeze", "day", "backfill"),
)
def day_freeze_backfill(ctx):
    image = _image(
        **{
            "3418": bytes([1, 0, 1]),
            "3925": bytes([0, 0, 0, 28, 4, 21]),
        }
    )
    # DI A8F00053 carries the power-down time as BCD.
    power_down = bytes([0x00, 0x00, 0x12, 0x28, 0x04, 0x21])
    scenario = "\n".join(
        [
            "at 0 energy clear",
            f"at 1 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
            f"at 2 uart 0 {_write_645('A8F00053', power_down).hex()}",
            "at 3 power off",
            "at 6 power on",
        ]
    ) + "\n"
    run = ctx.run(cycles=10, scenario=scenario, seed_eeprom=image)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    day = storage.freeze_records(run.eeprom, run.flash)["day"]
    assert day.manager.checksum_ok, day.manager
    assert day.manager.count == 2, day.manager
    assert [record.sequence for record in day.records] == [0xFFFFFFFF, 0]
    assert [
        record.timestamp.strftime("%Y-%m-%d %H:%M:%S")
        for record in day.records
        if record.timestamp is not None
    ] == ["2021-04-29 00:00:00", "2021-04-30 00:00:00"]
    assert (
        run.eeprom[PARAMETER_BASE + 3925 : PARAMETER_BASE + 3931]
        == bytes([0, 0, 0, 30, 4, 21])
    ), run.eeprom[PARAMETER_BASE + 3925 : PARAMETER_BASE + 3931].hex()


@case(
    "ACC-FRZ-011",
    "a 20-day outage backfills only the 7 most recent days (2021-04-24 to "
    "2021-04-30), and re-running the same outage on that image appends no "
    "duplicate record",
    ("freeze", "day", "backfill", "cap"),
)
def backfill_cap_and_no_duplicate(ctx):
    image = _image(
        **{
            "3418": bytes([1, 0, 1]),
            "3925": bytes([0, 0, 0, 1, 4, 21]),
        }
    )
    power_down = bytes([0x00, 0x00, 0x12, 0x10, 0x04, 0x21])
    scenario = "\n".join(
        [
            "at 0 energy clear",
            f"at 1 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
            f"at 2 uart 0 {_write_645('A8F00053', power_down).hex()}",
            "at 3 power off",
            "at 6 power on",
        ]
    ) + "\n"
    first = ctx.run(cycles=10, scenario=scenario, seed_eeprom=image)
    assert first.exit_code == 0, f"firmware_sim exited {first.exit_code}"

    day = storage.freeze_records(first.eeprom, first.flash)["day"]
    assert day.manager.checksum_ok, day.manager
    assert day.manager.count == 7, day.manager
    assert [record.sequence for record in day.records] == [
        0xFFFFFFFF,
        0,
        1,
        2,
        3,
        4,
        5,
    ]
    assert [
        record.timestamp.strftime("%Y-%m-%d")
        for record in day.records
        if record.timestamp is not None
    ] == [f"2021-04-{day_of_month:02d}" for day_of_month in range(24, 31)]
    assert first.eeprom[PARAMETER_BASE + 3925 : PARAMETER_BASE + 3931] == bytes(
        [0, 0, 0, 30, 4, 21]
    )

    region = first.flash[
        storage.DAY_FREEZE_REGION_OFFSET : storage.DAY_FREEZE_REGION_OFFSET
        + storage.DAY_FREEZE_REGION_SIZE
    ]
    second = ctx.run(
        cycles=10,
        scenario=scenario,
        seed_eeprom=first.eeprom,
        seed_flash=first.flash,
    )
    assert second.exit_code == 0, f"firmware_sim exited {second.exit_code}"
    again = storage.freeze_records(second.eeprom, second.flash)["day"]
    assert again.manager.count == 7, again.manager
    assert second.flash[
        storage.DAY_FREEZE_REGION_OFFSET : storage.DAY_FREEZE_REGION_OFFSET
        + storage.DAY_FREEZE_REGION_SIZE
    ] == region
