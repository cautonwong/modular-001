"""DL/T 645-2007 and DL/T 698.45 frame builders and parsers.

Standard library only. The wire formats follow the read-only reference in
test-platform/src/testplatform/protocol/dlt645.py and dlt698.py; nothing is
imported from that tree at runtime.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass
from typing import Any


class FrameError(ValueError):
    """A frame or APDU is malformed."""


# ---------------------------------------------------------------------------
# DL/T 645-2007
# ---------------------------------------------------------------------------

FRAME_START = 0x68
FRAME_END = 0x16
READ_DATA = 0x11
# Write data: request 14H, normal response 94H, exception response D4H.
WRITE_DATA = 0x14
WRITE_RESPONSE = 0x94
WRITE_ERROR = 0xD4
# Remote control: request 1CH, normal response 9CH, exception response DCH.
CONTROL = 0x1C
CONTROL_RESPONSE = 0x9C
CONTROL_ERROR = 0xDC
CMD_TRIP = 0x1A
CMD_ALLOW_CLOSE = 0x1B
CMD_ALARM = 0x2A
CMD_ALARM_CLEAR = 0x2B
CMD_KEEP_POWER = 0x3A
CMD_KEEP_CLEAR = 0x3B

ADDRESS_BYTES = 6
DATA_ID_BYTES = 4
DATA_OFFSET = 0x33
MAX_DATA_BYTE = 0xFF - DATA_OFFSET
_HEADER_BYTES = 10
_MIN_FRAME = ADDRESS_BYTES + 6
_MAX_FRAME_DATA_LEN = 0xFF


def checksum(data: bytes) -> int:
    """DL/T 645 CS: arithmetic sum of the bytes, low byte."""
    return sum(data) & 0xFF


def address_bytes(address: str) -> bytes:
    """Six-byte BCD meter address, least significant byte first."""
    if (
        isinstance(address, str)
        and len(address) == ADDRESS_BYTES * 2
        and address.isascii()
        and address.isdigit()
    ):
        return bytes.fromhex(address)[::-1]
    raise ValueError(
        f"meter address must be {ADDRESS_BYTES * 2} BCD digits "
        f'(for example "000000000001"), got {address!r}'
    )


def address_text(raw: bytes) -> str:
    """Render six wire-order address bytes as the usual 12-digit string."""
    return raw[::-1].hex()


def data_id_bytes(data_id: str) -> bytes:
    """Four-byte data identifier, least significant byte first."""
    if isinstance(data_id, str) and len(data_id) == DATA_ID_BYTES * 2:
        try:
            return bytes.fromhex(data_id)[::-1]
        except ValueError:
            pass
    raise ValueError(
        f"data id must be {DATA_ID_BYTES * 2} hex digits "
        f'(for example "00010000"), got {data_id!r}'
    )


def data_id_text(raw: bytes) -> str:
    """Render four wire-order data-id bytes as the usual 8-digit string."""
    return raw[::-1].hex()


def build_read_request(address: str, data_id: str) -> bytes:
    """Build a read-data request: 68 | address | 68 | 11 | 04 | DI | CS | 16."""
    return _build_frame(address, READ_DATA, data_id_bytes(data_id))


def build_control_request(
    address: str,
    command: int,
    *,
    password: bytes = b"\x00\x00\x00\x00",
    operator: bytes = b"\x00\x00\x00\x00",
    payload: bytes = b"\x00",
) -> bytes:
    """Build a control request (C=1CH).

    Data layout is password(4) | operator code(4) | N1 command | payload.
    `payload` carries N2 and, for trip/close, the six-byte valid-until time
    the firmware reads at data[10].
    """
    if len(password) != 4 or len(operator) != 4:
        raise ValueError("password and operator code must each be 4 bytes")
    if not 0 <= command <= 0xFF:
        raise ValueError(f"control command must be one byte, got {command}")
    data = bytes(password) + bytes(operator) + bytes([command]) + bytes(payload)
    return _build_frame(address, CONTROL, data)


def parse_read_request(frame: bytes) -> tuple[str, str]:
    """Parse a read-data request into (meter address, data id)."""
    address, control, raw = parse_frame_645(frame)
    if control != READ_DATA:
        raise ValueError(f"not a read-data request (control {control:#04x}): {frame.hex()}")
    data = _unescape(raw)
    if len(data) != DATA_ID_BYTES:
        raise ValueError(
            f"read request carries {DATA_ID_BYTES} data-id bytes, "
            f"got {len(data)}: {frame.hex()}"
        )
    return address, data_id_text(data)


def parse_read_response(frame: bytes, address: str, data_id: str) -> bytes:
    """Validate a read-data response and return its value bytes, low byte first.

    A wrong address, the wrong data id, a bad checksum, or an exception
    response all raise instead of returning a plausible-looking reading.
    """
    expected_address = address_text(address_bytes(address))
    wanted_data_id = data_id_bytes(data_id)
    received_address, control, raw = parse_frame_645(frame)
    if received_address != expected_address:
        raise ValueError(
            f"response address is {received_address}, not {address}: {frame.hex()}"
        )
    if control == READ_DATA | 0x80 | 0x40:
        raise ValueError(
            f"meter {address} returned an exception response, "
            f"error word {raw.hex()}"
        )
    if control != READ_DATA | 0x80:
        raise ValueError(f"not a read-data response (control {control:#04x}): {frame.hex()}")
    data = _unescape(raw)
    if len(data) < DATA_ID_BYTES or data[:DATA_ID_BYTES] != wanted_data_id:
        found = data_id_text(data[:DATA_ID_BYTES]) if data else "<empty>"
        raise ValueError(
            f"response data id is {found}, not {data_id}: {frame.hex()}"
        )
    return data[DATA_ID_BYTES:]


def build_write_request(
    address: str,
    data_id: str,
    value: bytes,
    *,
    password: bytes = b"\x00\x00\x00\x00",
    operator: bytes = b"\x00\x00\x00\x00",
) -> bytes:
    """Build a write-data request (C=14H).

    Data layout is DI(4) | password(4) | operator(4) | value, matching
    Comm_Write() in targets/stategrid_meter/mapper_645/ctl_14_write.c.
    """
    if len(password) != 4 or len(operator) != 4:
        raise ValueError("password and operator code must each be 4 bytes")
    data = data_id_bytes(data_id) + bytes(password) + bytes(operator) + bytes(value)
    return _build_frame(address, WRITE_DATA, data)


def parse_write_response(frame: bytes, address: str, data_id: str) -> bytes:
    """Validate a write-data response and return its value bytes.

    The firmware answers success with C=94H and an empty data field. A device
    that echoes DI | value is accepted too. An exception response (C=D4H)
    raises with the error byte.
    """
    expected_address = address_text(address_bytes(address))
    wanted_data_id = data_id_bytes(data_id)
    received_address, control, raw = parse_frame_645(frame)
    if received_address != expected_address:
        raise ValueError(
            f"response address is {received_address}, not {address}: {frame.hex()}"
        )
    if control == WRITE_DATA | 0x80 | 0x40:
        raise ValueError(
            f"meter {address} rejected the write, "
            f"error byte {_unescape(raw).hex()}: {frame.hex()}"
        )
    if control != WRITE_DATA | 0x80:
        raise ValueError(f"not a write-data response (control {control:#04x}): {frame.hex()}")
    data = _unescape(raw)
    if data[:DATA_ID_BYTES] == wanted_data_id:
        return data[DATA_ID_BYTES:]
    if len(data) >= DATA_ID_BYTES:
        raise ValueError(
            f"response data id is {data_id_text(data[:DATA_ID_BYTES])}, "
            f"not {data_id}: {frame.hex()}"
        )
    return data


def parse_frame_645(frame: bytes) -> tuple[str, int, bytes]:
    """Validate a 645 frame and return (address, control, raw data field).

    The data field is returned with its 0x33 wire offset still applied; use
    `parse_read_request` or `parse_read_response` to decode it.
    """
    frame = _strip_wakeup(frame)
    if len(frame) < _MIN_FRAME:
        raise ValueError(f"frame shorter than the {_MIN_FRAME}-byte minimum: {frame.hex()}")
    if frame[0] != FRAME_START or frame[7] != FRAME_START or frame[-1] != FRAME_END:
        raise ValueError(f"bad frame delimiters (expected 68 ... 68 ... 16): {frame.hex()}")
    length = frame[9]
    if len(frame) != _MIN_FRAME + length:
        raise ValueError(
            f"frame length {len(frame)} does not match L={length} "
            f"(expected {_MIN_FRAME + length}): {frame.hex()}"
        )
    body, received = frame[:-2], frame[-2]
    expected = checksum(body)
    if received != expected:
        raise ValueError(
            f"checksum mismatch: frame has {received:#04x}, "
            f"body computes {expected:#04x}: {frame.hex()}"
        )
    return (
        address_text(frame[1 : 1 + ADDRESS_BYTES]),
        frame[ADDRESS_BYTES + 2],
        frame[ADDRESS_BYTES + 4 : -2],
    )


def response_length_645(frame: bytes) -> int | None:
    """Total frame length from L, or None until the 10-byte header arrives."""
    frame = _strip_wakeup(frame)
    if len(frame) < _HEADER_BYTES:
        return None
    return _MIN_FRAME + frame[9]


def iter_645_frames(data: bytes) -> list[bytes]:
    """Split a raw capture into complete 645 frames, skipping noise."""
    frames: list[bytes] = []
    i = 0
    while i < len(data):
        if data[i] != FRAME_START:
            i += 1
            continue
        chunk = data[i:]
        length = response_length_645(chunk)
        if length is None or len(chunk) < length:
            i += 1
            continue
        frame = chunk[:length]
        try:
            parse_frame_645(frame)
        except ValueError:
            i += 1
            continue
        frames.append(frame)
        i += length
    return frames


def _strip_wakeup(frame: bytes) -> bytes:
    # A master may send up to four FEH wake-up bytes before the frame.
    return frame.lstrip(b"\xfe") if frame.startswith(b"\xfe") else frame


def _build_frame(address: str, control: int, data: bytes) -> bytes:
    if len(data) > _MAX_FRAME_DATA_LEN:
        raise ValueError(
            f"L is one byte, so the data field is limited to "
            f"{_MAX_FRAME_DATA_LEN} bytes, got {len(data)}"
        )
    # The data field is offset by 0x33 on the wire (dlt645.py: _escape).
    body = (
        bytes([FRAME_START])
        + address_bytes(address)
        + bytes([FRAME_START, control, len(data)])
        + _escape(data)
    )
    return body + bytes([checksum(body), FRAME_END])


def _escape(data: bytes) -> bytes:
    for index, byte in enumerate(data):
        if byte > MAX_DATA_BYTE:
            raise ValueError(
                f"data byte {index} is {byte:#04x}, above the {MAX_DATA_BYTE:#04x} "
                f"ceiling (adding {DATA_OFFSET:#04x} must stay within 0xff)"
            )
    return bytes(byte + DATA_OFFSET for byte in data)


def _unescape(raw: bytes) -> bytes:
    if any(byte < DATA_OFFSET for byte in raw):
        raise ValueError(
            f"data bytes are offset by {DATA_OFFSET:#04x}; "
            f"a smaller byte means the frame is corrupt: {raw.hex()}"
        )
    return bytes(byte - DATA_OFFSET for byte in raw)


# ---------------------------------------------------------------------------
# DL/T 698.45
# ---------------------------------------------------------------------------

HEAD = 0x68
END = 0x16

SVC_GET_REQ = 0x05
SVC_SET_REQ = 0x06
SVC_ACTION_REQ = 0x07
SVC_GET_RES = 0x85
SVC_SET_RES = 0x86
SVC_ACTION_RES = 0x87
SVC_REPORT = 0x88

AXDR_NULL = 0x00
AXDR_ARRAY = 0x01
AXDR_STRUCTURE = 0x02
AXDR_BOOL = 0x03
AXDR_BIT_STRING = 0x04
AXDR_DOUBLE_LONG = 0x05
AXDR_DOUBLE_LONG_UNSIGNED = 0x06
AXDR_OCTET_STRING = 0x09
AXDR_VISIBLE_STRING = 0x0A
AXDR_UTF8_STRING = 0x0C
AXDR_INTEGER = 0x0F
AXDR_LONG = 0x10
AXDR_UNSIGNED = 0x11
AXDR_LONG_UNSIGNED = 0x12
AXDR_LONG64 = 0x14
AXDR_LONG64_UNSIGNED = 0x15
AXDR_ENUM = 0x16
AXDR_FLOAT32 = 0x17
AXDR_FLOAT64 = 0x18
AXDR_DATE_TIME = 0x19
AXDR_DATE = 0x1A
AXDR_TIME = 0x1B
AXDR_DATE_TIME_S = 0x1C

_APDU_TIME_TAG = b"\x00"
_APDU_RESPONSE_TAIL = b"\x00\x00"

_DAR_TEXT = {
    0: "success",
    1: "hardware failure",
    2: "temporarily unavailable",
    3: "read/write denied",
    4: "object undefined",
    5: "interface class mismatch",
    6: "object absent",
    7: "type mismatch",
    8: "out of range",
    15: "password error or unauthorized",
    20: "security authentication mismatch",
    22: "ESAM verification failed",
    23: "security authentication failed",
    27: "address error",
    33: "request timeout",
    255: "other",
}


@dataclass(frozen=True)
class Frame:
    """A parsed 698 link-layer frame."""

    sa: bytes
    ctrl: int
    apdu: bytes
    raw: bytes


def fcs16(data: bytes) -> int:
    """CRC-16/X-25: reflected polynomial 0x8408, init 0xFFFF, final xor 0xFFFF."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if crc & 1 else crc >> 1
    return crc ^ 0xFFFF


