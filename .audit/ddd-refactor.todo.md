# DDD / Clean Architecture refactor — todo

Goal: restructure Blade into `domain/`, `application/`, `infra/`, `targets/`
(composition root), obeying: user alloc of all resources, aggregate + aggregate
root + bounded context, consumer-defined interfaces, composition root, opaque
pointers.

Build: `arm-none-eabi-gcc` for target code, host gcc + Unity for tests.
Persistence must stay byte-compatible with existing meters.

## Branch and worktree
- work branch: `main` in `/home/edge/modular-001`
- publishing repo: `https://github.com/cautonwong/modular-001`, `main`
  (the operator requires local commits only; never push)
- `main` is never touched in the source repo

## Layer decision
DDD is four layers:
1. `domain/`      aggregates, aggregate roots, domain services, bounded contexts
2. `application/` use cases and protocol adapters
3. `infra/`       SOCes / boards / interrupt / RTOSes / HALes
4. `targets/`     composition root, the only place that sees all three

Dependency rule: `targets -> application -> domain <- infra`.
Interfaces are declared by consumers; infra implements them.

## Ground findings

Infra:
- Allocator lives in `applications/app_resource_tool/resource_tool.c`.
  `nd_malloc` zero-fills, `nd_calloc` does not (names swapped).
  `user_malloc` and `power_safe_malloc` are empty stubs.
- `create_instance()` is the real composition-root constructor.
- `create_nidavellir(recipe)` is a stub returning NULL.
- `task.c`, `task.h`, `router.c`, `tasklet.c`, `memory.c` are dead or do not compile.
- `cmsis_os2.c` is unreferenced; ThreadX is used directly in `os_entry.c`.
- Drivers include `nidavellir.h` and `applications.h`, so infra depends on domain headers.
- ~30 global fifo/pipeline/context externs in `nidavellir.h:46-111`.

Composition root:
- `applications.h` declares 70 symbols; `nidavellir.h:32` includes it.
- `port.h` is a macro DSL injecting fields and resource IDs into the domain aggregate.
- Target concerns spread across `driver_entry.c`, `os_entry.c`, `port.h`,
  `pin_function_def.h`, and v85xxp arch headers.
- `applications/app_698_server/stub.c` signatures already disagree with `applications.h`.
- `msta`, `msta_test`, and `app_meterbase_baremetal` project files reference missing paths.
- No working unit-test project; `nidavellir/tests/build/tests.vcxproj` compiles zero files.

Cross-cutting:
- Resource ownership is split three ways and has no deallocator.
- `instance_t`, the annotated aggregate root, does not contain fields code uses
  through it (`memory`, `areas_used`, `cache`, `event_flags`, `mbase_cb`).
- `context_dlms_t` is undefined and collides with `hdlc_control_block_t ctx_dlms`.
- Per-meter state lives in file-scope statics in meterbase and drivers.
- `packages/meterbase/iv/iv_data.c:166` calls `mallos`, a compile error.

## Constraints
- Persistence bytes must match existing meters. Legacy layout lives in
  `targets/<product>/persistence/`, never in domain.
- One allocation path: injected allocator, zeroed memory, dealloc with size.
- No global mutable state in domain or application.

## Build baseline
- `tools/legacy_baseline.py` parses the IAR `.ewp`, compiles all 117 sources with
  arm-none-eabi-gcc, and reports failures grouped by first error.
- Result: sources=117 ok=117 failed=0 missing_files=0.
- Fixes that unblocked it: newlib `pid_t` guard, GCC `__no_init` attribute,
  `_STEPPOINT` no-op, vendored CMSIS 5.1.0 `cmsis_gcc.h`, IAR source list
  updated for the moved `infra/kernel` files.
- `packages/kernel/resource.c` is not in the IAR project, so it stays out of the
  baseline; its enums are declared after use upstream.
- `tools/check-legacy.sh` remains a smaller, faster subset check.

## Progress
- [x] Ground: infra / kernel / drivers
- [x] Ground: applications + instances (composition root)
- [x] Ground: allocation, aggregates, globals
- [x] Ground: domain packages (energy, demand, measurement, tariff, events)
- [x] Scaffold: allocator port, Unity 2.6.0, CMake + ctest, layer check, ARM check
- [x] Unit 2: `domain/metering` opaque aggregate + energy book + legacy codec + tests
- [x] Unit 2b: `domain/metering/demand` sliding-window aggregate, no malloc, tests
- [x] Unit 3a: `domain/common/calendar` date, time, BCD, weekday, legacy day math + tests
- [x] Unit 3b: `domain/tariff` interval lookup, holiday, rest day, day table, fee + tests
- [x] Unit 3c: `domain/common/parameter_store` port, infra RAM table, legacy tariff parser + tests
- [ ] Unit 4: `domain/events` + `domain/freeze`
- [x] Unit 5: `domain/load_control` relay command state machine + tests
- [x] Unit 5b: `domain/load_control` feedback, LED blink, misoperation + tests
- [x] Unit 6a: `application/metering` one-second use case; composition root now only wires
- [x] Unit 7a: `infra/memory/arena` implements the allocator port + host composition test
- [x] Unit 7b: `domain/common/store` port, `infra/memory/byte_store`, legacy energy store adapter
- [x] Unit 7c: move `fifo` to `infra/kernel`, drop debug globals, host tests;
      legacy header forwards to the new one until callers migrate
