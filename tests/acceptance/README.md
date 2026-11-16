# acceptance runner

Acceptance cases run a Linux target process and observe only its contract:
exit code, stdout, trace, EEPROM and flash images.

```sh
cmake --build build --target firmware_sim
python3 tests/acceptance/run.py --sim build/firmware_sim
```

IDIS cases drive the host runner instead:

```sh
cmake --build build/ci-host --target idis_host_runner
python3 tests/acceptance/run.py \
  --runner build/ci-host/idis_host_runner \
  --cases targets/idis_ip_meter/tests/acceptance \
  --select tag:idis
```

Target-specific cases live under `targets/<target>/tests/acceptance/`. The
FreeRTOS target points at the bare-metal IDIS case directory with `CASES`, so
the case list is not copied twice. `tools/ci_matrix.py` runs the cases for both
IDIS targets and writes evidence under `build/acceptance/evidence/<target>/`.

The runner writes `summary.json`, `report.md`, `run-scope.json` and one JSONL
per case under the evidence directory. Selectors use the test-platform grammar:

```sh
python3 tests/acceptance/run.py --sim build/firmware_sim --select tag:persistence
python3 tests/acceptance/run.py --list
```

Exit codes are 0 clean, 1 failed, 2 invalid, 3 incomplete. A missing target
binary, an unknown selector and a selection with no cases are invalid, never
clean.

## Add a case

Create a `test_*.py` module in the target's acceptance directory and register a
stable id with `@case`. Assertions should use `ctx.run(...)` and the run
contract. Do not assert call order or internal fields. A test that needs a new
observation surface should add that surface to the target first; the acceptance
plan tracks those gaps.
