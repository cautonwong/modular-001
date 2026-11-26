"""Acceptance cases for the shared tariff table validation.

The 645 and 698 setters now call the same domain validator, so the 698 SET
responses must carry the count, fee, day-table and spacing DARs the mapper
maps from it.
"""

from __future__ import annotations

from cases import case

import protocol

BLANK_ADDRESS = b"\xff" * 6
FACTORY_MODE = b"\xfb"


def _frame(apdu: bytes) -> bytes:
    return protocol.build_frame(apdu, sa=BLANK_ADDRESS, ca=0x10, ctrl=0x43)


def _write_645(data_id: str, value: bytes) -> bytes:
    data = (
        protocol.data_id_bytes(data_id)
        + b"\x00\x00\x00\x00"
        + b"\x00\x00\x00\x00"
        + value
    )
    body = (
        bytes([0x68])
        + BLANK_ADDRESS
        + bytes([0x68, protocol.WRITE_DATA, len(data)])
        + bytes((byte + protocol.DATA_OFFSET) & 0xFF for byte in data)
    )
    return body + bytes([protocol.checksum(body), 0x16])


def _set(piid: int, oad: int, data: bytes) -> bytes:
    return _frame(protocol.set_request(piid, oad, data))


def _counts(seasons: int, tables: int, periods: int, fees: int, holidays: int) -> bytes:
    values = (seasons, tables, periods, fees, holidays)
    return bytes([0x01, 0x05, *sum([[0x11, value] for value in values], [])])


def _season_table(rows) -> bytes:
    out = bytes([0x01, len(rows)])
    for month, day, table in rows:
        out += bytes([0x02, 0x03, 0x11, month, 0x11, day, 0x11, table])
    return out


def _day_table(rows) -> bytes:
    out = bytes([0x01, len(rows)])
    for hour, minute, fee in rows:
        out += bytes([0x02, 0x03, 0x11, hour, 0x11, minute, 0x11, fee])
    return out


def _set_dars(run) -> list[int]:
    dars = []
    for frame in protocol.iter_698_frames(run.uart_tx):
        apdu = protocol.parse_frame(frame).apdu
        if apdu[0] == 0x86:
            dars.append(protocol.parse_set_response(apdu)[1])
    return dars


@case(
    "ACC-TAR-010",
    "a tariff table write outside the configured counts or the shared rules is "
    "rejected with the matching DAR, while a valid season and day table are "
    "accepted",
    ("tariff", "validation", "698"),
)
def tariff_table_validation(ctx):
    scenario = "\n".join(
        [
            f"at 0 uart 0 {_write_645('A8000100', FACTORY_MODE).hex()}",
            f"at 1 uart 0 {_set(1, 0x400C0200, _counts(2, 2, 2, 2, 0)).hex()}",
            f"at 2 uart 0 {_set(2, 0x40140200, _season_table([(1, 1, 3)])).hex()}",
            f"at 3 uart 0 {_set(3, 0x40140200, _season_table([(2, 29, 1)])).hex()}",
            f"at 4 uart 0 {_set(4, 0x40160201, _day_table([(0, 0, 3)])).hex()}",
            f"at 5 uart 0 {_set(5, 0x40160201, _day_table([(0, 0, 1), (0, 5, 1)])).hex()}",
            f"at 6 uart 0 {_set(6, 0x40140200, _season_table([(1, 1, 1), (5, 1, 2)])).hex()}",
            f"at 7 uart 0 {_set(7, 0x40160201, _day_table([(0, 0, 1), (0, 15, 2)])).hex()}",
        ]
    ) + "\n"
    run = ctx.run(cycles=10, scenario=scenario)
    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"

    dars = _set_dars(run)
    assert dars == [0, 3, 3, 19, 3, 0, 0], dars