- [x] Unit 7d: move `list` and `bitarithm` to `infra/kernel`; fix the broken
      `list_for_each_safe` macro; legacy headers forward; host tests
- [x] Unit 7e: move the whole `packages/kernel` into `infra/kernel`; update the
      IAR source list and include paths; baseline stays 117/117
- [x] Unit 7f: split `packages/drivers` into `infra/soc` and `infra/board`;
      update IAR paths; baseline stays 117/117
- [x] Unit 7g: move baremetal, esam, crypto, upgrade, boot into their layers;
      update solution and IAR paths; baseline stays 117/117
- [x] Unit 7h: move `dl698`, `dl645`, `dlms` into `application/adapters`
- [x] Unit 7i: move `instances` to `targets`, merging stategrid under the existing
      `targets/stategrid_meter`; tools and IAR paths updated; baseline 117/117
- [x] Unit 7j: move `packages/meterbase` to `domain/meterbase` as a staging area;
      new-code tests and strict checks are narrowed to migrated contexts
- [x] Unit 7k: move `applications` to `application/apps`; update solution, IAR
      and tool paths; baseline stays 117/117
- [x] Unit 7l: move `packages/test` to `tests/legacy` and kernel build files to
      `infra/kernel/build`; the `packages` directory is gone
- [x] Unit 7m: split `domain/meterbase` into `domain/{metering,tariff,events,freeze,
      load_control,display}/legacy`; dead app files to `application/meterbase/unbuilt`
- [ ] Unit 4: `domain/events` + `domain/freeze` clean rewrites (legacy moved, not rewritten)
- [x] Unit 4a: `domain/events/duration` debounce state machine + 6 tests
- [x] Unit 4b: `domain/events/{over_current,cover_open,power_off}` + 12 tests
- [x] Unit 4c: legacy over-current, cover-open and power-off call sites now use the
      new modules; IAR project compiles 121 sources
- [x] Unit 4d: emu error, relay error, RTC error, power abnormal and neutral current
      migrated too; `event_duration_pro` removed; IAR project compiles 123 sources
- [x] Unit 4e: `domain/freeze/schedule` periodic, month, bill and instant triggers
      + 13 tests; legacy judges now call them; IAR project compiles 124 sources
- [x] Unit 4f: `domain/freeze/backfill` day and settlement catch-up + 12 tests and
      `nd_calendar_from_total_day`; legacy power-on handlers now call them;
      IAR project compiles 125 sources
- [x] Unit 4g: `nd_event_duration_start` for the power-down path; power-off and
      power-abnormal low-power handlers no longer set active flags themselves
- [x] Unit 6a: `legacy_baseline.py` now relocatable-links the whole IAR object set
      and rejects internal undefined symbols; it found `domain/common/calendar.c`
      missing from the .ewp, which is now added. 127 sources, 0 undefined.
- [x] Unit 4h: `domain/freeze/sequence` rollback and recovery rules + 6 tests;
      `gs_freeze.c` sequence paths now call them
- [x] Unit 3d: tariff lookup migrated to `nd_tariff_table_t`;
      `targets/stategrid_meter/tariff_lookup.c` selects through the domain,
      `tariff_lookup_eeprom.c` supplies the EEPROM parameter store;
      `GetFeeTab`/`GetTOUFee` now call it and `FeeTable_CmpFind` is deleted;
      4 adapter tests; IAR project compiles 131 sources
- [x] Unit 5c: load control migrated to `load_control_state_t`;
      `targets/stategrid_meter/relay_control_adapter.c` bridges the legacy
      control block, the relay outputs, LEDs, events and 645 reports;
      `do_relay`/`relay_led_ctl` now call it and `Relay_Sec_Task` is deleted;
      domain gained the transition flag, limit getter and relay-state getter;
      IAR project compiles 134 sources
- [x] Unit 2c: metering energy migrated to `metering_state_t`;
      `targets/stategrid_meter/metering_energy_adapter.c` imports and exports
      the legacy arrays, and `Energy_Meter`, power-off, clear, set and the two
      int64 getters now use the aggregate. The book now carries pending
      fractions and pending month fractions on zero-pulse calls, and the
      combined read sums raw components before converting, matching legacy.
      `metering_configure` keeps meter constant, cal word and save step live.
      IAR project compiles 137 sources.
- [x] Unit 9a: Dead-file audit. All ten candidates are outside the IAR project
      and have no compiled include or symbol reference. Deleted seven pure
      duplicates or unused files; kept demand.c and the two bill files because
      they hold behavior the new contexts do not model yet. Removed the
      `application/meterbase` staging entries from both check skip lists.
