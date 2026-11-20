"""ACC-MET-005: interval and sliding demand on one active-power source.

The two placeholder OADs are 0x11000201 (5-minute interval) and 0x11000202
(1-minute slide x 5). Attribute 2 returns a six-element structure: current
value, current fee demand, maximum, maximum fee demand, maximum time, window
start time. Raw values are 0.1 W. The product OAD table replaces this layout.
"""

from __future__ import annotations

import sys
from pathlib import Path

from cases import case

import protocol

BLANK_SA = b"\xff" * 6
ROOT = Path(__file__).resolve().parents[4]

INTERVAL_OAD = 0x11000201
SLIDING_OAD = 0x11000202
DEMAND_SOURCE_PREFIXES = (
    "domain/metering/",
    "application/metering/",
    "targets/stategrid_meter/app/metering/",
)
LEGACY_SYMBOLS = (
    "add_demand_action",
    "scond_execute_for_demand",
    "minute_execute_for_demand",
)


def _request(piid: int, oad: int) -> str:
    return protocol.build_frame(
        protocol.get_request(piid, oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
    ).hex()


def _clock(payload: bytes) -> tuple[int, ...]:
    return (payload[0] << 8 | payload[1], payload[2], payload[3],
            payload[4], payload[5], payload[6])


def _views(run):
    frames = protocol.iter_698_frames(run.uart_tx)
    assert len(frames) == 6, run.uart_tx.hex()
    views = []
    for frame in frames:
        value = protocol.parse_get_response(
            protocol.parse_frame(frame).apdu).values()[0]
        assert value.tag == protocol.AXDR_STRUCTURE, value.tag
        assert len(value.children) == 6, len(value.children)
        views.append(value.children)
    return views


def _single_view(run):
    frames = protocol.iter_698_frames(run.uart_tx)
    assert len(frames) == 1, run.uart_tx.hex()
    value = protocol.parse_get_response(
        protocol.parse_frame(frames[0]).apdu).values()[0]
    assert value.tag == protocol.AXDR_STRUCTURE, value.tag
    assert len(value.children) == 6, len(value.children)
    return value.children


@case(
    "ACC-MET-005",
    "interval (5-minute block) and sliding (1-minute x 5) demand publish value, "
    "maximum and fee demand at their window boundaries from one power source",
    ("metering", "demand"),
)
def demand_windows(ctx):
    scenario = (
        "at 0 meter power 100\n"
        "at 299 meter power 200\n"
        f"at 310 uart 0 {_request(1, INTERVAL_OAD)}\n"
        f"at 311 uart 0 {_request(2, SLIDING_OAD)}\n"
        f"at 370 uart 0 {_request(3, INTERVAL_OAD)}\n"
        f"at 371 uart 0 {_request(4, SLIDING_OAD)}\n"
        f"at 610 uart 0 {_request(5, INTERVAL_OAD)}\n"
        f"at 611 uart 0 {_request(6, SLIDING_OAD)}\n"
    )
    run = ctx.run(cycles=620, scenario=scenario,
                  start_time="2024-08-28 16:40:00")
    assert run.exit_code == 0, run.stderr
    views = _views(run)
    first = views[0::2]
    second = views[1::2]

    interval_first, sliding_first = first[0], second[0]
    assert interval_first[0].value == 1000, interval_first[0].value
    assert interval_first[1].value == 1000, interval_first[1].value
    assert interval_first[2].value == 1000, interval_first[2].value
    assert interval_first[3].value == 1000, interval_first[3].value
    assert _clock(interval_first[4].value) == (2024, 8, 28, 16, 45, 0)
    assert sliding_first[2].value == 1000, sliding_first[2].value

    interval_step, sliding_step = first[1], second[1]
    assert interval_step[0].value == 1000, interval_step[0].value
    assert interval_step[2].value == 1000, interval_step[2].value
    assert sliding_step[0].value == 1200, sliding_step[0].value
    assert sliding_step[1].value == 1200, sliding_step[1].value
    assert sliding_step[2].value == 1200, sliding_step[2].value
    assert _clock(sliding_step[4].value) == (2024, 8, 28, 16, 46, 0)

    interval_end, sliding_end = first[2], second[2]
    assert interval_end[0].value == 2000, interval_end[0].value
    assert interval_end[1].value == 2000, interval_end[1].value
    assert interval_end[2].value == 2000, interval_end[2].value
    assert sliding_end[0].value == 2000, sliding_end[0].value
    assert sliding_end[2].value == 2000, sliding_end[2].value
    assert _clock(interval_end[4].value) == (2024, 8, 28, 16, 50, 0)
    assert _clock(sliding_end[4].value) == (2024, 8, 28, 16, 50, 0)
    assert _clock(interval_end[5].value) == (2024, 8, 28, 16, 50, 0)


@case(
    "ACC-MET-005-MANIFEST",
    "the selected firmware sources include the domain demand aggregate, exclude "
    "the legacy demand staging, and contain no malloc",
    ("metering", "demand", "manifest"),
)
def demand_sources(ctx):
    del ctx
    sys.path.insert(0, str(ROOT / "tools"))
    import product_manifest  # noqa: E402

    index = product_manifest.component_index()
    product = ROOT / "products/meter_electricity_single_phase_cn/product.yaml"
    components, errors = product_manifest.closure(product, index)
    assert not errors, errors
    sources = []
    for name in components:
        sources.extend(product_manifest.component_sources(index[name]))
    relative = {path.relative_to(ROOT).as_posix() for path in sources}

    assert "domain/metering/demand.c" in relative
    assert not any(path.endswith("legacy/demand.c") for path in relative)

    demand_sources = [
        path for path in sources
        if path.relative_to(ROOT).as_posix().startswith(DEMAND_SOURCE_PREFIXES)
    ]
    assert demand_sources
    for path in demand_sources:
        text = path.read_text(encoding="utf-8", errors="replace")
        assert "malloc(" not in text, path
        for symbol in LEGACY_SYMBOLS:
            assert symbol not in text, (path, symbol)


@case(
    "ACC-MET-005-CLEAR",
    "a total clear writes zero demand maxima to the EEPROM before the meter "
    "resets, so a reset without a power-off save boots with zero maxima",
    ("metering", "demand", "persistence"),
)
def demand_clear_before_reset(ctx):
    seed = ctx.run(
        cycles=306,
        scenario="at 0 meter power 100\nat 305 power off\n",
        start_time="2024-08-28 16:40:00",
    )
    assert seed.exit_code == 0, seed.stderr

    # The power on restores the seeded maximum; the clear then persists zeros
    # and the run ends without a power-off save.
    cleared = ctx.run(
        cycles=10,
        seed_eeprom=seed.eeprom,
        scenario=(
            "at 0 power off\n"
            "at 2 power on\n"
            f"at 4 uart 0 {_request(1, INTERVAL_OAD)}\n"
            "at 8 meter clear\n"
        ),
        start_time="2024-08-28 16:40:00",
    )
    assert cleared.exit_code == 0, cleared.stderr
    restored = _single_view(cleared)
    assert restored[2].value == 1000, restored[2].value

    rebooted = ctx.run(
        cycles=6,
        seed_eeprom=cleared.eeprom,
        scenario=(
            "at 0 power off\n"
            "at 2 power on\n"
            f"at 4 uart 0 {_request(2, INTERVAL_OAD)}\n"
        ),
        start_time="2024-08-28 16:40:00",
    )
    assert rebooted.exit_code == 0, rebooted.stderr
    current, fee, maximum, max_fee, max_time, _start = _single_view(rebooted)
    assert current.value == 0, current.value
    assert fee.value == 0, fee.value
    assert maximum.value == 0, maximum.value
    assert max_fee.value == 0, max_fee.value
    assert max_time.value == b"\x00" * 7, max_time.value