def build_frame(
    apdu: bytes,
    sa: bytes = b"\x01",
    ca: int = 0x10,
    ctrl: int = 0x43,
) -> bytes:
    """Wrap an APDU in a 698 frame: 68 | L | C | SA | CA | HCS | APDU | FCS | 16."""
    if not apdu:
        raise FrameError("APDU must not be empty")
    if not 1 <= len(sa) <= 16:
        raise FrameError(f"SA length must be 1..16, got {len(sa)}")

    body = bytes([ctrl]) + bytes([(len(sa) - 1) & 0x0F]) + bytes(sa) + bytes([ca])
    # L counts every byte except the two delimiters (dlt698.py: build_frame).
    l_field = 2 + len(body) + 2 + len(apdu) + 2
    if l_field > 0x3FFF:
        raise FrameError(f"frame too long: L={l_field} exceeds the 14-bit limit")

    # HCS covers L + C + SA + CA, including the length field
    # (dlt698.py: build_frame).
    head = l_field.to_bytes(2, "little") + body
    hcs = fcs16(head)
    frame = bytes([HEAD]) + head + hcs.to_bytes(2, "little") + apdu
    return frame + fcs16(frame[1:]).to_bytes(2, "little") + bytes([END])


def parse_frame(data: bytes) -> Frame:
    """Validate and split a 698 frame; any inconsistency raises FrameError."""
    if len(data) < 12:
        raise FrameError(f"frame too short: {len(data)} bytes")
    if data[0] != HEAD:
        raise FrameError(f"start delimiter is 0x{data[0]:02X}, not 0x68")
    if data[-1] != END:
        raise FrameError(f"end delimiter is 0x{data[-1]:02X}, not 0x16")

    l_field = int.from_bytes(data[1:3], "little") & 0x3FFF
    if l_field != len(data) - 2:
        raise FrameError(
            f"length field {l_field} does not match frame length {len(data)} "
            f"(expected {len(data) - 2})"
        )

    want_fcs = fcs16(data[1:-3])
    got_fcs = int.from_bytes(data[-3:-1], "little")
    if want_fcs != got_fcs:
        raise FrameError(f"FCS mismatch: computed 0x{want_fcs:04X}, frame has 0x{got_fcs:04X}")

    sa_len = (data[4] & 0x0F) + 1
    sa_start = 5
    sa = data[sa_start : sa_start + sa_len]
    ca_idx = sa_start + sa_len
    want_hcs = fcs16(data[1 : ca_idx + 1])
    got_hcs = int.from_bytes(data[ca_idx + 1 : ca_idx + 3], "little")
    if want_hcs != got_hcs:
        raise FrameError(f"HCS mismatch: computed 0x{want_hcs:04X}, frame has 0x{got_hcs:04X}")

    return Frame(sa=sa, ctrl=data[3], apdu=data[ca_idx + 3 : -3], raw=data)


