# IDIS single-phase product image

Bare-metal image for the V85xxP / stategrid single-phase board.

```bash
cmake -S targets/idis_ip_meter_freertos/app -B build/idis -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/arm-none-eabi.cmake"
cmake --build build/idis
python3 tools/check_idis_firmware.py build/idis/idis.elf

# Host tests and firmware in one run.
./tools/check-idis-prep.sh /tmp/idis-host /tmp/idis-fw
```

The default build is the 104 single-phase product (`IDIS_PHASES=1`). A future 105 target can reserve the same code with `-DIDIS_PHASES=3`; no 105 board or product is built yet.

The app target adds `infra/boards/stategrid_single_phase` as a subdirectory,
builds the board object library, and links it into `idis.elf`. The board's own
`board.elf` is produced by the same build.

Outputs:

- `build/idis/idis.elf` with debug info
- `build/idis/idis.bin` raw flash image
- `build/idis/idis.map` linker map
- section totals printed after every build

Scope: startup, SoC clock/SysTick/RTC, full board driver set (EEPROM, NOR
flash, RN8209, ESAM, SPI/UART/IIC, relays, GPIO, ADC), all 22 IDIS profiles,
and the complete one-second service loop (average, PQ, events, calendars,
scheduler, single action, monitors, limiter, M-Bus clients, demand, energy,
profiles, push state machine).

Persistence is split by device. Runtime state (clock, energy, demand,
disconnects, PQ, calendars, scripts, push, monitors, limiter, image metadata,
setup, events) lives in the board EEPROM through `board_store_port(state)`. Setup
values are read and written on demand, so the 162 parameters do not occupy
RAM. Profile records and image blocks live in the external NOR flash through
`board_flash_port(state)`; the sector store erases only when a written sector
actually changes.

The interrupt vector table covers all 32 peripheral IRQs and dispatches
through `Isr_handle_regsit()`. Registered callbacks are the PMU key input,
the RTC second tick, and the RS485, IR and module UART channels. SysTick
drives the software timer list used by UART timeouts and relay pulses. The
remaining vector entries and the timer/DMA handlers are present but have no
product consumer. LCD and ESAM are stategrid-meter features and stay out of
this IDIS image.

RN8209 scaling reads the factory calibration from EEPROM offset `0xE400`.
The record is used only when its magic matches and the voltage, current and
power factors are each greater than zero. The record has no checksum field,
so a slot whose magic does not match looks like unwritten media. `0xE400` is
authoritative. The legacy `0xE000` slot is read only while `0xE400` has no
usable record; a valid legacy record is adopted, rewritten to `0xE400` (a
failed rewrite is retried on the next boot), and then never read again
because the old offset belongs to the registry blob. The production tool must
write `0xE400`; a tool that keeps writing `0xE000` corrupts registry entries
255-256 and the firmware cannot prevent it.

Boot reports two cases as COSEM event 14 (NV memory error): a record that
carries the magic but a non-positive factor, and a legacy slot that a saved
registry blob covers. Both keep the compiled default factors in
`board_ports.c`. Blank media, and a legacy slot overwritten by anything other
than a registry blob, is indistinguishable from never-calibrated and uses the
defaults with no event. That is the documented deployment risk of a meter
whose calibration the registry destroyed before this firmware shipped. Factory
calibration parameters beyond the three scale factors are not read; the legacy
`nd_get_index()` resource lookup has been removed.
