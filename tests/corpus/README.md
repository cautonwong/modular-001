# Protocol regression corpus

This directory is the landing zone for field captures. Each vector is
compiled into `tests/unit/test_protocol_corpus.c` and replayed through the
production matcher, so a capture does not depend on a hand-built frame.

Current vectors are protocol regression seeds, not field captures:

- `645 control 0x11`: valid 645 frame with checksum and address.
- `645 bad checksum`: same frame with the checksum byte flipped.
- `698 read request`: valid 698 frame with HCS/FCS.
- `698 bad HCS`: same frame with the header checksum flipped.

When adding a field capture, keep the raw bytes, label the source and date,
and add the expected matcher result. Do not normalize the capture; the
matcher must accept the bytes as recorded.
