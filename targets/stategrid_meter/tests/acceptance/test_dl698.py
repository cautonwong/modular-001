from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6


def _get(ctx, oad: int, *, cycles: int = 3, prefix: str = ""):
    request = protocol.build_frame(
        protocol.get_request(1, oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=cycles,
        scenario=prefix + f"at 1 uart 0 {request.hex()}\n",
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert frames, f"no 698 response for OAD {oad:08x}: {run.uart_tx.hex()}"
    result = protocol.parse_get_response(protocol.parse_frame(frames[0]).apdu)
    assert result.oad == oad, f"response OAD {result.oad:08x}, expected {oad:08x}"
    return result


@case(
    "ACC-698-001",
    "a 698 GET of the total active energy returns the zeroed blank-meter object",
    ("698", "metering"),
)
def get_total_energy(ctx):
    result = _get(ctx, 0x00000201)

    value = result.values()[0]
    assert value.tag == protocol.AXDR_DOUBLE_LONG, value.tag
    assert value.value == 0, value.value


@case(
    "ACC-698-002",
    "a 698 SET survives a restart and is visible through GET",
    ("698", "persistence"),
)
def set_parameter_survives_restart(ctx):
    set_apdu = protocol.set_request(9, 0x40120200, b"\x04\x08\x5a")
    set_frame = protocol.build_frame(set_apdu, sa=BLANK_SA, ca=0x10, ctrl=0x43)
    first = ctx.run(cycles=4, scenario=f"at 1 uart 0 {set_frame.hex()}\n")
    assert first.exit_code == 0, f"first run exited {first.exit_code}"

    get_frame = protocol.build_frame(
        protocol.get_request(10, 0x40120200), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    second = ctx.run(
        cycles=4,
        seed_eeprom=first.eeprom,
        scenario=f"at 1 uart 0 {get_frame.hex()}\n",
    )
    assert second.exit_code == 0, f"second run exited {second.exit_code}"
    frames = protocol.iter_698_frames(second.uart_tx)
    assert frames, f"no GET response after restart: {second.uart_tx.hex()}"
    result = protocol.parse_get_response(protocol.parse_frame(frames[0]).apdu)
    assert result.oad == 0x40120200
    assert result.data == bytes.fromhex("04085a"), result.data.hex()


@case(
    "ACC-698-009",
    "698 GET reads variable and parameter objects and SET changes a writable bit string",
    ("698", "parameter"),
)
def get_variable_and_set_parameter(ctx):
    voltage = _get(ctx, 0x20000200).values()[0].children[0]
    current = _get(ctx, 0x20010200).values()[0].children[0]
    power = _get(ctx, 0x20040200).values()[0]
    address = _get(ctx, 0x40010200).values()[0]

    assert voltage.tag == protocol.AXDR_LONG_UNSIGNED
    assert current.tag == protocol.AXDR_DOUBLE_LONG
    assert power.tag == protocol.AXDR_ARRAY and len(power.children) == 2
    assert address.tag == protocol.AXDR_OCTET_STRING and address.value == b"\xff" * 6

    set_apdu = protocol.set_request(
        5, 0x20150400, b"\x04\x20" + b"\xff\xff\xff\xff"
    )
    set_frame = protocol.build_frame(set_apdu, sa=BLANK_SA, ca=0x10, ctrl=0x43)
    get_frame = protocol.build_frame(
        protocol.get_request(6, 0x20150400), sa=BLANK_SA, ca=0x10, ctrl=0x43
    )
    run = ctx.run(
        cycles=8,
        scenario=(
            f"at 1 uart 0 {set_frame.hex()}\n"
            f"at 3 uart 0 {get_frame.hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert len(frames) == 2, f"expected SET and GET responses: {run.uart_tx.hex()}"

    piid, dar = protocol.parse_set_response(protocol.parse_frame(frames[0]).apdu)
    assert (piid, dar) == (5, 0), (piid, dar)
    readback = protocol.parse_get_response(protocol.parse_frame(frames[1]).apdu)
    assert readback.oad == 0x20150400
    assert readback.data == bytes.fromhex("0420ffffffff"), readback.data.hex()


@case(
    "ACC-698-003",
    "a 698 ACTION executes a supported method and returns DAR success",
    ("698", "control"),
)
def action_report_confirmation(ctx):
    request = protocol.build_frame(
        protocol.action_request(7, 0x2015, 0x7F, param=b"\x04\x20" + b"\x00" * 4),
        sa=BLANK_SA,
        ca=0x10,
        ctrl=0x43,
    )
    run = ctx.run(cycles=4, scenario=f"at 1 uart 0 {request.hex()}\n")
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    frames = protocol.iter_698_frames(run.uart_tx)
    assert frames, f"no ACTION response: {run.uart_tx.hex()}"

    result = protocol.parse_action_response(protocol.parse_frame(frames[0]).apdu)
    assert (result.piid, result.oi, result.method) == (7, 0x2015, 0x7F)
    assert result.dar == 0, result.dar
    assert result.ok


@case(
    "ACC-698-010",
    "698 GET answers each implemented object class with its declared data type",
    ("698", "object-model"),
)
def get_object_families(ctx):
    expected = {
        0x00000200: (protocol.AXDR_ARRAY, 1),
        0x00000201: (protocol.AXDR_DOUBLE_LONG, None),
        0x00100200: (protocol.AXDR_ARRAY, 1),
        0x20000200: (protocol.AXDR_ARRAY, 1),
        0x20010200: (protocol.AXDR_ARRAY, 1),
        0x20040200: (protocol.AXDR_ARRAY, 2),
        0x301B0A00: (protocol.AXDR_ARRAY, 1),
        0x40010200: (protocol.AXDR_OCTET_STRING, None),
        0x60000200: (protocol.AXDR_ARRAY, 1),
        0x80000200: (protocol.AXDR_STRUCTURE, 2),
        0xE0000200: (protocol.AXDR_ARRAY, 1),
    }
    lines = []
    for piid, oad in enumerate(expected, 1):
        request = protocol.build_frame(
            protocol.get_request(piid, oad), sa=BLANK_SA, ca=0x10, ctrl=0x43
        )
        lines.append(f"at {piid} uart 0 {request.hex()}")
    run = ctx.run(cycles=len(expected) + 2, scenario="\n".join(lines) + "\n")
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    frames = protocol.iter_698_frames(run.uart_tx)
    assert len(frames) == len(expected), (
        f"expected {len(expected)} responses, got {len(frames)}: {run.uart_tx.hex()}"
    )
    for frame, (oad, (tag, size)) in zip(frames, expected.items()):
        result = protocol.parse_get_response(protocol.parse_frame(frame).apdu)
        assert result.oad == oad
        value = result.values()[0]
        assert value.tag == tag, f"OAD {oad:08x} tag {value.tag:#x}, expected {tag:#x}"
        if size is not None:
            assert len(value.children) == size, (
                f"OAD {oad:08x} has {len(value.children)} children, expected {size}"
            )
