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
- `main` is never touched

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

## Progress
- [x] Ground: infra / kernel / drivers
- [x] Ground: applications + instances (composition root)
- [x] Ground: allocation, aggregates, globals
- [x] Ground: domain packages (energy, demand, measurement, tariff, events)
- [x] Scaffold: allocator port, Unity 2.6.0, CMake + ctest, layer check, ARM check
- [x] Unit 2: `domain/metering` opaque aggregate + energy book + legacy codec + tests
- [x] Unit 3a: `domain/common/calendar` date, time, BCD, legacy day math + tests
- [ ] Unit 3b: `domain/tariff` fee tables, TOU, step, season, electricity price
- [ ] Unit 4: `domain/events` + `domain/freeze`
- [ ] Unit 5: `domain/load_control`
- [ ] Unit 6: `application/adapters` protocol migration
- [ ] Unit 7: `infra` reorganisation + resource provider
- [ ] Unit 8: `targets/stategrid_meter` composition root
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
