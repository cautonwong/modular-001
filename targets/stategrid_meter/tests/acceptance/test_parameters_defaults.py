"""Acceptance row ACC-PARAM-001: factory default parameter initialization.

A boot with a blank parameter area reaches auto_parameter_inital() only when
the cover is open and a power-on message arrives, so the scenario opens the
cover and power-cycles. The assertions decode the EEPROM image with the
external parameter layout and then read two business OADs from the freshly
initialized meter address.
"""

from __future__ import annotations

from cases import case

import parameters
import protocol

METER_SA = b"\x11" * 6
ENERGY = 0x00000401
VOLTAGE = 0x20000201


def _get(piid: int, oad: int) -> str:
    frame = protocol.build_frame(
        protocol.get_request(piid, oad), sa=METER_SA, ca=0x10, ctrl=0x43
    )
    return frame.hex()


@case(
    "ACC-PARAM-001",
    "a blank parameter area with the cover open and one power cycle is "
    "auto-initialized: every parameter checksum verifies, the factory defaults "
    "(2000 imp/kWh, 300000 over-current, 1440 min, 0 s relay-off, address "
    "0x111111111111) are present, and energy/voltage GETs answer at that address",
    ("parameters", "defaults", "eeprom"),
)
def blank_parameter_auto_init(ctx):
    scenario = (
        "at 0 cover open\n"
        "at 1 power off\n"
        "at 3 power on\n"
        "at 6 meter const 2000 cal 128\n"
        f"at 8 uart 0 {_get(1, ENERGY)}\n"
        f"at 10 uart 0 {_get(2, VOLTAGE)}\n"
    )
    run = ctx.run(cycles=14, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    status = parameters.verify_parameter_checksums(run.eeprom)
    assert all(status.values()), status
    read = parameters.read_parameters(run.eeprom)
    assert read["meter_const"] == 2000, read["meter_const"]
    assert read["imax_protect_value"] == 300000, read["imax_protect_value"]
    assert read["imax_protect_time"] == 1440, read["imax_protect_time"]
    assert read["remote_relay_off_time"] == 0, read["remote_relay_off_time"]
    assert read["meter_address"] == b"\x11" * 6, read["meter_address"].hex()

    responses = {}
    for frame in protocol.iter_698_frames(run.uart_tx):
        result = protocol.parse_get_response(protocol.parse_frame(frame).apdu)
        responses[result.oad] = result.data
    assert responses[ENERGY][0] == 0x14, responses[ENERGY].hex()
    assert len(responses[ENERGY]) == 9, responses[ENERGY].hex()
    assert responses[VOLTAGE] == bytes.fromhex("120000"), responses[VOLTAGE].hex()
