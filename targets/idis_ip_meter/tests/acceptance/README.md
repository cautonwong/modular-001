# IDIS acceptance

`test_idis_acceptance.py` registers the IDIS acceptance cases with the shared
runner in `tests/acceptance/` and drives `idis_host_runner` through its
external contract. The file is the authoritative list; the first four are:

- `idis.power-loss`: PQ records and the event log survive an abrupt exit
  without the final save.
- `idis.billing-default`: the default monthly billing schedule fires at the
  month boundary.
- `idis.default-calendar`: the default daily tariff calendar selects script 1.
- `idis.restart-persistence`: energy, demand, alarm and billing survive a
  restart.

Run them locally:

```sh
python3 tests/acceptance/run.py \
  --runner build/ci-host/idis_host_runner \
  --cases targets/idis_ip_meter/tests/acceptance \
  --select tag:idis
```

`tools/ci_matrix.py` runs the cases for both IDIS targets and writes evidence
under `build/acceptance/evidence/<target>/`.
