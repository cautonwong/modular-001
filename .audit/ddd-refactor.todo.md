# DDD / Clean Architecture refactor — todo

Goal: restructure Blade into `domain/`, `targets/` (composition root), `infra/`
(SOCes/boards/interrupt/RTOSes/HALes), obeying: user alloc of all resources,
aggregate + aggregate root + bounded context, consumer-defined interfaces,
composition root, opaque pointers.

Constraint: no Linux build (MSVC / IAR only). Verification is structural until a
host build or CI exists.

## Branch policy
- work branch: `refactor/ddd-clean-arch` (all tasks land here)
- integration branch: `modular` (work branch merges here)
- `main` is never touched

## Layer decision
DDD/Clean Architecture is four layers. Physical layout:
1. `domain/`      aggregates, aggregate roots, domain services, bounded contexts
2. `application/` use cases
3. `infra/`       SOCes / boards / interrupt / RTOSes / HALes
4. `targets/`     composition root (not a layer; the only place that sees all)

Dependency rule: `targets -> application -> domain <- infra`.
Interfaces are declared by consumers; infra implements them.

## Architect phases
1. Ground — map domain / infra / composition root as they exist today
2. Sketch — at least two structurally distinct layer designs
3. Agree — synthesize; default proceed without checkpoint
4. Implement — scaffold first, then migrate one bounded context end to end
5. Scrap — only if implementation shows a repeated wrong shape

## Ground findings

Infra (from subagent report):
- Allocator lives in `applications/app_resource_tool/resource_tool.c`, not kernel.
  `nd_malloc` zero-fills, `nd_calloc` does not (names swapped). `user_malloc` and
  `power_safe_malloc` are empty stubs, so caller-supplied allocation does not exist yet.
- `create_instance()` in `resource_tool.c:349` is the real composition-root constructor.
- `create_nidavellir(recipe)` is a stub returning NULL.
- `task.c`, `task.h`, `router.c`, `tasklet.c`, `memory.c` are dead or do not compile.
- `cmsis_os2.c` is unreferenced; ThreadX is used directly in `instances/stategrid_meter/os_entry.c`.
- Drivers include `nidavellir.h` and `applications.h`, so infra depends on domain headers today.
- ~30 global fifo/pipeline/context externs in `nidavellir.h:46-111`, marked to-be-deleted.

## Task steps
- [x] Ground: infra/kernel/drivers map
- [ ] Ground: domain packages map
- [x] Ground: applications + instances (composition root) map
- [ ] Ground: allocation + interface/global-state evidence

Composition root (from subagent report):
- `applications.h` declares 70 symbols; kernel umbrella `nidavellir.h:32` includes it.
- `port.h` is a macro DSL injecting fields and resource IDs into the domain aggregate.
- Target concerns spread across `driver_entry.c`, `os_entry.c`, `port.h`,
  `pin_function_def.h`, and v85xxp arch headers.
- `applications/app_698_server/stub.c` signatures already disagree with `applications.h`.
- `msta`, `msta_test`, and `app_meterbase_baremetal` project files reference missing paths.
- No working unit-test project; `nidavellir/tests/build/tests.vcxproj` compiles zero files.
- [ ] Sketch design A and design B
- [ ] Screen against design red flags
- [x] Synthesize target layout + dependency rule + interface contracts (draft in docs/architecture.md)
- [x] Implement scaffold: dirs, allocator port, Unity tests, layer check, ARM check
- [ ] Migrate first bounded context end to end