def response_length(frame: bytes) -> int | None:
    """Total frame length from L, or None until the three-byte prefix arrives."""
    if len(frame) < 3:
        return None
    total = (int.from_bytes(frame[1:3], "little") & 0x3FFF) + 2
    return total if total >= 12 else None


def iter_698_frames(data: bytes) -> list[bytes]:
    """Split a raw capture into complete 698 frames, skipping noise."""
    frames: list[bytes] = []
    i = 0
    while i < len(data):
        if data[i] != HEAD:
            i += 1
            continue
        chunk = data[i:]
        length = response_length(chunk)
        if length is None or len(chunk) < length:
            i += 1
            continue
        frame = chunk[:length]
        try:
            parse_frame(frame)
        except FrameError:
            i += 1
            continue
        frames.append(frame)
        i += length
    return frames


def get_request(piid: int, oad: int) -> bytes:
    """GET-Request-Normal: 05 | 01 | PIID | OAD(4) | time tag."""
    return (
        bytes([SVC_GET_REQ, 0x01, piid & 0xFF])
        + oad.to_bytes(4, "big")
        + _APDU_TIME_TAG
    )


def get_response(piid: int, oad: int, data: bytes) -> bytes:
    """GET-Response-Normal: 85 | 01 | PIID | OAD(4) | 01 | Data | 00 | 00."""
    return (
        bytes([SVC_GET_RES, 0x01, piid & 0xFF])
        + oad.to_bytes(4, "big")
        + b"\x01"
        + data
        + _APDU_RESPONSE_TAIL
    )


