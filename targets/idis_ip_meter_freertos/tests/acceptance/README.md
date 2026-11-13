# IDIS acceptance

These cases drive `idis_host_runner` through the external contract:

- `power_loss.sh`: records survive an abrupt exit without the final save.
- `billing_default.sh`: the default monthly billing schedule fires at midnight.
- `default_calendar.sh`: the default daily tariff calendar selects script 1.
- `restart_persistence.sh`: energy, demand, alarm and billing survive a restart.

The shared test-platform lives in `tests/acceptance/`; target-specific cases
live here and are run by `tools/ci_matrix.py`.
