"""Acceptance cases for metering rows beyond ACC-MET-001/002/006.

Evidence is external only: 698 responses, 645 responses, and EEPROM bytes
decoded by storage.py. Fixed start times and blank images keep runs
deterministic.
"""

from __future__ import annotations

from cases import case

import parameters
import protocol
import storage

BLANK_ADDRESS = b"\xff" * 6
BLANK_IMAGE = b"\xff" * storage.EEPROM_SIZE
METER_CONST = 3200
FACTORY_MODE = b"\xfb"
ENERGY_BASE = (0x00012345).to_bytes(4, "little")
THRESHOLD_30 = (30).to_bytes(2, "little")


def _get(piid: int, oad: int) -> bytes:
    return protocol.build_frame(
        protocol.get_request(piid, oad), sa=BLANK_ADDRESS, ca=0x10, ctrl=0x43
    )


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


def _read_645(data_id: str) -> bytes:
    data = protocol.data_id_bytes(data_id)
    body = (
        bytes([0x68])
        + BLANK_ADDRESS
        + bytes([0x68, protocol.READ_DATA, len(data)])
        + bytes((byte + protocol.DATA_OFFSET) & 0xFF for byte in data)
    )
    return body + bytes([protocol.checksum(body), 0x16])


def _action(piid: int, oi: int, method: int) -> bytes:
    return protocol.build_frame(
        protocol.action_request(piid, oi, method, param=b"\x00"),
        sa=BLANK_ADDRESS, ca=0x10, ctrl=0x43
    )


def _get_values(run) -> list[protocol.AxdrValue]:
    values = []
    for frame in protocol.iter_698_frames(run.uart_tx):
        apdu = protocol.parse_frame(frame).apdu
        if apdu[0] == 0x85:
            values.append(protocol.parse_get_response(apdu).values()[0])
    return values


def _image() -> bytes:
    return parameters.set_parameters(BLANK_IMAGE, {"meter_const": METER_CONST})