def set_request(piid: int, oad: int, data: bytes) -> bytes:
    """SET-Request-Normal: 06 | 01 | PIID | OAD(4) | Data | time tag.

    The gatekeeper reads the normal-choice byte and fr_timetag_check requires
    the trailing 0x00 marker when no time tag is present. `data` must already
    be A-XDR encoded, including its type tag.
    """
    return (
        bytes([SVC_SET_REQ, 0x01, piid & 0xFF])
        + oad.to_bytes(4, "big")
        + data
        + _APDU_TIME_TAG
    )


def parse_set_response(apdu: bytes) -> tuple[int, int]:
    """Parse SET-Response-Normal and return (PIID, DAR).

    Layout: 86 | 01 | PIID | OAD(4) | DAR | FollowReport | time tag.
    """
    if len(apdu) < 10:
        raise FrameError(f"SET-Response too short: {len(apdu)} bytes")
    if apdu[0] != SVC_SET_RES:
        raise FrameError(f"not a SET-Response: service code 0x{apdu[0]:02X}")
    if apdu[1] != 0x01:
        raise FrameError(f"only SetResponseNormal([1]) is supported, got [{apdu[1]}]")
    return apdu[2], apdu[7]


def action_request(
    piid: int,
    oi: int,
    method: int,
    *,
    mode: int = 0,
    param: bytes | None = None,
) -> bytes:
    """ACTION-Request-Normal: 07 | 01 | PIID | OMD(4) | Data | time tag.

    `param` must already be A-XDR encoded, including its type tag.
    """
    if not 1 <= method <= 0xFF:
        raise FrameError(f"method must be 1..255, got {method}")
    omd = oi.to_bytes(2, "big") + bytes([method, mode & 0xFF])
    return (
        bytes([SVC_ACTION_REQ, 0x01, piid & 0xFF])
        + omd
        + (param or b"")
        + _APDU_TIME_TAG
    )


