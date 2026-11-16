from __future__ import annotations

from cases import case


REGISTRY_OFFSET = 55296
EEPROM_SIZE = 64 * 1024


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
    image = bytearray(EEPROM_SIZE)
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
