from __future__ import annotations

import struct
import zlib

from cases import case


REGISTRY_OFFSET = 55296
EEPROM_SIZE = 64 * 1024
FLASH_SIZE = 4 * 1024 * 1024
# The frozen pre-v4 LOAD_PROFILE_1 base; the v4 layout moved LP1 to 8192.
LEGACY_LP1_BASE = 4096
LEGACY_META_MAGIC = 0x49445031


def legacy_profile_meta() -> bytes:
    meta = bytearray(32)
    meta[0:4] = LEGACY_META_MAGIC.to_bytes(4, "big")
    meta[4] = 3
    meta[5:7] = (64).to_bytes(2, "big")
    meta[7:9] = (3).to_bytes(2, "big")
    meta[15:17] = (1).to_bytes(2, "big")
    crc = 0xFFFF
    for byte in meta[:30]:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (
                crc << 1) & 0xFFFF
    meta[30:32] = crc.to_bytes(2, "big")
    return bytes(meta)


def registry_blob(entries) -> bytes:
    body = bytearray(b"NDRG" + bytes([1, 8]))
    body += len(entries).to_bytes(2, "big")
    for class_id, obis in entries:
        body += bytes([class_id, 0]) + obis
    body += (sum(body) & 0xFFFF).to_bytes(2, "big")
    return bytes(body)


def require(run, key: str, expected: str | None = None) -> str:
    actual = run.values.get(key)
    assert actual is not None, (
        f"missing {key} in runner output: {run.stdout[-300:]!r}")
    if expected is not None:
        assert actual == expected, f"{key}={actual}, expected {expected}"
    return actual


@case(
    "idis.billing-default",
    "the default monthly billing single action fires at the month boundary",
    ("idis", "billing", "persistence"),
)
def billing_default(ctx):
    run = ctx.run(seconds=10, start_time="2026-02-28T23:59:55")
    assert run.exit_code == 0, f"runner exited {run.exit_code}: {run.stderr[-200:]}"
    require(run, "profiles", "22")
    # The default action fires at 2026-03-01 00:00:00; the host's end-of-run
    # billing capture then adds the second record.
    require(run, "billing", "2")
    require(run, "ec2", "1")


@case(
    "idis.default-calendar",
    "the default daily tariff calendar selects script 1",
    ("idis", "tariff"),
)
def default_calendar(ctx):
    run = ctx.run(seconds=5, start_time="2026-01-01T00:00:00")
    assert run.exit_code == 0, f"runner exited {run.exit_code}: {run.stderr[-200:]}"
    require(run, "profiles", "22")
    require(run, "tariff2", "0")
    tariff1 = int(require(run, "tariff1"))
    assert tariff1 > 0, f"tariff1={tariff1}"


@case(
    "idis.power-loss",
    "PQ records and the event log survive an abrupt exit without the final save",
    ("idis", "power-loss", "persistence"),
)
def power_loss(ctx):
    first = ctx.run(seconds=130, no_final_save=True)
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "profiles", "22")
    require(first, "pq", "2")
    require(first, "event_log", "1")

    second = ctx.run(seconds=5)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "profiles", "22")
    require(second, "pq", "2")
    require(second, "event_log", "1")


@case(
    "idis.restart-persistence",
    "energy, demand, alarm and billing survive a restart",
    ("idis", "persistence"),
)
def restart_persistence(ctx):
    first = ctx.run(seconds=60)
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "profiles", "22")
    require(first, "alarm", "0x00000002")

    second = ctx.run(seconds=5)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "profiles", "22")
    require(second, "alarm", "0x00000002")
    require(second, "max_demand", "5000,1000,2000,2000")
    require(second, "rate2", "5000")
    require(second, "billing", "2")

    energy_first = int(require(first, "energy_raw"))
    energy_second = int(require(second, "energy_raw"))
    assert energy_second >= energy_first, (
        f"energy regressed: {energy_first} -> {energy_second}")


@case(
    "idis.energy-journal-power-cut",
    "pulses accepted since the last base commit survive an abrupt exit",
    ("idis", "energy", "power-loss", "persistence"),
)
def energy_journal_power_cut(ctx):
    # The host injects one pulse per second after a 4000-pulse seed; raw units
    # are pulses * 10000 / 2000 = pulses * 5.
    first = ctx.run(seconds=130, no_final_save=True)
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "energy_raw", "20650")

    second = ctx.run(seconds=5)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    # 4000 seed + 130 journal pulses + 5 new pulses; no loss, no double count.
    require(second, "energy_raw", "20675")


@case(
    "idis.tariff-switch-energy",
    "pulses before the scripted tariff switch stay on tariff 1 and later pulses land on tariff 2",
    ("idis", "tariff", "energy"),
)
def tariff_switch_energy(ctx):
    # The host drives one 5-unit active-import pulse per second and executes
    # the tariff script at second 30, so 31 pulses land on tariff 1 and 29 on
    # tariff 2.
    run = ctx.run(seconds=60, start_time="2026-01-01T00:00:00")
    assert run.exit_code == 0, f"runner exited {run.exit_code}: {run.stderr[-200:]}"
    require(run, "profiles", "22")
    require(run, "energy_raw", "20300")
    require(run, "tariff1", "20155")
    require(run, "tariff2", "145")


