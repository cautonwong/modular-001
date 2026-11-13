"""DL/T 698.45 security-mode acceptance rows.

SecurityMode_En (0xF1010200) gates the SecurityMode_EXData table (0xF1010300).
The mode cases configure the table in factory mode, leave factory through 645,
then read the response DARs. Cipher/MAC allow paths stay blocked on the ESAM
stub in tests/sim/sim_stubs.c; only the denial paths are asserted.
"""

from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6
SEC_ON = b"\x11\x01"
BOOL_ON = b"\x03\x01"
CIPHER_BLOCK = b"\x11" * 8
FACTORY_LEAVE_DATA = (
    protocol.data_id_bytes("A8000100") + b"\x02\x00\x00\x00" + b"\x00" * 4 + b"\x00"
)


def _frame(apdu: bytes, ctrl: int = 0x43) -> bytes:
    return protocol.build_frame(apdu, sa=BLANK_SA, ca=0x10, ctrl=ctrl)


def _get(piid: int, oad: int) -> str:
    return _frame(protocol.get_request(piid, oad)).hex()


def _set(piid: int, oad: int, data: bytes) -> str:
    return _frame(protocol.set_request(piid, oad, data)).hex()


def _action(piid: int, oi: int, method: int, param: bytes) -> str:
    return _frame(protocol.action_request(piid, oi, method, mode=0, param=param)).hex()


def _645(control: int, data: bytes) -> str:
    body = bytes([0x68]) + b"\xff" * 6 + bytes([0x68, control, len(data)])
    body += bytes((byte + 0x33) & 0xFF for byte in data)
    return (body + bytes([protocol.checksum(body), 0x16])).hex()


def _security_request(choice: int, inner: bytes, verification: int) -> str:
    apdu = bytes([0x10, choice, len(inner)]) + inner + bytes([verification])
    return _frame(apdu).hex()


def _security_table(entries: list[tuple[int, int]]) -> bytes:
    body = bytes([0x01, len(entries)])
    for oi, mode in entries:
        body += bytes([0x02, 0x02, 0x50]) + oi.to_bytes(2, "big")
        body += bytes([0x12]) + mode.to_bytes(2, "big")
    return body


def _apdus(run) -> list[bytes]:
    return [protocol.parse_frame(frame).apdu for frame in protocol.iter_698_frames(run.uart_tx)]


def _get_dar(apdu: bytes) -> int:
    assert apdu[:2] == b"\x85\x01", apdu.hex()
    return apdu[8]


@case(
    "ACC-SEC-001",
    "a listed OI is denied with DAR 20 or served with DAR 0/4 according to its "
    "security mode word, and SecurityMode_En=0 leaves the same table unenforced",
    ("dl698", "security"),
)
def security_mode_lookup(ctx):
    table = _security_table(
        [
            (0x3011, 0x4000),  # GET plain not allowed
            (0x202C, 0x8000),  # GET plain allowed
            (0x4300, 0x0800),  # SET plain allowed
            (0x5000, 0x0400),  # SET plain MAC required
        ]
    )
    enabled = ctx.run(
        cycles=12,
        scenario=(
            f"at 1 uart 0 {_set(1, 0xF1010200, SEC_ON)}\n"
            f"at 2 uart 0 {_set(2, 0xF1010300, table)}\n"
            f"at 3 uart 0 {_645(0x14, FACTORY_LEAVE_DATA)}\n"
            f"at 5 uart 0 {_get(3, 0x30110400)}\n"
            f"at 6 uart 0 {_get(4, 0x202C0200)}\n"
            f"at 7 uart 0 {_set(5, 0x43000700, BOOL_ON)}\n"
            f"at 8 uart 0 {_set(6, 0x50000400, BOOL_ON)}\n"
            f"at 9 uart 0 {_get(7, 0xF1010300)}\n"
        ),
    )
    assert enabled.exit_code == 0, f"firmware_sim exited {enabled.exit_code}"
    apdus = _apdus(enabled)
    assert len(apdus) == 7, [apdu.hex() for apdu in apdus]
    assert protocol.parse_set_response(apdus[0]) == (1, 0), apdus[0].hex()
    assert protocol.parse_set_response(apdus[1]) == (2, 0), apdus[1].hex()
    assert _get_dar(apdus[2]) == 20, apdus[2].hex()
    assert _get_dar(apdus[3]) == 4, apdus[3].hex()
    assert protocol.parse_set_response(apdus[4]) == (5, 0), apdus[4].hex()
    assert protocol.parse_set_response(apdus[5]) == (6, 20), apdus[5].hex()
    readback = apdus[6]
    assert readback[7] == 0x01, readback.hex()
    data = readback[8:-2]
    for entry in ("503011124000", "50202c128000", "504300120800", "505000120400"):
        assert bytes.fromhex(entry) in data, (entry, data.hex())

    bypassed = ctx.run(
        cycles=8,
        scenario=(
            f"at 1 uart 0 {_set(1, 0xF1010300, table)}\n"
            f"at 2 uart 0 {_645(0x14, FACTORY_LEAVE_DATA)}\n"
            f"at 4 uart 0 {_get(2, 0x30110400)}\n"
            f"at 5 uart 0 {_set(3, 0x50000400, BOOL_ON)}\n"
        ),
    )
    assert bypassed.exit_code == 0, f"firmware_sim exited {bypassed.exit_code}"
    apdus = _apdus(bypassed)
    assert len(apdus) == 3, [apdu.hex() for apdu in apdus]
    assert protocol.parse_get_response(apdus[1]).data == bytes.fromhex("120000")
    assert protocol.parse_set_response(apdus[2]) == (3, 3), apdus[2].hex()