@dataclass(frozen=True)
class ActionResult:
    """A parsed ACTION-Response-Normal."""

    piid: int
    oi: int
    method: int
    mode: int
    dar: int
    data: bytes

    @property
    def ok(self) -> bool:
        return self.dar == 0

    @property
    def error(self) -> str:
        return _DAR_TEXT.get(self.dar, f"undefined result {self.dar}")


def parse_action_response(apdu: bytes) -> ActionResult:
    """Parse ACTION-Response-Normal and expose DAR as `.ok` / `.error`."""
    if len(apdu) < 9:
        raise FrameError(f"ACTION-Response too short: {len(apdu)} bytes")
    if apdu[0] != SVC_ACTION_RES:
        raise FrameError(f"not an ACTION-Response: service code 0x{apdu[0]:02X}")
    if apdu[1] != 0x01:
        raise FrameError(f"only ActionResponseNormal([1]) is supported, got [{apdu[1]}]")

    piid = apdu[2]
    oi = int.from_bytes(apdu[3:5], "big")
    method, mode = apdu[5], apdu[6]
    dar = apdu[7]
    rest = apdu[8:]
    if len(rest) < 3:
        raise FrameError(f"ACTION-Response is missing the data flag or tail: {len(rest)} bytes")

    data = b""
    if rest[0] == 0x01:
        data = rest[1:-2]
        if not data:
            raise FrameError("ACTION-Response data flag is 01 but the data section is empty")
        axdr_values(data)
    elif rest[0] != 0x00:
        raise FrameError(f"unknown ACTION-Response data flag: 0x{rest[0]:02X}")
    return ActionResult(piid=piid, oi=oi, method=method, mode=mode, dar=dar, data=data)


