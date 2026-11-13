"""Wire-vector and round-trip tests for the standalone protocol codec."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import protocol


class Dlt645Tests(unittest.TestCase):
    def test_read_request_matches_upstream_vector(self):
        frame = protocol.build_read_request("000000000001", "00010000")
        self.assertEqual(frame.hex(), "6801000000000068110433333433b316")

    def test_read_request_round_trip(self):
        frame = protocol.build_read_request("000000000001", "00010000")
        self.assertEqual(
            protocol.parse_read_request(frame), ("000000000001", "00010000")
        )

    def test_read_response_matches_reference_vector(self):
        frame = bytes.fromhex("6801000000000068910833333433896745339f16")
        self.assertEqual(
            protocol.parse_read_response(frame, "000000000001", "00010000"),
            bytes.fromhex("56341200"),
        )

    def test_exception_response_is_rejected_before_unescaping(self):
        frame = bytes.fromhex("6801000000000068d10104a716")
        with self.assertRaisesRegex(ValueError, "exception response"):
            protocol.parse_read_response(frame, "000000000001", "00010000")

    def test_control_request_round_trip(self):
        frame = protocol.build_control_request(
            "000000000001", protocol.CMD_ALLOW_CLOSE
        )
        address, control, raw_data = protocol.parse_frame_645(frame)
        self.assertEqual(address, "000000000001")
        self.assertEqual(control, protocol.CONTROL)
        self.assertEqual(raw_data.hex(), "33" * 8 + "4e" + "33")
        self.assertEqual(protocol.response_length_645(frame), len(frame))

    def test_write_request_matches_firmware_layout(self):
        frame = protocol.build_write_request(
            "000000000001", "00010000", bytes.fromhex("1234")
        )
        self.assertEqual(
            frame.hex(),
            "6801000000000068140e33333433333333333333333345670416",
        )

    def test_write_response_returns_empty_value_and_rejects_exception(self):
        normal = bytes.fromhex("680100000000006894006516")
        self.assertEqual(
            protocol.parse_write_response(normal, "000000000001", "00010000"),
            b"",
        )
        exception = bytes.fromhex("6801000000000068d40138de16")
        with self.assertRaisesRegex(ValueError, "error byte 05"):
            protocol.parse_write_response(
                exception, "000000000001", "00010000"
            )

    def test_corrupted_checksum_is_rejected(self):
        frame = bytearray(protocol.build_read_request("000000000001", "00010000"))
        frame[-2] ^= 0xFF
        with self.assertRaisesRegex(ValueError, "checksum"):
            protocol.parse_frame_645(bytes(frame))


class Dlt698Tests(unittest.TestCase):
    FULL_FRAME = bytes.fromhex(
        "68190003000110d8bc8501012000020001174367b333000075de16"
    )

    def test_full_frame_matches_standard_vector(self):
        frame = protocol.parse_frame(self.FULL_FRAME)
        self.assertEqual(frame.ctrl, 0x03)
        self.assertEqual(frame.sa, b"\x01")
        self.assertEqual(
            frame.apdu.hex(), "8501012000020001174367b3330000"
        )
        self.assertEqual(protocol.fcs16(self.FULL_FRAME[1:-3]), 0xDE75)
        self.assertEqual(protocol.response_length(self.FULL_FRAME), len(self.FULL_FRAME))

    def test_frame_build_round_trip(self):
        apdu = protocol.get_request(0x01, 0x40010200)
        frame = protocol.build_frame(apdu)
        self.assertEqual(frame[0], protocol.HEAD)
        self.assertEqual(frame[-1], protocol.END)
        self.assertEqual(int.from_bytes(frame[1:3], "little"), len(frame) - 2)
        self.assertEqual(protocol.parse_frame(frame).apdu, apdu)

    def test_get_request_matches_standard_vector(self):
        self.assertEqual(
            protocol.get_request(0x01, 0x40010200).hex(), "0501014001020000"
        )

    def test_get_response_matches_standard_vector_and_round_trips(self):
        apdu = protocol.get_response(
            0x01, 0x40010200, bytes.fromhex("0906201601290001")
        )
        self.assertEqual(apdu.hex(), "850101400102000109062016012900010000")
        result = protocol.parse_get_response(apdu)
        self.assertEqual((result.piid, result.oad), (0x01, 0x40010200))
        self.assertEqual(
            result.scalars(), [(protocol.AXDR_OCTET_STRING, bytes.fromhex("201601290001"))]
        )

    def test_set_request_matches_firmware_vector(self):
        frame = protocol.build_frame(
            protocol.set_request(
                5, 0x40010200, bytes.fromhex("0906111111111111")
            ),
            sa=bytes.fromhex("111111111111"),
            ca=0x11,
            ctrl=0x43,
        )
        self.assertEqual(
            frame.hex(),
            "681f00430511111111111111499b06010540010200090611111111111100"
            "4eee16",
        )

    def test_set_response_returns_piid_and_dar(self):
        self.assertEqual(
            protocol.parse_set_response(bytes.fromhex("86010540010200000000")),
            (5, 0),
        )
        self.assertEqual(
            protocol.parse_set_response(bytes.fromhex("860105400102001b0000")),
            (5, 0x1B),
        )

    def test_action_request_matches_standard_vector(self):
        apdu = protocol.action_request(
            0x05, 0x0010, 0x01, param=bytes.fromhex("0F00")
        )
        self.assertEqual(apdu.hex(), "070105001001000f0000")

    def test_action_response_matches_standard_vectors(self):
        success = protocol.parse_action_response(bytes.fromhex("8701050010010000000000"))
        self.assertTrue(success.ok)
        self.assertEqual(success.dar, 0)
        failed = protocol.parse_action_response(bytes.fromhex("870107800001000f000000"))
        self.assertFalse(failed.ok)
        self.assertEqual(failed.dar, 15)
        self.assertIn("password", failed.error.lower())

    def test_axdr_float32_matches_the_standard_frame_payload(self):
        encoded = protocol.axdr_encode(protocol.AXDR_FLOAT32, 231.7)
        self.assertEqual(encoded.hex(), "174367b333")
        value, consumed = protocol.axdr_decode(encoded)
        self.assertEqual(consumed, len(encoded))
        self.assertEqual(value.tag, protocol.AXDR_FLOAT32)
        self.assertAlmostEqual(value.value, 231.7, places=3)

    def test_axdr_array_round_trip_keeps_children(self):
        child = protocol.axdr_encode(protocol.AXDR_LONG_UNSIGNED, 0x096D)
        encoded = protocol.axdr_encode(protocol.AXDR_ARRAY, [child, child, child])
        value, consumed = protocol.axdr_decode(encoded)
        self.assertEqual(consumed, len(encoded))
        self.assertTrue(value.is_container)
        self.assertEqual([item.payload for item in value.children], [b"\x09\x6d"] * 3)
        self.assertEqual(value.value, (0x096D, 0x096D, 0x096D))

    def test_truncated_axdr_is_rejected(self):
        with self.assertRaises(protocol.FrameError):
            protocol.axdr_decode(bytes.fromhex("090501"))


if __name__ == "__main__":
    unittest.main()