@case(
    "idis.midnight-capture",
    "crossing midnight writes the load profiles and runs the calendar script once",
    ("idis", "tariff", "persistence"),
)
def midnight_capture(ctx):
    run = ctx.run(seconds=10, start_time="2026-01-01T23:59:55")
    assert run.exit_code == 0, f"runner exited {run.exit_code}: {run.stderr[-200:]}"
    require(run, "profiles", "22")
    require(run, "lp1", "1")
    require(run, "lp2", "1")
    require(run, "scripts", "1")


@case(
    "idis.restart-no-duplicate-capture",
    "a restart after midnight neither repeats the capture nor replays the calendar script",
    ("idis", "persistence"),
)
def restart_no_duplicate_capture(ctx):
    first = ctx.run(seconds=10, start_time="2026-01-01T23:59:55")
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "lp1", "1")
    require(first, "lp2", "1")

    second = ctx.run(seconds=5)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "profiles", "22")
    require(second, "lp1", "1")
    require(second, "lp2", "1")
    require(second, "scripts", "0")


@case(
    "idis.registry-load",
    "the object list comes from the external registry blob in EEPROM",
    ("idis", "registry", "persistence"),
)
def registry_load(ctx):
    blob = registry_blob([
        (8, bytes([0, 0, 1, 0, 0, 255])),
        (3, bytes([1, 0, 1, 8, 0, 255])),
    ])
    # The rest of the image is erased EEPROM (0xFF), not zero-filled: the
    # pulse base slots must read as blank, not as torn.
    image = bytearray(b"\xFF" * EEPROM_SIZE)
    image[REGISTRY_OFFSET:REGISTRY_OFFSET + len(blob)] = blob
    (ctx.case_dir / "eeprom.bin").write_bytes(image)

    first = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "profiles", "22")
    require(first, "registry", "2")

    second = ctx.run(seconds=2)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "registry", "2")


@case(
    "idis.power-failure",
    "the power-failure record captures one 5-second outage and survives a restart",
    ("idis", "pq", "persistence"),
)
def power_failure(ctx):
    first = ctx.run(seconds=60, start_time="2026-01-01T00:00:00")
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "profiles", "22")
    require(first, "power", "1/5")
    require(first, "event_log", "1")
    require(first, "event", "7")
    require(first, "counter", "1")
    require(first, "alarm", "0x00000002")

    second = ctx.run(seconds=2)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "power", "1/5")


@case(
    "idis.registry-default",
    "without an external blob the object list falls back to the five built-in objects",
    ("idis", "registry"),
)
def registry_default(ctx):
    run = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert run.exit_code == 0, f"runner exited {run.exit_code}: {run.stderr[-200:]}"
    require(run, "profiles", "22")
    require(run, "registry", "5")

    second = ctx.run(seconds=2)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "registry", "5")


@case(
    "idis.script-override-persist",
    "the installer's disconnect script override is stored once and survives a restart",
    ("idis", "scripts", "persistence"),
)
def script_override_persist(ctx):
    first = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "script", "1")

    second = ctx.run(seconds=2)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "script", "1")


@case(
    "idis.legacy-profile-hold",
    "a pre-v4 profile image at the old base is retained, every profile holds, and the hold is reported",
    ("idis", "profile", "boot", "persistence"),
)
def legacy_profile_hold(ctx):
    meta = legacy_profile_meta()
    image = bytearray(b"\xFF" * FLASH_SIZE)
    image[LEGACY_LP1_BASE:LEGACY_LP1_BASE + len(meta)] = meta
    (ctx.case_dir / "flash.bin").write_bytes(image)

    first = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "profiles", "22")
    # Event 12 is the boot report for the hold; the event log profile itself
    # is held, so its entry count stays zero while the retained image is kept.
    require(first, "event", "12")
    require(first, "billing", "0")
    require(first, "event_log", "0")
    kept = (ctx.case_dir / "flash.bin").read_bytes()
    assert kept[LEGACY_LP1_BASE:LEGACY_LP1_BASE + len(meta)] == meta, (
        "boot or capture overwrote the retained pre-v4 LOAD_PROFILE_1 image")

    second = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "profiles", "22")
    require(second, "event", "12")
    kept = (ctx.case_dir / "flash.bin").read_bytes()
    assert kept[LEGACY_LP1_BASE:LEGACY_LP1_BASE + len(meta)] == meta, (
        "restart discarded the retained pre-v4 image")