@case(
    "ACC-MET-003",
    "pulses injected while tariff 2 is selected add to the F2 slot of OAD "
    "0x00000400 and to the total, leaving the other slots unchanged",
    ("metering", "energy", "tariff"),
)
def multi_tariff_energy(ctx):
    image = parameters.set_parameters(BLANK_IMAGE, {"meter_const": METER_CONST, "fee_count": 3})
    run = ctx.run(
        cycles=7,
        scenario=(
            "at 0 energy clear\n"
            "at 1 meter const 3200 cal 128 tariff 2\n"
            "at 1 pulse 32\n"
            f"at 3 uart 0 {_get(1, 0x00000400).hex()}\n"
            f"at 4 uart 0 {_get(2, 0x00000401).hex()}\n"
            f"at 5 uart 0 {_get(3, 0x00000403).hex()}\n"
        ),
        seed_eeprom=image,
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    array, total, slot2 = _get_values(run)
    assert array.tag == protocol.AXDR_ARRAY, array.tag
    assert [child.value for child in array.children] == [100, 0, 100, 0], [
        child.value for child in array.children
    ]
    assert total.value == 100, total.value
    assert slot2.value == 100, slot2.value


@case(
    "ACC-MET-004",
    "645 DI A8FFFFF0 sets the forward active energy after factory-mode entry "
    "(A8000100=FB); the value survives a restart and a 698 ACTION 0x4300 "
    "method 3 clears it back to zero on EEPROM and on the 698 read",
    ("metering", "energy"),
)
def set_and_clear_energy(ctx):
    set_run = ctx.run(
        cycles=6,
        scenario=(
            "at 0 energy clear\n"
            "at 1 meter const 3200 cal 128\n"
            f"at 2 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}\n"
            f"at 4 uart 0 {_write_645('A8FFFFF0', ENERGY_BASE).hex()}\n"
        ),
        seed_eeprom=_image(),
    )
    assert set_run.exit_code == 0, f"firmware_sim exited {set_run.exit_code}"
    record = storage.energy_record(set_run.eeprom)
    assert record.forward[0].integer == 74565, record.forward[0]
    assert record.energy_checksum_ok, record
    assert storage.energy_record(set_run.eeprom, 198).forward[0].integer == 74565

    restart = ctx.run(
        cycles=10,
        scenario=(
            "at 1 meter const 3200 cal 128\n"
            f"at 3 uart 0 {_get(1, 0x00000401).hex()}\n"
            f"at 4 uart 0 {_read_645('00010000').hex()}\n"
            f"at 6 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}\n"
            f"at 8 uart 0 {_action(2, 0x4300, 3).hex()}\n"
        ),
        seed_eeprom=set_run.eeprom,
    )
    assert restart.exit_code == 0, f"firmware_sim exited {restart.exit_code}"
    total = _get_values(restart)[0]
    assert total.value == 7456500, total.value

    frames = protocol.iter_645_frames(restart.uart_tx)
    assert frames, f"no 645 read response: {restart.uart_tx.hex()}"
    _, control, raw = protocol.parse_frame_645(frames[0])
    assert control == 0x91, hex(control)
    data = bytes((byte - protocol.DATA_OFFSET) & 0xFF for byte in raw)
    assert data[4:] == bytes.fromhex("65450700"), data.hex()

    cleared = storage.energy_record(restart.eeprom)
    assert cleared.forward[0].integer == 0, cleared.forward[0]
    assert cleared.energy_checksum_ok, cleared
    assert storage.energy_record(restart.eeprom, 198).forward[0].integer == 0

    final = ctx.run(
        cycles=4,
        scenario=(
            "at 1 meter const 3200 cal 128\n"
            f"at 3 uart 0 {_get(1, 0x00000401).hex()}\n"
        ),
        seed_eeprom=restart.eeprom,
    )
    assert final.exit_code == 0, f"firmware_sim exited {final.exit_code}"
    assert _get_values(final)[0].value == 0


def _threshold_scenario(steps: int) -> str:
    lines = [
        "at 0 energy clear",
        f"at 2 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
        f"at 4 uart 0 {_write_645('A8F00055', THRESHOLD_30).hex()}",
    ]
    cycle = 5
    for _ in range(steps):
        lines.append(f"at {cycle} meter const 3200 cal 128")
        lines.append(f"at {cycle} pulse 32")
        cycle += 1
    lines.append(f"at {cycle} uart 0 {_get(1, 0x00000401).hex()}")
    return "\n".join(lines) + "\n"


@case(
    "ACC-MET-008",
    "with EC_SaveDiff set to 30 via 645 DI A8F00055, the 30th 0.01 kWh step "
    "writes the EEPROM energy copy (parameter_1+198) while 29 steps do not",
    ("metering", "energy", "persistence"),
)
def energy_save_threshold(ctx):
    image = _image()
    saved = ctx.run(cycles=40, scenario=_threshold_scenario(30), seed_eeprom=image)
    assert saved.exit_code == 0, f"firmware_sim exited {saved.exit_code}"
    assert _get_values(saved)[0].value == 3000
    assert saved.eeprom[194:196] == b"\x1e\x00", saved.eeprom[194:198].hex()
    assert saved.eeprom[196:198] == storage.checksum16(
        saved.eeprom[194:196]
    ).to_bytes(2, "big"), saved.eeprom[194:198].hex()
    record = storage.energy_record(saved.eeprom, 198)
    assert record.forward[0].integer == 30, record.forward[0]
    assert record.energy_checksum_ok, record

    early = ctx.run(cycles=40, scenario=_threshold_scenario(29), seed_eeprom=image)
    assert early.exit_code == 0, f"firmware_sim exited {early.exit_code}"
    assert _get_values(early)[0].value == 2900
    record = storage.energy_record(early.eeprom, 198)
    assert record.forward[0].integer == 0, record.forward[0]
    assert record.energy_checksum_ok, record


def _set(piid: int, oad: int, data: bytes) -> bytes:
    return protocol.build_frame(
        protocol.set_request(piid, oad, data), sa=BLANK_ADDRESS, ca=0x10, ctrl=0x43
    )


def _counts(seasons: int, tables: int, periods: int, fees: int, holidays: int) -> bytes:
    values = (seasons, tables, periods, fees, holidays)
    return bytes([0x01, 0x05, *sum([[0x11, value] for value in values], [])])


@case(
    "ACC-MET-011",
    "a tariff-count write with 13 fees is rejected at configuration time, "
    "while the same write with 2 fees succeeds and reads back",
    ("metering", "tariff", "validation"),
)
def invalid_fee_count_rejected(ctx):
    scenario = (
        f"at 0 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}\n"
        f"at 1 uart 0 {_set(1, 0x400C0200, _counts(1, 1, 1, 13, 0)).hex()}\n"
        f"at 2 uart 0 {_set(2, 0x400C0200, _counts(1, 1, 1, 2, 0)).hex()}\n"
        f"at 3 uart 0 {_get(3, 0x400C0200).hex()}\n"
    )
    run = ctx.run(cycles=6, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    apdus = [protocol.parse_frame(frame).apdu for frame in protocol.iter_698_frames(run.uart_tx)]
    assert len(apdus) == 3, [apdu.hex() for apdu in apdus]
    assert protocol.parse_set_response(apdus[0]) == (1, 19)
    assert protocol.parse_set_response(apdus[1]) == (2, 0)
    assert protocol.parse_get_response(apdus[2]).values()[0].value == (1, 1, 1, 2, 0)


@case(
    "ACC-MET-007",
    "a missing metering chip raises the metering-chip fault event after its "
    "debounce and the event carries a timestamp",
    ("metering", "fault", "event"),
)
def metering_chip_fault_event(ctx):
    scenario = (
        "at 1 chip ok\n"
        "at 2 chip fault\n"
        f"at 300 uart 0 {_get(1, 0x302F0400).hex()}\n"
        f"at 301 uart 0 {_get(2, 0x302F0A00).hex()}\n"
    )
    run = ctx.run(cycles=304, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    values = _get_values(run)
    assert len(values) == 2, [value.value for value in values]
    assert values[0].value == 1, values[0].value
    assert b"\x1c" in values[1].payload or values[1].children, values[1]


@case(
    "ACC-MET-012",
    "injected temperature and battery ADC values are readable over 698 with "
    "their documented scaling",
    ("metering", "adc", "temperature"),
)
def temperature_and_battery(ctx):
    scenario = (
        "at 1 chip ok\n"
        "at 2 adc temperature 2500\n"
        "at 3 adc battery 33000\n"
        f"at 5 uart 0 {_get(1, 0x20100200).hex()}\n"
        f"at 6 uart 0 {_get(2, 0x20110200).hex()}\n"
    )
    run = ctx.run(cycles=8, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    values = _get_values(run)
    assert [value.value for value in values] == [25, 330], [
        value.value for value in values
    ]
