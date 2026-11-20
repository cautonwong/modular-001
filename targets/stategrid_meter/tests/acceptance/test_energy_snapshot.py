"""Acceptance cases for the dual-slot energy snapshot.

Evidence is external only: the EEPROM image decoded by storage.py and 698
energy responses. A run that ends without a power-off save is the power cut
between commits; the next boot must restore the last committed generation and
nothing after it.
"""

from __future__ import annotations

import struct
import zlib

from cases import case

import parameters
import protocol
import storage

BLANK_ADDRESS = b"\xff" * 6
BLANK_IMAGE = b"\xff" * storage.EEPROM_SIZE
METER_CONST = 3200
FACTORY_MODE = b"\xfb"
THRESHOLD_30 = (30).to_bytes(2, "little")
ENERGY_OAD = 0x00000401
PULSES_PER_STEP = METER_CONST // 100


def _get(piid: int, oad: int) -> bytes:
    return protocol.build_frame(
        protocol.get_request(piid, oad), sa=BLANK_ADDRESS, ca=0x10, ctrl=0x43
    )


def _get_value(run) -> int:
    for frame in protocol.iter_698_frames(run.uart_tx):
        apdu = protocol.parse_frame(frame).apdu
        if apdu[0] == 0x85:
            return protocol.parse_get_response(apdu).values()[0].value
    raise AssertionError(f"no 698 GET response: {run.uart_tx.hex()}")


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


def _image() -> bytes:
    return parameters.set_parameters(BLANK_IMAGE, {"meter_const": METER_CONST})


def _threshold_scenario(steps: int, cycle: int = 5) -> tuple[str, int]:
    lines = [
        "at 0 energy clear",
        f"at 2 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
        f"at 4 uart 0 {_write_645('A8F00055', THRESHOLD_30).hex()}",
    ]
    for _ in range(steps):
        lines.append(f"at {cycle} meter const {METER_CONST} cal 128")
        lines.append(f"at {cycle} pulse {PULSES_PER_STEP}")
        cycle += 1
    lines.append(f"at {cycle} uart 0 {_get(1, ENERGY_OAD).hex()}")
    return "\n".join(lines) + "\n", cycle + 2


@case(
    "ACC-STO-007",
    "with EC_SaveDiff at 0.30 kWh, 29 uncommitted pulses are lost on a power "
    "cut while the 30th commits; the restart restores the committed total once "
    "and does not count it twice",
    ("storage", "energy", "power", "snapshot"),
)
def snapshot_loss_window(ctx):
    image = _image()
    scenario, cycles = _threshold_scenario(29)
    early = ctx.run(cycles=cycles, scenario=scenario, seed_eeprom=image)
    assert early.exit_code == 0, f"firmware_sim exited {early.exit_code}"
    assert _get_value(early) == 2900, _get_value(early)
    committed = storage.latest_energy_snapshot(early.eeprom)
    assert committed is not None, "29 steps must leave the clear commit"
    assert committed.record.forward[0].integer == 0, committed.record.forward[0]

    restart = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=early.eeprom,
    )
    assert restart.exit_code == 0, f"restart exited {restart.exit_code}"
    assert _get_value(restart) == 0, "the 29 uncommitted steps must be lost"

    scenario, cycles = _threshold_scenario(30)
    saved = ctx.run(cycles=cycles, scenario=scenario, seed_eeprom=image)
    assert saved.exit_code == 0, f"firmware_sim exited {saved.exit_code}"
    assert _get_value(saved) == 3000, _get_value(saved)
    snapshot = storage.latest_energy_snapshot(saved.eeprom)
    assert snapshot is not None, "the 30th step must commit"
    assert snapshot.record.forward[0].integer == 30, snapshot.record.forward[0]

    counted = ctx.run(
        cycles=6,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
            f"at 3 meter const {METER_CONST} cal 128\n"
            f"at 3 pulse {PULSES_PER_STEP}\n"
            f"at 5 uart 0 {_get(2, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=saved.eeprom,
    )
    assert counted.exit_code == 0, f"counted exited {counted.exit_code}"
    values = [
        protocol.parse_get_response(protocol.parse_frame(frame).apdu)
        .values()[0]
        .value
        for frame in protocol.iter_698_frames(counted.uart_tx)
        if protocol.parse_frame(frame).apdu[0] == 0x85
    ]
    assert values == [3000, 3100], values