@case(
    "idis.legacy-release",
    "a factory-confirmed maintenance action releases the pre-v4 hold and profile appends resume",
    ("idis", "profile", "boot", "persistence"),
)
def legacy_release(ctx):
    # The local factory entry, not a COSEM method: class-7 reset() has no
    # parameters, so the front end verifies the RELG confirmation
    # (0x52454C47) and calls nd_cosem_port_release_legacy as management SAP 1.
    # No reflash, no different build.
    confirmation = "0x52454C47"
    meta = legacy_profile_meta()
    image = bytearray(b"\xFF" * FLASH_SIZE)
    image[LEGACY_LP1_BASE:LEGACY_LP1_BASE + len(meta)] = meta
    (ctx.case_dir / "flash.bin").write_bytes(image)

    first = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "profiles", "22")
    require(first, "event", "12")
    require(first, "legacy_held", "1")
    require(first, "billing", "0")
    require(first, "event_log", "0")

    second = ctx.run(seconds=6, start_time="2026-01-01T00:00:00",
                     extra_args=("--release-legacy", confirmation))
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "maintenance", "0")
    require(second, "legacy_held", "0")
    require(second, "profiles", "22")
    # The release resets every ring; the next event and the end-of-run billing
    # capture must land as records instead of being refused.
    assert int(require(second, "billing")) >= 1, "billing appends did not resume"
    assert int(require(second, "event_log")) >= 1, "event log appends did not resume"

    third = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert third.exit_code == 0, f"runner exited {third.exit_code}: {third.stderr[-200:]}"
    require(third, "legacy_held", "0")
    require(third, "event_log", require(second, "event_log"))
    assert int(require(third, "billing")) >= int(require(second, "billing"))


@case(
    "idis.legacy-uart-release",
    "a framed maintenance command on the local optical port releases the pre-v4 hold and profile appends resume",
    ("idis", "profile", "boot", "persistence"),
)
def legacy_uart_release(ctx):
    # 0x68, length 5, command 1, confirmation 0x52454C47 ("RELG"), XOR.
    frame = "68050152454C4718"
    meta = legacy_profile_meta()
    image = bytearray(b"\xFF" * FLASH_SIZE)
    image[LEGACY_LP1_BASE:LEGACY_LP1_BASE + len(meta)] = meta
    (ctx.case_dir / "flash.bin").write_bytes(image)

    first = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "legacy_held", "1")
    require(first, "billing", "0")
    require(first, "event_log", "0")

    bad = ctx.run(seconds=2, start_time="2026-01-01T00:00:00",
                  extra_args=("--uart-input", "68050152454C4719"))
    assert bad.exit_code == 0, f"runner exited {bad.exit_code}: {bad.stderr[-200:]}"
    require(bad, "maintenance", "-1")
    require(bad, "legacy_held", "1")
    require(bad, "billing", "0")

    second = ctx.run(seconds=6, start_time="2026-01-01T00:00:00",
                     extra_args=("--uart-input", frame))
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "maintenance", "0")
    require(second, "legacy_held", "0")
    require(second, "profiles", "22")
    assert int(require(second, "billing")) >= 1, "billing appends did not resume"
    assert int(require(second, "event_log")) >= 1, "event log appends did not resume"

    third = ctx.run(seconds=2, start_time="2026-01-01T00:00:00")
    assert third.exit_code == 0, f"runner exited {third.exit_code}: {third.stderr[-200:]}"
    require(third, "legacy_held", "0")
    require(third, "event_log", require(second, "event_log"))


@case(
    "idis.boot-rejects-bad-energy-base",
    "a committed energy base for a different book size stops the boot instead of running a zero book",
    ("idis", "energy", "boot", "persistence"),
)
def boot_rejects_bad_energy_base(ctx):
    # Host EEPROM layout: two alternating 1 KiB base slots (host main enum).
    # A base whose header says 4 blob bytes is what a firmware with a different
    # book size, or a torn migration, leaves behind; base() accepts it and the
    # length check is the only thing between that record and a zero book.
    base_offsets = (0xE800, 0xEC00)
    first = ctx.run(seconds=5)
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    image = first.run_dir / "eeprom.bin"
    data = bytearray(image.read_bytes())
    rewritten = 0
    for offset in base_offsets:
        magic, version, _bytes, generation = struct.unpack_from(
            "<IHHI", data, offset)
        if magic != 0x4E445042 or version != 1:
            continue
        blob_bytes = 4
        header = struct.pack("<IHHI", magic, version, blob_bytes, generation)
        blob = data[offset + 12:offset + 12 + blob_bytes]
        crc = zlib.crc32(header + blob) & 0xFFFFFFFF
        data[offset:offset + 12] = header
        data[offset + 12 + blob_bytes:offset + 12 + blob_bytes + 4] = (
            struct.pack("<I", crc))
        rewritten += 1
    assert rewritten > 0, "run left no committed energy base to corrupt"
    image.write_bytes(data)

    second = ctx.run(seconds=5)
    assert second.exit_code != 0, (
        "boot continued on a base for a different book size; the zero book "
        f"would drop the billing register: {second.stdout[-200:]!r}")
    assert "energy" in second.stderr, (
        f"boot failure did not name the energy book: {second.stderr[-200:]!r}")