@dataclass(frozen=True)
class AxdrValue:
    """One A-XDR value. Scalars keep raw bytes in `payload`; containers keep children."""

    tag: int
    payload: bytes = b""
    children: tuple["AxdrValue", ...] = ()

    @property
    def is_container(self) -> bool:
        return self.tag in (AXDR_ARRAY, AXDR_STRUCTURE)

    @property
    def value(self) -> Any:
        """Decoded Python value for simple tags; containers become tuples."""
        if self.tag == AXDR_NULL:
            return None
        if self.tag == AXDR_BOOL:
            return self.payload != b"\x00"
        if self.tag == AXDR_BIT_STRING:
            return self.payload
        if self.tag in (AXDR_DATE_TIME, AXDR_DATE, AXDR_TIME, AXDR_DATE_TIME_S):
            return self.payload
        if self.tag in _AXDR_SIGNED:
            return int.from_bytes(self.payload, "big", signed=True)
        if self.tag in _AXDR_FLOATS:
            return struct.unpack(_AXDR_FLOATS[self.tag], self.payload)[0]
        if self.tag in (AXDR_OCTET_STRING, AXDR_UTF8_STRING):
            return self.payload
        if self.tag == AXDR_VISIBLE_STRING:
            return self.payload.decode("ascii", "replace")
        if self.is_container:
            return tuple(child.value for child in self.children)
        return int.from_bytes(self.payload, "big")


