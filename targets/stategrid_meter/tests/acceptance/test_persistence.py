from __future__ import annotations

from cases import case

# The first boot event record: type 0x06, boot sequence 0xFFFFFFFF, then the
# 2021-04-30 timestamp. Later boots append ten-byte records back to back.
EEPROM_BOOT_EVENT_OFFSET = 0x9F66
EEPROM_BOOT_EVENT_SIZE = 10


def boot_event(image: bytes, index: int) -> bytes:
    start = EEPROM_BOOT_EVENT_OFFSET + index * EEPROM_BOOT_EVENT_SIZE
    return image[start : start + EEPROM_BOOT_EVENT_SIZE]


@case(
    "ACC-PERSIST-001",
    "restarts keep the event log, append a boot record and advance the reset counter",
    ("persistence", "boot"),
)
def restart_preserves_state(ctx):
    first = ctx.run(cycles=5)
    assert first.exit_code == 0, f"first boot exited {first.exit_code}"
    assert first.reset_count == 0, f"first boot reset counter is {first.reset_count}"
    first_non_ff = first.non_ff_eeprom()
    first_event = boot_event(first.eeprom, 0)
    assert first_event == bytes.fromhex("06ffffffff1c281f5644"), first_event.hex()

    second = ctx.run(cycles=5, seed_eeprom=first.eeprom, seed_flash=first.flash)
    assert second.exit_code == 0, f"second boot exited {second.exit_code}"
    assert second.reset_count == 1, f"second boot reset counter is {second.reset_count}"
    assert len(second.eeprom) == len(first.eeprom)
    assert second.non_ff_eeprom() > first_non_ff, "the restart persisted no new bytes"
    assert boot_event(second.eeprom, 0) == first_event, (
        "the second boot rewrote the first boot event"
    )
    assert boot_event(second.eeprom, 1) == bytes.fromhex("06000000001c281f5644"), (
        boot_event(second.eeprom, 1).hex()
    )

    third = ctx.run(cycles=5, seed_eeprom=second.eeprom, seed_flash=second.flash)
    assert third.exit_code == 0, f"third boot exited {third.exit_code}"
    assert third.reset_count == 2, f"third boot reset counter is {third.reset_count}"
    assert boot_event(third.eeprom, 2) == bytes.fromhex("06000000011c281f5644"), (
        boot_event(third.eeprom, 2).hex()
    )

    # The two parameter copies occupy the first 2 KiB. A normal boot must not
    # rewrite them; later blocks (instant-freeze sequence and reset counter)
    # are expected to change.
    assert (
        second.eeprom[:2048] == first.eeprom[:2048]
    ), "boot rewrote parameter storage before the mutable blocks"
    assert second.flash == first.flash, "a five-cycle run wrote flash unexpectedly"
