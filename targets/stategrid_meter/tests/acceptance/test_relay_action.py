from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6

RELAY_STATUS = 0x80000500
RELAY_OI = 0x8000

TRIP = bytes.fromhex("0205110005f205020111001200001100")
CLOSE = bytes.fromhex("0205110005f205020111011200001100")


def _frame(request: bytes) -> bytes:
    return protocol.build_frame(request, sa=BLANK_SA, ca=0x10, ctrl=0x43)


def _get(piid: int, oad: int) -> str:
    return _frame(protocol.get_request(piid, oad)).hex()


def _action(piid: int, method: int, data: bytes) -> str:
    return _frame(protocol.action_request(piid, RELAY_OI, method, param=data)).hex()


def _relay_actions(run) -> list[tuple[int, str]]:
    actions = []
    for line in run.relay_log:
        fields = line.split()
        assert fields[0] == "cycle" and fields[2] == "time", line
        assert fields[5] == "action", line
        actions.append((int(fields[1]), fields[6]))
    return actions


@case(
    "ACC-LC-001",
    "remote trip disconnects the relay and remote close reconnects it",
    ("control", "relay"),
)
def remote_trip_and_close(ctx):
    scenario = (
        f"at 2 uart 0 {_action(1, 0x81, TRIP)}\n"
        f"at 3 uart 0 {_get(2, RELAY_STATUS)}\n"
        f"at 4 uart 0 {_get(3, RELAY_STATUS)}\n"
        f"at 8 uart 0 {_action(4, 0x82, CLOSE)}\n"
        f"at 9 uart 0 {_get(5, RELAY_STATUS)}\n"
        f"at 10 uart 0 {_get(6, RELAY_STATUS)}\n"
    )
    run = ctx.run(cycles=14, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    responses = [
        protocol.parse_frame(frame).apdu
        for frame in protocol.iter_698_frames(run.uart_tx)
    ]
    assert len(responses) == 6, run.uart_tx.hex()

    trip = protocol.parse_action_response(responses[0])
    assert (trip.piid, trip.oi, trip.method) == (1, RELAY_OI, 0x81)
    assert trip.dar == 0, trip.dar

    close = protocol.parse_action_response(responses[3])
    assert (close.piid, close.oi, close.method) == (4, RELAY_OI, 0x82)
    assert close.dar == 0, close.dar

    statuses = []
    for response in responses[1:3] + responses[4:]:
        result = protocol.parse_get_response(response)
        assert result.oad == RELAY_STATUS
        statuses.append(result.data)
    assert statuses == [b"\x04\x08\x80", b"\x04\x08\x80",
                        b"\x04\x08\x00", b"\x04\x08\x00"], statuses

    actions = _relay_actions(run)
    assert actions[0] == (2, "disconnect"), actions
    assert (8, "connect") in actions, actions
