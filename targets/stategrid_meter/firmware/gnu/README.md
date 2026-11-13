# GNU link-check scaffold

`startup.c`, `link_stubs.c` and `link.ld` are used by
`tools/check-firmware-link.py` to turn the IAR source list into a final ELF.

This is a link check, not a validated firmware runtime:

- the vector table contains only the reset entry and a default handler;
- `__CALL` and the CSTACK symbols are link stubs;
- the memory map comes from `V85XX_flash.icf` (256K flash, 32K RAM);
- no hardware initialization, IAR runtime or bootloader handoff is validated.

The IAR project remains the authoritative firmware build until the GNU
startup and board initialization are validated on hardware.