def _power_off_run(ctx, seed: bytes, steps: int) -> object:
    scenario = "\n".join(
        [
            "at 1 meter const %d cal 128" % METER_CONST,
            *[f"at 2 pulse {PULSES_PER_STEP}" for _ in range(steps)],
            "at 6 power off",
        ]
    ) + "\n"
    run = ctx.run(cycles=8, scenario=scenario, seed_eeprom=seed)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    return run


@case(
    "ACC-STO-008",
    "a torn newest snapshot slot falls back to the previous committed "
    "generation, observed through the 698 total after the restart",
    ("storage", "energy", "power", "snapshot", "torn"),
)
def snapshot_torn_slot_fallback(ctx):
    seed = ctx.run(cycles=4)
    assert seed.exit_code == 0, f"seed exited {seed.exit_code}"
    first = _power_off_run(ctx, seed.eeprom, 1)
    snapshot = storage.latest_energy_snapshot(first.eeprom)
    assert snapshot is not None, "the first power-off must commit"
    assert snapshot.record.forward[0].integer == 1, snapshot.record.forward[0]

    second = _power_off_run(ctx, first.eeprom, 1)
    latest = storage.latest_energy_snapshot(second.eeprom)
    assert latest is not None, "the second power-off must commit"
    assert latest.record.forward[0].integer == 2, latest.record.forward[0]
    assert latest.generation > snapshot.generation, (
        latest.generation,
        snapshot.generation,
    )

    torn = bytearray(second.eeprom)
    offset = (
        storage.ENERGY_SNAPSHOT_OFFSET
        + latest.slot * storage.ENERGY_SNAPSHOT_RECORD_SIZE
        + storage.ENERGY_SNAPSHOT_HEADER_SIZE
    )
    torn[offset] ^= 0x01

    restart = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=bytes(torn),
    )
    assert restart.exit_code == 0, f"restart exited {restart.exit_code}"
    assert _get_value(restart) == 100, "the previous commit must be recovered"
    recovered = storage.latest_energy_snapshot(restart.eeprom)
    assert recovered is not None and recovered.generation == snapshot.generation


def _sealed_record(steps: int) -> bytes:
    record = bytearray(storage.ENERGY_RECORD_SIZE)
    record[0:4] = steps.to_bytes(4, "little")
    record[156:160] = storage.checksum16(record[0:156]).to_bytes(4, "little")
    record[168:172] = storage.checksum16(record[160:168]).to_bytes(4, "little")
    return bytes(record)


@case(
    "ACC-STO-009",
    "with both snapshot slots torn, the meter recovers the legacy mirror as "
    "generation 1 instead of leaving persistence refused forever",
    ("storage", "energy", "power", "snapshot", "torn"),
)
def snapshot_torn_region_recovers_from_the_legacy_mirror(ctx):
    seed = ctx.run(cycles=4)
    assert seed.exit_code == 0, f"seed exited {seed.exit_code}"
    committed = _power_off_run(ctx, seed.eeprom, 1)
    assert storage.latest_energy_snapshot(committed.eeprom) is not None

    stale = bytearray(committed.eeprom)
    stale[
        storage.ENERGY_RECORD_OFFSET:
        storage.ENERGY_RECORD_OFFSET + storage.ENERGY_RECORD_SIZE
    ] = _sealed_record(7)
    legacy = bytes(stale[
        storage.ENERGY_RECORD_OFFSET:
        storage.ENERGY_RECORD_OFFSET + storage.ENERGY_RECORD_SIZE
    ])
    for slot in range(storage.ENERGY_SNAPSHOT_SLOTS):
        offset = (
            storage.ENERGY_SNAPSHOT_OFFSET
            + slot * storage.ENERGY_SNAPSHOT_RECORD_SIZE
            + storage.ENERGY_SNAPSHOT_HEADER_SIZE
        )
        stale[offset] ^= 0xFF

    run = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=bytes(stale),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _get_value(run) == 700, "the damaged region must recover the mirror"
    assert run.eeprom[
        storage.ENERGY_RECORD_OFFSET:
        storage.ENERGY_RECORD_OFFSET + storage.ENERGY_RECORD_SIZE
    ] == legacy, "the legacy mirror stays the recovery source, not a new base"
    recovered = storage.latest_energy_snapshot(run.eeprom)
    assert recovered is not None and recovered.generation == 1, recovered
    assert recovered.record.forward[0].integer == 7, recovered.record.forward[0]


def _set_poweroff_flag(image: bytearray, flag: int) -> None:
    image[
        storage.ENERGY_POWEROFF_FLAG_OFFSET:
        storage.ENERGY_POWEROFF_FLAG_OFFSET + 2
    ] = flag.to_bytes(2, "little")