- [x] Unit 2d: `application/metering/demand_registry` owns a fixed set of
      demand registers with their power pointers and output OADs, replacing the
      legacy linked-list wiring; 4 tests. The kept legacy `demand.c` is now
      superseded at the application layer.
- [x] Unit 9b: `applications.h` split. 35 prototypes moved to six context-owned
      headers under the metering, tariff, load control, freeze, events and
      display contexts. The monolith `mbase_control_block_t` moved to
      `nidavellir/include/kernel/legacy_control_block.h`. Umbrella 708 -> 282
      lines. `tools/legacy_probe.c` offsets are unchanged.
- [x] Unit 9c: Kernel decoupling. `nidavellir.h` no longer includes
      `applications.h`. Resource block ids, device enums, `Relay_Ck_Time` and
      `keydlymax` moved to `infra/kernel/resource_blocks.h`,
      `infra/board/relay_config.h` and `infra/kernel/key_config.h`. 44
      application/target callers now include `applications.h` explicitly.
      137/137 compile and link.
- [x] Unit 9d: Record group moved out of `applications.h` into
      `domain/events/legacy/gs_record.h`: the three record structs, two enums
      and 13 record prototypes. `gs_record.h` is now self-contained with
      stdint and forward declarations. Umbrella 282 -> 213 lines.
- [x] Unit 9e: `applications.h` deleted. 77 includers rewritten to specific
      owner headers: app config/entry, driver entry, mapper_698 resource map,
      fr_698_esam, v85xxp system, devices, ctl_other. `pin_function_def.h`
      made self-contained for `BIT`. `check_layers.py` now stages
      `application/adapters` and `application/upgrade`. 137/137 compile and
      link, probe offsets unchanged.
- [x] Unit 10a: dl698 first decoupling. `fr_698_comm.c` uses the generic
      `nd_get`/`nd_set`/`nd_action` map accessors instead of the target
      `resource_map_698`, so the target include is gone. The hardware ESAM
      implementation moved from `application/adapters/dl698/fr_698_esam.c` to
      `infra/security/esam/fr_698_esam.c`; the application header stays as the
      consumer-declared interface. IAR project updated, 137/137 link clean.
- [x] Unit 10b: dl698 time and security parameters. `dl698_time` reimplements
      the date checks, interval conversion and epoch conversion on the calendar
      domain, fixing wildcard days, leap days, 24/60/60 field acceptance and the
      unit-0 multiplier; 6 tests. The parameter port carries the explicit
      security mode and security enable reads, bound at startup by
      `dl698_parameter_adapter.c`. The unreachable frozen-period read is gone.
      140/140 compile and link.
- [x] Unit 10c: dl698 report parameters. Follow-report, report channel, active
      report and the four 3320 lists all read and write through the parameter
      port. `get_3320_save_addr` is deleted; `get_3_Event.c` uses the shared
      list accessor.
- [x] Unit 10d: report list model. `report_list.c` models the legacy layout as
      a 60-entry fixed array with parse, encode, find, remove and insert-front.
      The update, latest-follow, confirmation and active-filter functions are
      rewritten on it, so no pointer compaction of the persistence buffer is
      left. 7 tests, 33/33 host tests pass.
- [x] Unit 10e: monolith accessor. `meter_base_view.h` exposes typed pointer
      accessors for every dl698-used field, and `meter_base_adapter.c` binds
      the legacy block and owns the two persistent slice saves. The four dl698
      files no longer include `kernel/legacy_control_block.h` or touch
      `mbase_cb->`/`ctrl_block->` directly. 142/142 compile and link, probe
      offsets unchanged.
- [x] Unit 11a: portability seams. `application/ports/platform` owns the
      watchdog and event-output port, implemented by
      `targets/stategrid_meter/platform_adapter.c` and bound at startup.
      `domain/common/status.h` holds the platform-neutral power and event
      values. dl698 has no direct target or SoC includes left. The top-level
      design for multiple SoC, board, RTOS and domain variants is in
      `docs/portability.md`. 144/144 compile and link, 34/34 host tests.
- [x] Unit 11b: `nidavellir.h` split. The 783-line umbrella is now an
      11-line compatibility wrapper over `kernel_core.h`. Instance, message,
      object, channel, pid and block-id definitions moved to focused kernel
      headers; `context_698_t` moved to `fr_698_context.h` and
      `context_645_t` moved to `fr_645_context.h`. dl698 and the protocol
      adapters no longer include the umbrella. Every new header compiles
      standalone, and `instance_t`/`message_t` layouts match the old header
      byte for byte. 144/144 compile and link, 34/34 host tests.
- [x] Unit 11c: `kernel_core.h` split. The 450-line core is now a 62-line
      compatibility bundle over tasklet, rtc, log object, device, environment,
      ipc globals, serial, work-type, resource-map and flag headers. The packed
      `recipe_t` layout is preserved. `instance_t`, `message_t`, `tasklet_t`,
      `rtc_context_t`, `dev_info_t`, `recipe_t` and `irp_t` sizes and key
      offsets match the original header. 144/144 compile and link, 34/34 host
      tests.