_AXDR_FIXED = {
    AXDR_DOUBLE_LONG: 4,
    AXDR_DOUBLE_LONG_UNSIGNED: 4,
    AXDR_INTEGER: 1,
    AXDR_LONG: 2,
    AXDR_UNSIGNED: 1,
    AXDR_LONG_UNSIGNED: 2,
    AXDR_LONG64: 8,
    AXDR_LONG64_UNSIGNED: 8,
    AXDR_ENUM: 1,
    AXDR_FLOAT32: 4,
    AXDR_FLOAT64: 8,
    AXDR_DATE_TIME: 10,
    AXDR_DATE: 5,
    AXDR_TIME: 3,
    AXDR_DATE_TIME_S: 7,
}
_AXDR_SIGNED = {AXDR_DOUBLE_LONG, AXDR_INTEGER, AXDR_LONG, AXDR_LONG64}
_AXDR_FLOATS = {AXDR_FLOAT32: ">f", AXDR_FLOAT64: ">d"}
_AXDR_STRINGS = {AXDR_OCTET_STRING, AXDR_VISIBLE_STRING, AXDR_UTF8_STRING}
_MAX_AXDR_DEPTH = 16


def axdr_encode(tag: int, value: Any) -> bytes:
    """Encode one simple A-XDR value.

    `value` is a bool for BOOL, an int for numeric tags, bytes or text for
    strings, and a sequence of already-encoded values for ARRAY/STRUCTURE.
    """
    if tag == AXDR_NULL:
        return bytes([AXDR_NULL])
    if tag == AXDR_BOOL:
        return bytes([AXDR_BOOL, 1 if value else 0])
    if tag in _AXDR_FLOATS:
        return bytes([tag]) + struct.pack(_AXDR_FLOATS[tag], value)
    if tag in _AXDR_STRINGS:
        if isinstance(value, str):
            payload = value.encode("ascii" if tag == AXDR_VISIBLE_STRING else "utf-8")
        else:
            payload = bytes(value)
        if len(payload) > 0xFF:
            raise FrameError(f"A-XDR string is limited to 255 bytes, got {len(payload)}")
        return bytes([tag, len(payload)]) + payload
    if tag in (AXDR_ARRAY, AXDR_STRUCTURE):
        items = [bytes(item) for item in value]
        if len(items) > 0xFF:
            raise FrameError(f"A-XDR container is limited to 255 elements, got {len(items)}")
        return bytes([tag, len(items)]) + b"".join(items)
    if tag in _AXDR_FIXED:
        width = _AXDR_FIXED[tag]
        if isinstance(value, (bytes, bytearray)):
            payload = bytes(value)
            if len(payload) != width:
                raise FrameError(
                    f"A-XDR tag 0x{tag:02X} needs {width} bytes, got {len(payload)}"
                )
        else:
            try:
                payload = int(value).to_bytes(width, "big", signed=tag in _AXDR_SIGNED)
            except OverflowError as exc:
                raise FrameError(f"value {value!r} does not fit A-XDR tag 0x{tag:02X}") from exc
        return bytes([tag]) + payload
    raise FrameError(f"unsupported A-XDR tag 0x{tag:02X}")


def axdr_decode(data: bytes) -> tuple[AxdrValue, int]:
    """Decode one A-XDR value from the start of `data`; returns (value, bytes used)."""
    return _decode_one(data, 0)


def axdr_values(data: bytes) -> tuple[AxdrValue, ...]:
    """Decode a run of A-XDR values; trailing or truncated bytes raise FrameError."""
    values: list[AxdrValue] = []
    offset = 0
    while offset < len(data):
        value, consumed = _decode_one(data[offset:], 0)
        values.append(value)
        offset += consumed
    return tuple(values)


