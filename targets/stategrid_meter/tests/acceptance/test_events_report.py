"""Event record reading and record-capture visibility.

Every check reads 698 responses or record bytes from the UART capture.
"""

from __future__ import annotations

from cases import case

import protocol

BLANK_SA = b"\xff" * 6


def _frame(apdu: bytes, ctrl: int = 0x43) -> bytes:
    return protocol.build_frame(apdu, sa=BLANK_SA, ca=0x10, ctrl=ctrl)


def _get(piid: int, oad: int) -> bytes:
    return _frame(protocol.get_request(piid, oad))


def _set(piid: int, oad: int, data: bytes) -> bytes:
    return _frame(protocol.set_request(piid, oad, data))


def _record_request(piid: int, oad: int, rsd: bytes) -> bytes:
    """GET-Request-Record: 05 | 03 | PIID | OAD | RSD | RCSD-null | time tag."""
    apdu = bytes([0x05, 0x03, piid]) + oad.to_bytes(4, "big") + rsd + b"\x00" + b"\x00"
    return _frame(apdu)


def _apdus(run) -> list[bytes]:
    return [protocol.parse_frame(frame).apdu for frame in protocol.iter_698_frames(run.uart_tx)]


def _record(apdu: bytes) -> tuple[int, bytes]:
    """Return (record count, record bytes) from a GET-Response-Record."""
    assert apdu[0] == 0x85 and apdu[1] == 0x03, apdu.hex()
    position = 8 + apdu[7] * 5
    if apdu[position] != 0x01:
        raise AssertionError(f"record request failed: {apdu.hex()}")
    return apdu[position + 1], apdu[position + 2 :]


def _one_record(apdu: bytes) -> bytes:
    count, body = _record(apdu)
    assert count == 1, (count, apdu.hex())
    return body


def _get_bytes(apdu: bytes) -> bytes:
    assert apdu[0] == 0x85 and apdu[1] == 0x01, apdu.hex()
    assert apdu[7] == 0x01, apdu.hex()
    return apdu[8:-2]


LAST_N = lambda n: b"\x09" + bytes([n])

# Cover-open event: OAD 0x301B0200 records, count 0x301B0400, value 0x301B0A00.
COVER_EVENT = 0x301B0200
COVER_COUNT = 0x301B0400
COVER_VALUE = 0x301B0A00
COVER_CAPTURE = 0x301B0300


@case(
    "ACC-EVT-009",
    "reading the cover-open event object returns the stored records: the "
    "count attribute is 2, LAST_N(1) and LAST_N(2) each return one distinct "
    "record with both event timestamps, and LAST_N(3) returns an empty list",
    ("events", "698", "record"),
)
def event_record_cursor(ctx):
    run = ctx.run(
        cycles=30,
        scenario=(
            "at 2 cover open\n"
            "at 8 cover closed\n"
            "at 12 cover open\n"
            "at 18 cover closed\n"
            f"at 22 uart 0 {_record_request(1, COVER_EVENT, LAST_N(1)).hex()}\n"
            f"at 23 uart 0 {_record_request(2, COVER_EVENT, LAST_N(2)).hex()}\n"
            f"at 24 uart 0 {_record_request(3, COVER_EVENT, LAST_N(3)).hex()}\n"
            f"at 25 uart 0 {_get(4, COVER_COUNT).hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    apdus = _apdus(run)
    assert len(apdus) == 4, [apdu.hex() for apdu in apdus]

    newest = _one_record(apdus[0])
    previous = _one_record(apdus[1])
    assert newest != previous, (newest.hex(), previous.hex())
    for body in (newest, previous):
        assert body[0] == 0x06, body.hex()
        assert body.count(b"\x1c") == 2, body.hex()
    assert _record(apdus[2])[0] == 0, apdus[2].hex()

    count = protocol.parse_get_response(apdus[3])
    assert count.oad == COVER_COUNT
    assert count.data.hex() == "120002", count.data.hex()


@case(
    "ACC-EVT-012",
    "configuring the cover-open capture list to one end-only object is "
    "accepted and read back, and the event record shows that object only in "
    "the end record: one timestamp before the event ends, three after",
    ("events", "698", "capture"),
)
def capture_visibility(ctx):
    # array(1) of DTC_OAD 0x20218200: freeze time 0x20210200 with scheme 4
    # (end-only), so it is invisible while the event is active.
    capture = bytes.fromhex("0101") + b"\x51" + (0x20218200).to_bytes(4, "big")
    run = ctx.run(
        cycles=22,
        scenario=(
            f"at 2 uart 0 {_set(1, COVER_CAPTURE, capture).hex()}\n"
            f"at 3 uart 0 {_get(2, COVER_CAPTURE).hex()}\n"
            "at 4 cover open\n"
            f"at 8 uart 0 {_record_request(3, COVER_EVENT, LAST_N(1)).hex()}\n"
            "at 12 cover closed\n"
            f"at 16 uart 0 {_record_request(4, COVER_EVENT, LAST_N(1)).hex()}\n"
        ),
    )
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    apdus = _apdus(run)
    assert len(apdus) == 4, [apdu.hex() for apdu in apdus]

    piid, dar = protocol.parse_set_response(apdus[0])
    assert (piid, dar) == (1, 0), (piid, dar)
    assert (0x20218200).to_bytes(4, "big") in _get_bytes(apdus[1]), apdus[1].hex()

    active = _one_record(apdus[2])
    ended = _one_record(apdus[3])
    assert active.count(b"\x1c") == 1, active.hex()
    assert ended.count(b"\x1c") == 3, ended.hex()