- [x] Unit 12a: monorepo architecture. `docs/monorepo.md` defines the
      component model, product matrix (electricity, water, gas, PLC and RF
      modules), protocol convergence, and the replacement of
      pipeline/filter/message with a typed event bus, channel ports and direct
      use-case interfaces. The existing domain modules are recorded as
      component candidates, not finished components.
- [x] Unit 12b: bus and channel components. `nd_bus` is a fixed-topic,
      fixed-subscriber, synchronous event bus with no allocation; `nd_channel`
      is a non-blocking read/write/flush byte port. Both have component
      metadata, host tests, a `blade_components` CMake target and an IAR
      project group. 146/146 compile and link, 36/36 host tests.
- [x] Unit 12c: component manifests, products and wiring. Domain, application
      and infra modules have component manifests; `products/` describes the
      single-phase meter, water, gas, PLC and RF assemblies;
      `tools/check_components.py` enforces metadata and include direction and
      runs from `check-legacy.sh`. The kernel tick path publishes second and
      power events, and `infra/channel/fifo_channel` bridges legacy FIFOs to
      `nd_channel`. 147/147 compile and link, 37/37 host tests.
- [x] Unit 12d: dl698 component migration. `protocol.dl698` owns the three
      698 channel contexts through `nd_channel`. FIFO-backed channels are
      accessed zero-copy, so no new RAM is allocated. The three rx pipelines
      are out of the recipe; the tx pipelines keep only their message FIFO and
      are skipped by the kernel loop. Received frames publish
      `ND_EVENT_DL698_FRAME` on the bus. 148/148 compile and link, 37/37 host
      tests.
- [x] Unit 12e: dl698 message_t removal. `fr_698_message.h` defines the
      protocol-local `nd_dl698_msg_t`; every dl698 entry, handler and callback
      uses it. `send_mesg`, `recv_mesg_v2` and `message_t` no longer appear in
      the 698 adapter. Active reports call `nd_dl698_send` through
      `context_698_t.send`; the tx pipelines are out of the recipe and their
      input FIFOs are no longer allocated. 148/148 compile and link, 37/37 host
      tests.
- [x] Unit 12f: dl645 migration. `fr_645_message.h` defines
      `nd_dl645_msg_t` with the old message header layout, so master cache
      offsets are unchanged. The 645 adapter has no `message_t`, `send_mesg`
      or `recv_mesg`; master forwarding uses `context_645_t.forward`.
      `protocol.dl645` consumes channel-backed FIFOs zero-copy and publishes
      `ND_EVENT_DL645_FRAME`. It is not wired into the single-phase product,
      which has no 645 context. 149/149 compile and link, 37/37 host tests.
- [x] Unit 12g: dlms cleanup and buffer decision. `fr_dlms_message.h`
      defines a local `nd_dlms_msg_t`; the DLMS sources no longer use
      `message_t`. Include paths, internal declarations and the
      `fifoBuf_putByte` call are fixed, and the stale duplicate
      `application_context_name.c` is deleted. DLMS is still not in a product
      build. `docs/buffer-strategy.md` records the decision not to use lwIP
      pbuf as the internal protocol buffer.
- [x] Unit 12h: layered buffer component. `components/core/buf` defines
      `nd_frame`, `nd_block` and `nd_cursor` for link-frame chains and
      application blocks. Cursors read and skip across frames without
      linearizing; `nd_block_copy` is the explicit contiguous path. 7 host
      tests. The strategy document now distinguishes transport, link and
      application buffers and keeps pbuf as an optional IP-side backend.
      150/150 compile and link, 38/38 host tests.
- [x] Unit 12i: 698 segmentation on `nd_block`. `context_698_t` now owns
      `rx_block` and six `nd_frame` slots. First, middle and last segments
      append through `nd_block_append`; last-segment rewind truncates through
      `nd_block_truncate`; `apdu_rx_len` mirrors the block total. Frames point
      into the existing `apdu_d_rx` storage, so no new slice buffers are
      allocated. The block is still a chain over a contiguous APDU view; the
      next step is separate RX buffers plus cursor-based parsing.
      150/150 compile and link, 38/38 host tests.
- [x] Unit 12j: infra meter status port. `infra/board/meter_status.h`
      declares the meter type, metering-chip error state and ESAM power state
      that board and security code consume. `targets/stategrid_meter/meter_status_adapter.c`
      implements it over the legacy control block. `ndv_RN8209.c`, `esam_pro.c`
      and `nidavellir.c` no longer include `kernel/legacy_control_block.h` or
      touch `mbase_cb` for these fields. 151/151 compile and link, 38/38 host
      tests.
- [x] Unit 12k: ESAM status port. `infra/security/esam/esam_status.h`
      declares chip abnormal, key state, auth-on, meter number update and the
      three auth timeouts. `targets/stategrid_meter/esam_status_adapter.c`
      implements them over the legacy block and keeps the meter number save at
      the old static EEPROM slice. `fr_698_esam.c` no longer includes
      `kernel/legacy_control_block.h` or touches `mbase_cb`; ESAM power and
      close delay use the meter status port. 152/152 compile and link, 38/38
      host tests.