def _decode_one(data: bytes, depth: int) -> tuple[AxdrValue, int]:
    if not data:
        raise FrameError("A-XDR data is empty")
    tag = data[0]
    if tag == AXDR_NULL:
        return AxdrValue(tag), 1
    if tag == AXDR_BOOL:
        if len(data) < 2:
            raise FrameError("A-XDR bool is missing its byte")
        return AxdrValue(tag, data[1:2]), 2
    if tag == AXDR_BIT_STRING:
        if len(data) < 2:
            raise FrameError("A-XDR bit-string is missing its bit count")
        bits = data[1]
        payload_len = (bits + 7) // 8
        width = 2 + payload_len
        if len(data) < width:
            raise FrameError(
                f"A-XDR bit-string declares {bits} bits, only "
                f"{len(data) - 2} payload bytes available: {data.hex()}"
            )
        return AxdrValue(tag, data[2:width]), width
    if tag in _AXDR_FIXED:
        width = 1 + _AXDR_FIXED[tag]
        if len(data) < width:
            raise FrameError(
                f"A-XDR tag 0x{tag:02X} needs {width} bytes, only {len(data)} available: {data.hex()}"
            )
        return AxdrValue(tag, data[1:width]), width
    if tag in _AXDR_STRINGS:
        if len(data) < 2:
            raise FrameError(f"A-XDR tag 0x{tag:02X} is missing its length byte")
        width = 2 + data[1]
        if len(data) < width:
            raise FrameError(
                f"A-XDR tag 0x{tag:02X} declares {data[1]} bytes, "
                f"only {len(data) - 2} available: {data.hex()}"
            )
        return AxdrValue(tag, data[2:width]), width
    if tag in (AXDR_ARRAY, AXDR_STRUCTURE):
        if depth >= _MAX_AXDR_DEPTH:
            raise FrameError(f"A-XDR nesting exceeds {_MAX_AXDR_DEPTH} levels")
        if len(data) < 2:
            raise FrameError(f"A-XDR tag 0x{tag:02X} is missing its element count")
        count = data[1]
        offset = 2
        children: list[AxdrValue] = []
        for _ in range(count):
            child, consumed = _decode_one(data[offset:], depth + 1)
            children.append(child)
            offset += consumed
        return AxdrValue(tag, children=tuple(children)), offset
    raise FrameError(f"unknown A-XDR tag 0x{tag:02X}")


@dataclass(frozen=True)
class GetResult:
    """A parsed GET-Response-Normal."""

    piid: int
    oad: int
    data: bytes

    def values(self) -> tuple[AxdrValue, ...]:
        """Decode the Data field, keeping arrays and structures as one layer."""
        return axdr_values(self.data)

    def scalars(self) -> list[tuple[int, bytes]]:
        """Return (tag, payload) pairs; refuse to pick an element out of a container."""
        values = self.values()
        containers = [value for value in values if value.is_container]
        if containers:
            raise FrameError(
                f"Data is an array/structure (0x{containers[0].tag:02X}); "
                "the scalar view cannot choose one element"
            )
        return [(value.tag, value.payload) for value in values]


def parse_get_response(apdu: bytes) -> GetResult:
    """Parse GET-Response-Normal; a DAR response raises instead of returning data."""
    if len(apdu) < 10:
        raise FrameError(f"GET-Response too short (missing tail): {len(apdu)} bytes")
    if apdu[0] != SVC_GET_RES:
        raise FrameError(f"not a GET-Response: service code 0x{apdu[0]:02X}")
    if apdu[1] != 0x01:
        raise FrameError(f"only GetResponseNormal([1]) is supported, got [{apdu[1]}]")

    piid = apdu[2]
    oad = int.from_bytes(apdu[3:7], "big")
    flag = apdu[7]
    if flag != 0x01:
        dar = apdu[8] if len(apdu) > 8 else None
        detail = f" ({_DAR_TEXT[dar]})" if dar in _DAR_TEXT else ""
        raise FrameError(f"GET failed: DAR={dar}{detail}")
    data = apdu[8:-2]
    values = axdr_values(data)
    if len(values) != 1:
        raise FrameError(
            f"GET-Response Data must hold one A-XDR value, decoded {len(values)}: {data.hex()}"
        )
    return GetResult(piid=piid, oad=oad, data=data)