@case(
    "ACC-SEC-002-BLOCKED",
    "a security request with verification choice 4, with ciphertext plus "
    "SID_MAC on an unconnected link, and a unicast plaintext ACTION on the "
    "broadcast-time object 0x40007F00 each return the expected SECURITY_Response DAR",
    ("dl698", "security"),
)
def security_request_checks(ctx):
    run = ctx.run(
        cycles=6,
        scenario=(
            f"at 1 uart 0 {_security_request(0x00, protocol.get_request(1, 0xF1010200), 0x04)}\n"
            f"at 2 uart 0 {_security_request(0x01, CIPHER_BLOCK, 0x00)}\n"
            f"at 3 uart 0 {_security_request(0x00, protocol.action_request(3, 0x4000, 0x7F, mode=0), 0x01)}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    apdus = _apdus(run)
    assert [apdu.hex() for apdu in apdus] == ["90020100", "90020f00", "90020300"], [
        apdu.hex() for apdu in apdus
    ]


@case(
    "ACC-SEC-006-BLOCKED",
    "with SecurityMode_En on, plaintext GET is served where GET_PLAIN is set, "
    "plaintext ACTION runs where ACTION_PLAIN is set, and plaintext ACTION is "
    "denied with DAR 20 where only ACTION_PLAIN_MAC is set",
    ("dl698", "security"),
)
def security_mode_transition(ctx):
    table = _security_table(
        [
            (0x8000, 0x8080),  # GET_PLAIN + ACTION_PLAIN
            (0x2015, 0x0080),  # ACTION_PLAIN
            (0x5000, 0x0040),  # ACTION_PLAIN_MAC
        ]
    )
    run = ctx.run(
        cycles=10,
        scenario=(
            f"at 1 uart 0 {_set(1, 0xF1010200, SEC_ON)}\n"
            f"at 2 uart 0 {_set(2, 0xF1010300, table)}\n"
            f"at 3 uart 0 {_645(0x14, FACTORY_LEAVE_DATA)}\n"
            f"at 5 uart 0 {_get(3, 0x80000400)}\n"
            f"at 6 uart 0 {_action(4, 0x2015, 0x7F, bytes.fromhex('042000000001'))}\n"
            f"at 7 uart 0 {_action(5, 0x5000, 3, bytes.fromhex('120000'))}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    apdus = _apdus(run)
    assert len(apdus) == 5, [apdu.hex() for apdu in apdus]
    assert protocol.parse_set_response(apdus[0]) == (1, 0), apdus[0].hex()
    assert protocol.parse_set_response(apdus[1]) == (2, 0), apdus[1].hex()
    assert protocol.parse_get_response(apdus[2]).data == bytes.fromhex("040800")
    allowed = protocol.parse_action_response(apdus[3])
    assert (allowed.oi, allowed.method, allowed.dar) == (0x2015, 0x7F, 0), apdus[3].hex()
    denied = protocol.parse_action_response(apdus[4])
    assert (denied.oi, denied.method, denied.dar) == (0x5000, 3, 20), apdus[4].hex()