- [x] Unit 12l: infra target-header cleanup. Deleted the Windows-only
      `infra/board/esam698_win.c`, which was in no project. Added
      `infra/board/meter_config.h` for the meter type, calibration and heat
      compensation parameter ids. `ndv_RN8209.c` and `nidavellir.c` stopped
      including the target `port.h`; `fr_698_esam.c` stopped including
      `cpu_surport.h` and uses `ND_AC_POWER_ON`. No infra file includes
      `port.h` now. 152/152 compile and link, 38/38 host tests.
- [x] Unit 12m: pin header move. `targets/stategrid_meter/pin_function_def.h`
      now lives at `infra/board/stategrid_meter/pin_function_def.h`. The 23
      consumers and both IAR project files use the new path. Content is
      unchanged; the next step is splitting the GPIO macros into semantic board
      ports. 152/152 compile and link, 38/38 host tests.
- [x] Unit 12n: semantic board port. `infra/board/stategrid_meter/board_io.{h,c}`
      wraps backlight, PLC event output, relay LED and feedback, cover input,
      key input and RS485 receiver enable, plus ADC and UART channel ids.
      Application, domain legacy, target, SoC ISR and ESAM security consumers
      now call those functions; `fr_645_report.c` uses the application platform
      port for PLC events. Only board drivers and `board_io.c` include the pin
      header. 153/153 compile and link, 38/38 host tests.
- [x] Unit 12o: board driver pin closure. `board_io` now owns EEPROM power and
      IIC lines, SPI flash power and bus lines, RS485 direction, relay coils,
      RN8209 reset and TX lines, and ESAM chip select and power. The IIC, SPI,
      NOR flash, UART, relay, RN8209 and ESAM board drivers call those
      functions. Only `board_io.c` includes `pin_function_def.h`; no other
      source uses the raw pin macros. 153/153 compile and link, 38/38 host
      tests.
- [x] Unit 12p: pin header folded. Deleted
      `infra/board/stategrid_meter/pin_function_def.h` and moved its register
      access into `board_io.c` as bit-band, set/clear and bit read helpers plus
      explicit pin numbers per peripheral. The IAR header entries are gone.
      `board_io.c` is now the only source that touches GPIO registers.
      153/153 compile and link, 38/38 host tests.
- [x] Unit 12q: first consumer ports. `domain/display/backlight` exposes
      `nd_backlight_set`; `domain/load_control/relay_status` exposes
      `nd_relay_status_is_off`. Display and runstatus code no longer include
      `board_io.h`. The stategrid target binds both ports to `board_io` before
      `dev_init`, and host tests cover bound and unbound dispatch.
      157/157 compile and link, 40/40 host tests.
- [x] Unit 12r: metering input port. `domain/metering/metering_input` exposes
      temperature and battery reads with the old raw ADC values. The stategrid
      adapter maps them to the board ADC channels and is bound in the
      composition root. `electrical_measurement.c` no longer includes board_io
      or calls the MCU ADC functions. 159/159 compile and link, 41/41 host
      tests.
- [x] Unit 12s: private pin modules. `board_io.h` now exposes only semantic
      operations. IIC, SPI, UART/RS485, RN8209, relay coil and ESAM line
      operations live in private `*_pins.h` headers with their own source files.
      ADC channel ids moved to `meter_config.h`; UART index ids moved to
      `uart_driver.h`. 165/165 compile and link, 41/41 host tests.
- [x] Unit 12t: platform boundary. Moved `infra/board` to
      `targets/stategrid_meter/board`. Moved the board-dependent nidavellir,
      time_link, SoC ISR and ESAM sources into target `rtos`, `soc` and
      `security` directories. Added `tools/check_platform_boundary.py`, wired
      into `check-legacy.sh`; non-staging platform includes and RTOS calls now
      fail. Platform boundary staging is now zero.
      165/165 compile and link, 41/41 host tests.
- [x] Unit 12u: application/apps platform cleanup. Moved the product entry and
      display files to `targets/stategrid_meter`, moved app_msta to
      `targets/msta/apps`, and moved the app test cases to `tests/legacy`.
      application/apps no longer includes platform headers or calls RTOS APIs.
      Staging debt dropped from 116 to 18 entries, all in the dl645 adapter and
      domain legacy. 165/165 compile and link, 41/41 host tests.
- [x] Unit 12v: platform boundary staging cleared. Added domain runtime and
      timer ports, a record-message port, and target adapters for each. Moved
      electrical_measurement to target. Replaced platform event and power
      macros with domain constants, and removed the remaining dl645, event,
      freeze and tariff platform includes. The platform boundary checker
      reports zero staging and zero non-staging violations. 170/170 compile and
      link, 43/43 host tests.
