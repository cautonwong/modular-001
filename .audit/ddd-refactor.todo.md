# DDD / Clean Architecture refactor — todo

Goal: restructure Blade into `domain/`, `application/`, `infra/`, `targets/`
(composition root), obeying: user alloc of all resources, aggregate + aggregate
root + bounded context, consumer-defined interfaces, composition root, opaque
pointers.

Build: `arm-none-eabi-gcc` for target code, host gcc + Unity for tests.
Persistence must stay byte-compatible with existing meters.

## Branch and worktree
- work branch: `refactor/ddd-clean-arch`
- integration branch: `modular`
- working copy: `/tmp/blade-refactor`
- publishing repo: `https://github.com/cautonwong/modular-001`, `main`
  (push blocked by the auto-mode classifier; operator runs it with the `!` prefix)
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

## Unbuilt legacy files (present in the tree, not in the IAR project)
- domain/metering/legacy: demand.c, "electrical measurement.c", iv/iv_data.c
- domain/tariff/legacy: gs_month_bill.c, gs_step_bill.c
- domain/display/legacy: display.c
- application/meterbase/unbuilt: factory.c, parainit.c, tasklet.c, meterbase.h

## Remaining
- Split `domain/meterbase` into metering, tariff, events, freeze, display,
  load_control and delete the staging entries from the check skip lists.
- Migrate legacy callers onto the new aggregates, then delete forwarder headers
  and `applications.h`.
- Move `nidavellir/include` public headers into their layers.
- Replace `application/apps` and `application/adapters` staging with real
  adapters over the domain.
- Link step for the ARM baseline (object compile is green, linking not yet set
  up).
- [x] Unit 8a: `targets/host` composition root wiring metering, demand, load control,
      tariff, arena, byte store and parameter store + host integration tests
- [ ] Unit 9: delete legacy surfaces (`applications.h`, `port.h` field macros, monolith)

## Domain ground findings
- meterbase is 29 files / 10,858 lines. Cycle: meterbase <-> dl698 <-> dl645
  through headers and calls. `gs_record.h` is a cross-package declaration hub.
- Many domain files are in no MSVC project (energy.c, display.c, relay_control.c,
  touFee.c). IAR VG_DTZY.ewp is the build that actually compiles meterbase.
- `demand_list` is dereferenced but never declared. `iv_data.c:166` calls `mallos`.
  `iv_data.c:234` has a full-width comma.
- crypto has 57 .c files and zero .h, plus a missing `inc/` directory. Not compilable.
- No `free` anywhere in meterbase. Raw malloc only in `demand.c` and `iv_data.c`.

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
