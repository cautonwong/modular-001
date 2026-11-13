# firmware_sim acceptance

Acceptance cases run the Linux whole-firmware twin as an external process and
observe only its contract: exit code, trace, EEPROM and flash images.

```sh
cmake --build build --target firmware_sim
python3 tests/acceptance/run.py --sim build/firmware_sim
```

Target-specific IDIS cases live in `targets/idis_ip_meter/tests/acceptance/` and are run by `ci.sh`, not by the global CTest list.

The runner writes `summary.json`, `report.md`, `run-scope.json` and one JSONL
per case under `build/acceptance/evidence/<UTC-stamp>/`. Each run keeps its
images under `build/acceptance/work/<case-id>/run-NN/`.

Selectors use the test-platform grammar:

```sh
python3 tests/acceptance/run.py --sim build/firmware_sim --select tag:persistence
python3 tests/acceptance/run.py --list
```

Exit codes are 0 clean, 1 failed, 2 invalid, 3 incomplete. A missing simulator,
an unknown selector and a selection with no cases are invalid, never clean.

## Add a case

Create a `test_*.py` module, register a stable id, and add the module to
`CASE_MODULES` in `run.py`. Assertions should use `ctx.run(...)` and the
external artifacts on `SimRun`. Do not assert call order or internal fields.
A test that needs a new observation surface should add that surface to the twin
first; the acceptance plan tracks those gaps.
