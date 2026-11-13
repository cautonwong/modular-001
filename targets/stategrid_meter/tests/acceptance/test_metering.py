from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6


def _read_after_meter(ctx, oad: int) -> protocol.AxdrValue:
    request = protocol.build_frame(
        protocol.get_request(1, oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=4,
        scenario=(
            "at 1 meter voltage 220 current 5 power 1000\n"
            f"at 2 uart 0 {request.hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert frames, f"no response for OAD {oad:08x}: {run.uart_tx.hex()}"
    return protocol.parse_get_response(protocol.parse_frame(frames[0]).apdu).values()[0]


@case(
    "ACC-MET-001",
    "32 injected pulses at a 3200 imp/kWh constant add 0.01 kWh to the total",
    ("metering", "energy"),
)
def forward_active_energy(ctx):
    request = protocol.build_frame(
        protocol.get_request(1, 0x00000401), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=6,
        scenario=(
            "at 0 energy clear\n"
            "at 1 meter const 3200 cal 128\n"
            "at 1 pulse 32\n"
            f"at 3 uart 0 {request.hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert frames, f"no energy response: {run.uart_tx.hex()}"
    value = protocol.parse_get_response(protocol.parse_frame(frames[0]).apdu).values()[0]
    assert value.tag == protocol.AXDR_LONG64, value.tag
    assert value.value == 100, value.value


@case(
    "ACC-MET-002",
    "reverse pulses add only to the reverse energy register",
    ("metering", "energy"),
)
def reverse_active_energy(ctx):
    reverse_oad = 0x00200401
    forward_oad = 0x00100401
    reverse_request = protocol.build_frame(
        protocol.get_request(1, reverse_oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    forward_request = protocol.build_frame(
        protocol.get_request(2, forward_oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=6,
        scenario=(
            "at 0 energy clear\n"
            "at 1 meter const 3200 cal 32 direction reverse\n"
            "at 1 pulse 32\n"
            f"at 3 uart 0 {reverse_request.hex()}\n"
            f"at 4 uart 0 {forward_request.hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert len(frames) == 2, run.uart_tx.hex()
    reverse = protocol.parse_get_response(protocol.parse_frame(frames[0]).apdu)
    forward = protocol.parse_get_response(protocol.parse_frame(frames[1]).apdu)
    assert reverse.values()[0].value == 100, reverse.values()[0].value
    assert forward.values()[0].value == 0, forward.values()[0].value


@case(
    "ACC-MET-006",
    "injected voltage, current and power are readable over 698 with their scales",
    ("metering", "instantaneous"),
)
def instantaneous_values(ctx):
    voltage = _read_after_meter(ctx, 0x20000200).children[0]
    current = _read_after_meter(ctx, 0x20010200).children[0]
    power = _read_after_meter(ctx, 0x20040200)

    assert voltage.tag == protocol.AXDR_LONG_UNSIGNED
    assert voltage.value == 2200, voltage.value
    assert current.tag == protocol.AXDR_DOUBLE_LONG
    assert current.value == 5000, current.value
    assert power.tag == protocol.AXDR_ARRAY and len(power.children) == 2
    assert power.children[0].value == 10000, power.children[0].value
    assert power.children[1].value == 10000, power.children[1].value