- [x] Unit 12w: kernel-header boundary ratchet. Extended the platform
      boundary checker to `nidavellir.h`, `kernel/*` and `nidavellir/*`.
      Recorded the current 41 legacy kernel-header dependencies in
      `tools/platform_boundary_baseline.txt`; new violations fail. Debt is
      concentrated in domain/events legacy, dl698/dl645 adapters and the
      remaining application apps. 170/170 compile and link, 43/43 host tests.
- [x] Unit 12y: dl645 adapter decoupled. Added `fr_645_env` accessors for the
      legacy control block, APDU cache, password index and pipeline view, with
      a target adapter implementation. Replaced the context FIFO with a private
      `fr_645_fifo_t`. The dl645 adapter has no kernel/platform include left.
      Kernel baseline drops from 55 to 41 entries. 171/171 compile and link,
      43/43 host tests.
- [x] Unit 12z: dl698 adapter decoupled. Added `fr_698_env` accessors and
      target adapter for time, APDU cache, power/reset state, power-off counter
      and pipeline view. Localized dl698 pids and removed the component's direct
      instance field access via a `now_ms` config callback. The dl698 adapter
      and component have no kernel/instance/pids include left. Kernel baseline
      drops from 51 to 41 entries. 172/172 compile and link, 43/43 host tests.
- [x] Unit 12aa: domain/events/legacy cleared. The 11 event and record
      implementation files moved to `targets/stategrid_meter/events/legacy`
      because they directly use the monolith block, EEPROM record addresses and
      kernel messages. `domain/events/legacy` keeps only the two interface
      headers and is no longer in STAGING or the ARM skip list. Kernel baseline
      drops from 41 to 19 entries. 172/172 compile and link, 43/43 host tests.
- [x] Unit 12ab: remaining kernel baseline cleared. Moved the display, freeze,
      load-control, metering and tariff legacy implementations to
      `targets/stategrid_meter/*/legacy`. Gave the dlms files a local pipeline
      view header and the dl645 component a local pipeline view plus a `now_ms`
      callback. Domain legacy directories now hold only interface headers and
      leave STAGING and the ARM skip list. Kernel baseline drops from 19 to 0.
      172/172 compile and link, 43/43 host tests.
- [x] Unit 12ac: report and security policies extracted. `domain/report`
      owns report flag matching, channel enable parsing, 2015 masking, 3011
      active-report eligibility and 3320 attribute variants. `domain/security`
      owns the exact/wildcard security-mode tables, the permission decision and
      the plain-read OAD allowlists. The adapters keep protocol framing and
      instance access. Ten host tests cover the extracted rules. 174/174 compile
      and link, 45/45 host tests.
- [x] Unit 12ad: report retry and framing split. `domain/report/report_retry`
      owns command bits, retry delays and counts, PIID, and power-on/off
      eligibility. `fr_report_frames.c` owns the 3011 and 3320/2015 APDU frame
      construction. `fr_report.c` keeps list maintenance, response handling and
      orchestration. Eight host tests cover the retry decisions and updates.
      176/176 compile and link, 46/46 host tests.
- [x] Unit 12ae: ESAM RN/MAC flow split. `fr_698_esam_flow.c` now owns the
      SID_MAC, RN, RN_MAC and SID handlers, SID parsing and ESAM result mapping.
      `fr_698_security.c` keeps security parameter parsing, request permission
      checks and the encode entry. 177/177 compile and link, 46/46 host tests.
- [x] Unit 12af: ESAM input and result split. `fr_698_esam_input.c` owns the
      common input setup, SID/octet parsing, temporary buffer binding and RN
      copy. `domain/security/esam_result` owns ESAM status and verification
      result mapping, with an explicit MAC enforcement flag. The adapter keeps
      enforcement off to preserve legacy behavior. 179/179 compile and link,
      47/47 host tests.
- [x] Unit 12ag: report frame encoding split. `report_encoder.c` owns the
      3011 header/RCSD list, normal header, 3320 array item, 2015 bit-string
      item, length backfill and bounds checks. `fr_report_frames.c` keeps data
      collection, ESAM tails and cache copies. Four encoder tests. 180/180
      compile and link, 48/48 host tests.
- [x] Unit 12ah: ESAM MAC enforcement enabled. The verification handler now
      compares MACs only when both input and output MAC buffers are present and
      long enough, and passes `enforce_mac = 1` to the result mapper. Plain RN
      requests without a MAC remain accepted. 180/180 compile and link, 48/48
      host tests.
- [x] Unit 12ai: ESAM host compatibility validation. Added an optional-MAC
      matcher that accepts a plain RN with no MAC but rejects missing, short or
      mismatched MACs. Recorded the host matrix and the remaining hardware
      checks in `docs/esam-compatibility.md`. 48/48 host tests.
- [x] Unit 12aj: ESAM RN/MAC tail encoding. `nd_report_encoder_rn_mac_tail`
      emits the 0x02/0x0c marker, random bytes, 0x04 and the four ESAM MAC
      bytes, or FF fill on ESAM failure. Both 3011 and normal reports use it.
      Three tail tests added. 180/180 compile and link, 48/48 host tests.