def _write_ec2(image: bytearray, record: bytes) -> None:
    """Write the two parameter_item_1 EC2 items with their item checksums."""
    energy = record[:storage.ENERGY_EC2_ENERGY_SIZE]
    month = record[
        storage.ENERGY_EC2_ENERGY_SIZE:
        storage.ENERGY_EC2_ENERGY_SIZE + storage.ENERGY_EC2_MONTH_SIZE
    ]
    image[
        storage.ENERGY_EC2_ENERGY_OFFSET:
        storage.ENERGY_EC2_ENERGY_OFFSET + len(energy)
    ] = energy
    offset = storage.ENERGY_EC2_ENERGY_OFFSET + len(energy)
    image[offset:offset + 2] = storage.checksum16(energy).to_bytes(2, "big")
    image[
        storage.ENERGY_EC2_MONTH_OFFSET:
        storage.ENERGY_EC2_MONTH_OFFSET + len(month)
    ] = month
    offset = storage.ENERGY_EC2_MONTH_OFFSET + len(month)
    image[offset:offset + 2] = storage.checksum16(month).to_bytes(2, "big")


def _all_values(run) -> list[int]:
    return [
        protocol.parse_get_response(protocol.parse_frame(frame).apdu)
        .values()[0]
        .value
        for frame in protocol.iter_698_frames(run.uart_tx)
        if protocol.parse_frame(frame).apdu[0] == 0x85
    ]


@case(
    "ACC-STO-010",
    "on first boot with an empty snapshot and the 0x5AA5 power-off flag "
    "clear, the EC2 backup is adopted instead of the older power-off record",
    ("storage", "energy", "adoption"),
)
def snapshot_adopts_the_newest_legacy_source(ctx):
    image = bytearray(_image())
    poweroff = _sealed_record(1)
    image[
        storage.ENERGY_RECORD_OFFSET:
        storage.ENERGY_RECORD_OFFSET + len(poweroff)
    ] = poweroff
    _write_ec2(image, _sealed_record(5))
    _set_poweroff_flag(image, 0)

    run = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=bytes(image),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _get_value(run) == 500, (
        "the EC2 backup is newer than the 0x450D record once the power-off "
        "flag was consumed"
    )


@case(
    "ACC-STO-011",
    "the billing-day monthly clear clears the book: the next pulse starts "
    "the month at zero and a reboot restores zero",
    ("storage", "energy", "month", "clear"),
)
def monthly_clear_survives_the_next_second_and_reboot(ctx):
    month_oad = 0x20310200
    image = bytearray(_image())
    # Billing day 1 at 00:00 with an old last-bill time, so the month
    # boundary is due; the parameter item checksums ride along.
    image = bytearray(
        parameters.set_parameters(
            bytes(image), {"balance_time": bytes((1, 0, 0x63, 0x63, 0x63, 0x63))}
        )
    )
    image[5965:5971] = bytes((0, 0, 0, 1, 1, 20))  # 2020-01-01 00:00:00
    image[5971:5973] = storage.checksum16(bytes(image[5965:5971])).to_bytes(2, "big")

    lines = [
        f"at 1 meter const {METER_CONST} cal 128",
        "at 1 time '2021-04-30 23:50:00'",
    ]
    lines.extend(f"at {cycle} pulse {PULSES_PER_STEP}" for cycle in range(3, 33))
    lines.append(f"at 35 uart 0 {_get(1, month_oad).hex()}")
    lines.append(f"at 605 pulse {PULSES_PER_STEP}")
    lines.append(f"at 610 uart 0 {_get(2, month_oad).hex()}")

    run = ctx.run(cycles=615, scenario="\n".join(lines) + "\n",
                  seed_eeprom=bytes(image))
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    values = _all_values(run)
    assert values == [30, 1], values

    restart = ctx.run(
        cycles=6,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 3 uart 0 {_get(1, month_oad).hex()}\n"
        ),
        seed_eeprom=run.eeprom,
    )
    assert restart.exit_code == 0, f"restart exited {restart.exit_code}"
    assert _get_value(restart) == 0, "the clear must be the restored month"


def _frame_645(control: int, data: bytes) -> bytes:
    body = (
        bytes([0x68])
        + BLANK_ADDRESS
        + bytes([0x68, control, len(data)])
        + bytes((byte + protocol.DATA_OFFSET) & 0xFF for byte in data)
    )
    return body + bytes([protocol.checksum(body), 0x16])