- [x] Unit 12ak: 698 request permission split. `domain/security/security_request`
      owns application-link, auth, infrared, terminal-timeout, broadcast/group and
      broadcast-time rules. The adapter only gathers request state and maps
      results. Seven permission tests. 181/181 compile and link, 49/49 host tests.
- [x] Unit 12al: 698 security encode split. `fr_698_esam_encode.c` owns the
      ESAM call and secure response tail assembly. `security_encoder.c` owns the
      01/00/04/MAC or 00 tail encoding with bounds checks. Three encoder tests.
      183/183 compile and link, 50/50 host tests.
- [x] Unit 12am: fr_698 frame split. `fr_698_frame.c` owns header/tail packing,
      the matcher, timetag, address handling, APDU checks and length checks.
      `fr_698_internal.h` holds adapter-internal prototypes. `fr_698.c` keeps
      segmentation, send, gatekeeper and entry points. 184/184 compile and link,
      50/50 host tests.
- [x] Unit 12an: gatekeeper and segmentation split. `fr_698_segment.c` owns
      receive block assembly, segmentation, scrambler and segment transmission.
      `fr_698_gatekeeper.c` owns service checks, context reset and server/client
      gatekeepers. `fr_698.c` is down to send handling and the two entries.
      186/186 compile and link, 50/50 host tests.
- [x] Unit 12ao: transmit state split. `fr_698_tx.c` owns send segmentation,
      connection parameter selection, frame packing calls and the transmit
      entry. `fr_698.c` keeps only the receive entry. 187/187 compile and link,
      50/50 host tests.
- [x] Unit 12ap: client forwarding and protocol state split. `fr_698_client.c`
      owns the client gatekeeper with its dead `#if 0` block removed.
      `fr_698_state.c` owns context reset and channel initialization.
      `fr_698_gatekeeper.c` is down to 284 lines. 189/189 compile and link,
      50/50 host tests.
- [x] Unit 12aq: client forward callback. `context_698_t` gains a nullable
      `forward` callback. The client gatekeeper copies the remaining APDU into
      `apdu_d_rx`, fills the protocol message and invokes it; unbound contexts
      keep the old drop behavior. No target binding yet. 189/189 compile and
      link, 50/50 host tests.
- [x] Unit 12ar: DLMS moved to a protocol component. The files live under
      `components/protocol/dlms` with a local `nx_dlms.h`, so the component no
      longer takes its structure definitions from `kernel/object.h`. It remains
      legacy and outside the product build. Staging is now limited to dl645 and
      dl698. 189/189 compile and link, 50/50 host tests.
- [x] Unit 12as: dl645/dl698 layer migration. Added an application-owned FIFO
      interface, moved the 645 ID table header to the adapter, and removed the
      remaining kernel/target includes. STAGING is empty. 189/189 compile and
      link, 50/50 host tests. ARM still skips adapters because the legacy
      headers are not self-contained.
- [x] Unit 12at: protocol matcher host tests. Extracted `fr_645_matcher` and
      `fr_698_matcher`, added valid-frame tests plus bad 645 checksum and bad
      698 HCS cases. 191/191 compile and link, 52/52 host tests.
- [x] Unit 12au: 698 frame packing tests. Extracted `fr_698_pack`, added
      host coverage for the frame header, APDU payload and tail. 192/192
      compile and link, 53/53 host tests.
- [x] Unit 12av: 645 frame packing tests. Extracted `fr_645_pack`, added
      host coverage for the address/control/length/checksum/tail and APDU
      scrambling. 193/193 compile and link, 54/54 host tests.
- [x] Unit 12aw: AXDR codec tests. Extracted `axdr_decode`, `axdr_len_encode`
      and `axdr_get_len` into `fr_axdr_codec`, with host tests for length forms,
      encode boundaries and TLV sizes. 194/194 compile and link, 55/55 host
      tests.
- [x] Unit 12ax: 645 data field parsing tests. Added `fr_645_parse_91_data`
      and used it in the 91 decode path; host tests cover ID byte order, payload
      offset and length. 195/195 compile and link, 56/56 host tests.
- [x] Unit 12ay: adapters under ARM check. `check-arm.sh` now compiles the
      adapter sources and headers for Cortex-M0 with permissive legacy flags
      while domain remains strict. 195/195 compile and link, 56/56 host tests.
- [x] Unit 12az: adapter warnings tightened. Removed `-w`, `-fpermissive` and
      forced includes; adapters now compile under the normal strict flags with
      only selected legacy warning suppressions. `fr_axdr.h` is self-contained.
      195/195 compile and link, 56/56 host tests.
- [x] Handoff document written at `docs/HANDOFF.md` for the next session

## Unbuilt legacy files (present in the tree, not in the IAR project)
- domain/metering/legacy: demand.c (kept by user decision; demand is not wired
  in the current firmware, get_1_Demand returns OBJECT_NOT_EXIST)
- domain/tariff/legacy: gs_month_bill.c, gs_step_bill.c (kept by user
  decision; no billing handler in the current firmware)

## Portability design record
- `docs/device-model.md` records the build-time board and RTOS selection. The
  current product line assumes one board per product, so there is no runtime
  board vtable and no global board binding. The assumption is not permanent;
  shared communication boards and board revisions keep the board implementation
  reusable but still selected per target. The composition root calls board
  functions directly. Zephyr mapping uses `gpio_dt_spec` and Zephyr devices
  inside the board implementation; domain/application never see Zephyr or
  devicetree headers.
- Vendor SoC HAL and RTOS code may stay unchanged under `infra/soc` and
  `infra/rtos`; the product target includes and calls them directly to
  implement consumer ports. `tools/check_platform_boundary.py` allows platform
  implementation directories and fails shared code that reaches into them.

## Remaining
- Replace `application/apps`, `application/adapters` and `application/upgrade`
  staging with real adapters over the domain. They still include infra/target
  headers directly; the layer check stages those directories for now.
- Migrate the remaining application apps off the legacy `nidavellir.h`
  compatibility wrapper and onto the focused kernel headers.
- Reduce `kernel_core.h` to an explicit include list used only by legacy code,
  or remove it once the last app includes the focused headers directly.
- Keep the semantic `board_io` API small. Add consumer ports only when an
  application or domain module needs a capability; kernel and target code can
  call `board_io` directly.
- Do not add a generic RTOS adapter. A product that changes RTOS copies its
  target and build configuration and changes the concrete dependency. The
  copying stays cheap only while domain/application keep Zephyr and other
  RTOS headers out.
- Add a fake board implementation for host tests. A fake RTOS is not needed
  while domain and application stay scheduler-free.
- Rewrite the remaining local implementations in `fr_report.c` and
  `fr_698_security.c` against domain models, keeping the persisted byte layout
  unchanged and adding host tests for each extracted rule.
- When demand or billing is scheduled, port the legacy behavior into
  `domain/metering/demand` and `domain/tariff` before enabling it.
- [x] Unit 8a: `targets/host` composition root wiring metering, demand, load control,
      tariff, arena, byte store and parameter store + host integration tests
- [ ] Unit 9: delete legacy surfaces (`applications.h`, `port.h` field macros, monolith)

## Unit 13 record catalog
- [x] Moved record/freeze ids to `domain/events/event_id.h`; `targets/stategrid_meter/port.h`
      includes it instead of defining the enum and freeze macros.
- [x] `domain/events/event_catalog.c` maps every handled event id to its record OAD and
      mode (freeze 0-7, start 10, end 11). `recv_record_process` now calls the lookup and
      keeps only storage and reporting glue.
- [x] 5 host tests cover event pairs, single events, table groups, freeze groups and
      unmapped ids. `ctest` 72/72, baseline 205/205, ARM/layers/components clean.
- `docs/ROADMAP.md` records the next phases: narrow `instance_t`, detach the metering
  port from `mbase`, product-manifest build, then hardware validation. `mbase` split
  remains deferred by user decision.

## Domain ground findings
- meterbase is 29 files / 10,858 lines. Cycle: meterbase <-> dl698 <-> dl645
  through headers and calls. `gs_record.h` is a cross-package declaration hub.
- Many domain files are in no MSVC project (energy.c, display.c, relay_control.c,
  touFee.c). IAR VG_DTZY.ewp is the build that actually compiles meterbase.
- `demand_list` is dereferenced but never declared. `demand.c` is the only
  remaining raw-malloc user in the metering staging area.
- crypto has 57 .c files and zero .h, plus a missing `inc/` directory. Not compilable.

## Unit 2 record
- `domain/metering/energy.{h,c}`: pure energy book. No allocation, no globals.
- `targets/stategrid_meter/persistence/legacy_energy_record.h`: frozen 172-byte
  slice with `_Static_assert` offsets measured from the original headers.
- `legacy_energy_codec.c`: maps book fields to the legacy record. Checksums and
  the reserved padding word are left to the persistence layer.
- Tests: 12 energy cases, 4 codec cases. `ctest` 4/4.
- Legacy offsets measured by `tools/legacy_probe.c`:
  active_p 0, active_n 52, active_p_dec 104, active_n_dec 130, energy_checksum
  156, month_energy_dec 160, month_energy 164, use_energy_checksum 168, size 172.
  Inside the persistent block, active_p starts at byte 3.

## P1-P3 execution record
- [x] P1.1/P1.2: opaque `nd_energy_ledger_t`, instance/mbase boundary checker.
- [x] P2.1: product dependency closure, source generator, firmware link filtered by product.
- [x] P2.2: 52 component manifests, no target catch-all, all components check includes/layers.
- [x] P2.3: FIFO channel port; adapters do not include target. 698 client forward remains unbound (no consumer).
- [x] P3.3: `tools/ci.sh` and GitHub workflow.
- [ ] P3.1: hardware ESAM/report/power-cycle regression.
- [ ] P3.2: field captures for the corpus (harness exists).
- [ ] P3.4: validated GNU runtime/startup and board initialization.