def _action(piid: int, oi: int, method: int, param: bytes) -> bytes:
    return protocol.build_frame(
        protocol.action_request(piid, oi, method, mode=0, param=param),
        sa=BLANK_ADDRESS,
        ca=0x10,
        ctrl=0x43,
    )


@case(
    "ACC-STO-012",
    "a torn EC2 month item does not discard the valid EC2 energy item: the "
    "meter adopts the 5.00 kWh energy half, not the older EC1 record",
    ("storage", "energy", "adoption", "torn"),
)
def snapshot_keeps_the_valid_ec2_energy_half(ctx):
    image = bytearray(_image())
    poweroff = _sealed_record(1)
    image[
        storage.ENERGY_RECORD_OFFSET:
        storage.ENERGY_RECORD_OFFSET + len(poweroff)
    ] = poweroff
    _write_ec2(image, _sealed_record(5))
    image[storage.ENERGY_EC2_MONTH_OFFSET] ^= 0xFF  # month item checksum breaks
    _set_poweroff_flag(image, 0)

    run = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=bytes(image),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _get_value(run) == 500, (
        "the valid EC2 energy half must survive the torn month half"
    )


def _committed_energy_lines() -> tuple[list[str], int]:
    lines = [
        f"at 2 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
        f"at 3 uart 0 {_write_645('A8F00055', THRESHOLD_30).hex()}",
    ]
    cycle = 5
    for _ in range(30):
        lines.append(f"at {cycle} meter const {METER_CONST} cal 128")
        lines.append(f"at {cycle} pulse {PULSES_PER_STEP}")
        cycle += 1
    return lines, cycle


@case(
    "ACC-STO-013",
    "a 645 meter clear whose snapshot commit fails answers error byte 01 "
    "instead of success, and the committed total survives a restart",
    ("storage", "energy", "clear", "645", "fault"),
)
def clear_commit_failure_is_reported_on_645(ctx):
    lines, cycle = _committed_energy_lines()
    lines.append(f"at {cycle} uart 0 {_get(1, ENERGY_OAD).hex()}")
    lines.append(f"at {cycle + 1} eeprom on")
    clear = _frame_645(0x1A, bytes((0x02, 0xFF, 0xFF, 0xFF)) + b"\x00" * 4)
    lines.append(f"at {cycle + 2} uart 0 {clear.hex()}")

    run = ctx.run(cycles=cycle + 5, scenario="\n".join(lines) + "\n",
                  seed_eeprom=_image())
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _get_value(run) == 3000, _get_value(run)
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == 3, f"expected three 645 responses: {run.uart_tx.hex()}"
    address, control, raw = protocol.parse_frame_645(frames[-1])
    assert address == "ffffffffffff", address
    assert control == 0xDA, hex(control)
    assert bytes((byte - protocol.DATA_OFFSET) & 0xFF for byte in raw) == b"\x01"

    restart = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=run.eeprom,
    )
    assert restart.exit_code == 0, f"restart exited {restart.exit_code}"
    assert _get_value(restart) == 3000, "the failed clear must not persist"


@case(
    "ACC-STO-015",
    "a 645 forward-energy base set (DI A8FFFFF0) whose snapshot commit fails "
    "answers error byte 01 instead of success, and the committed total "
    "survives a restart",
    ("storage", "energy", "base", "645", "fault"),
)
def energy_base_set_commit_failure_is_reported_on_645(ctx):
    lines, cycle = _committed_energy_lines()
    lines.append(f"at {cycle} uart 0 {_get(1, ENERGY_OAD).hex()}")
    lines.append(f"at {cycle + 1} eeprom on")
    base = (
        protocol.data_id_bytes("A8FFFFF0")
        + b"\x02\x00\x00\x00"
        + b"\x00" * 4
        + (45).to_bytes(4, "little")
    )
    lines.append(f"at {cycle + 2} uart 0 {_frame_645(0x14, base).hex()}")

    run = ctx.run(cycles=cycle + 5, scenario="\n".join(lines) + "\n",
                  seed_eeprom=_image())
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _get_value(run) == 3000, _get_value(run)
    frames = protocol.iter_645_frames(run.uart_tx)
    assert len(frames) == 3, f"expected three 645 responses: {run.uart_tx.hex()}"
    address, control, raw = protocol.parse_frame_645(frames[-1])
    assert address == "ffffffffffff", address
    assert control == 0xD4, hex(control)
    assert bytes((byte - protocol.DATA_OFFSET) & 0xFF for byte in raw) == b"\x01"

    restart = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=run.eeprom,
    )
    assert restart.exit_code == 0, f"restart exited {restart.exit_code}"
    assert _get_value(restart) == 3000, "the failed base set must not persist"


@case(
    "ACC-STO-014",
    "a 698 ACTION 0x4300 method 3 clear whose snapshot commit fails answers "
    "DAR 255 instead of success, and the committed total survives a restart",
    ("storage", "energy", "clear", "698", "fault"),
)
def clear_commit_failure_is_reported_on_698(ctx):
    lines, cycle = _committed_energy_lines()
    lines.append(f"at {cycle} uart 0 {_get(1, ENERGY_OAD).hex()}")
    lines.append(f"at {cycle + 1} eeprom on")
    action = _action(2, 0x4300, 3, b"\x00")
    lines.append(f"at {cycle + 2} uart 0 {action.hex()}")

    run = ctx.run(cycles=cycle + 5, scenario="\n".join(lines) + "\n",
                  seed_eeprom=_image())
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert _get_value(run) == 3000, _get_value(run)
    apdus = [
        protocol.parse_frame(frame).apdu
        for frame in protocol.iter_698_frames(run.uart_tx)
    ]
    actions = [
        protocol.parse_action_response(apdu)
        for apdu in apdus
        if apdu[0] == protocol.SVC_ACTION_RES
    ]
    assert len(actions) == 1, [apdu.hex() for apdu in apdus]
    assert actions[0].dar == 255, actions[0].dar

    restart = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=run.eeprom,
    )
    assert restart.exit_code == 0, f"restart exited {restart.exit_code}"
    assert _get_value(restart) == 3000, "the failed clear must not persist"


@case(
    "ACC-STO-016",
    "a meter clear whose base restore keeps failing must not overwrite the "
    "legacy energy record with the zero RAM book: the record bytes stay and a "
    "restart that re-adopts the legacy mirror reads 700",
    ("storage", "energy", "clear", "restore", "fault"),
)
def clear_while_restore_pending_keeps_the_legacy_record(ctx):
    image = bytearray(_image())
    record = _sealed_record(7)
    image[
        storage.ENERGY_RECORD_OFFSET:
        storage.ENERGY_RECORD_OFFSET + len(record)
    ] = record
    _set_poweroff_flag(image, storage.ENERGY_POWEROFF_FLAG)
    # A CRC-valid base whose legacy record checksums do not validate: the
    # restore refuses the base and retries forever, while EEPROM writes work.
    header = struct.pack(
        "<IHHI",
        storage.ENERGY_SNAPSHOT_MAGIC,
        storage.ENERGY_SNAPSHOT_VERSION,
        storage.ENERGY_RECORD_SIZE,
        1,
    )
    blob = bytes(storage.ENERGY_RECORD_SIZE)
    crc = zlib.crc32(header + blob) & 0xFFFFFFFF
    slot = header + blob + struct.pack("<I", crc)
    image[
        storage.ENERGY_SNAPSHOT_OFFSET:
        storage.ENERGY_SNAPSHOT_OFFSET + len(slot)
    ] = slot

    clear = _frame_645(0x1A, bytes((0x02, 0xFF, 0xFF, 0xFF)) + b"\x00" * 4)
    run = ctx.run(
        cycles=5,
        scenario=f"at 2 uart 0 {clear.hex()}\n",
        seed_eeprom=bytes(image),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    assert run.eeprom[
        storage.ENERGY_RECORD_OFFSET:
        storage.ENERGY_RECORD_OFFSET + len(record)
    ] == record, (
        "a failed clear must not replace the legacy record with the zero book"
    )

    # The recovery path: the damaged snapshot region is erased and the next
    # boot adopts the legacy mirror.
    recovered = bytearray(run.eeprom)
    recovered[
        storage.ENERGY_SNAPSHOT_OFFSET:storage.ENERGY_SNAPSHOT_REGION_END
    ] = b"\xff" * (
        storage.ENERGY_SNAPSHOT_REGION_END - storage.ENERGY_SNAPSHOT_OFFSET
    )
    restart = ctx.run(
        cycles=4,
        scenario=(
            f"at 1 meter const {METER_CONST} cal 128\n"
            f"at 2 uart 0 {_get(1, ENERGY_OAD).hex()}\n"
        ),
        seed_eeprom=bytes(recovered),
    )
    assert restart.exit_code == 0, f"restart exited {restart.exit_code}"
    assert _get_value(restart) == 700, "the restart must restore the record"
