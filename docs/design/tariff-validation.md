# Tariff table validation

Date: 2026-11-25. Baseline: `9e96b789`.

`domain/tariff` owns the tariff table limits and one validator that both the
645 and the 698 configuration paths call. Before this change each mapper read
the legacy EEPROM bytes and applied its own bounds; the two rule sets differed
on day-table/fee bounds, spacing and wrap handling.

## Limits

`ND_TARIFF_MAX_SEASONS` 14, `ND_TARIFF_MAX_DAY_TABLES` 8,
`ND_TARIFF_MAX_PERIODS` 14, `ND_TARIFF_MAX_HOLIDAYS` 20,
`ND_TARIFF_MAX_FEES` 12. Step tariffs have no domain aggregate; their
`MaxStepNum` count stays a 645 mapper constant.

## Shared rules

Seasons (`nd_tariff_validate_seasons(seasons, count, day_table_count)`):
- count 1..14; an empty table is an error.
- month 1..12 and day 1..month length (non-leap year); the season key is
  `month << 8 | day`.
- day table 1..min(8, day_table_count); the caller passes the configured
  day-table count, so a season cannot point at a table that is not configured.
- entries increase; at most one wrap (the last-to-first step is the single
  permitted decrease); equal starts are allowed only with the same day table.

Periods (`nd_tariff_validate_periods(periods, count, fee_count)`):
- count 1..14; an empty day table is an error.
- hour 0..23, minute 0..59; the key is `hour << 8 | minute`.
- fee 1..min(12, fee_count).
- entries increase; at most one wrap; equal starts are allowed only with the
  same fee; consecutive starts, including the wrap, are at least 15 minutes
  apart.

Whole table (`nd_tariff_table_validate(table)`): count bounds, seasons with
the table's day-table count, each configured day table's periods with the
table's fee count, holiday dates and day tables, and the rest-day table when
set. A zero rest-day table means unconfigured and is accepted.

## Channel mapping

645 (`SeasonTab_Format_Check`, `TouTab_Format_Check`): BCD records are
decoded into the interval structs and validated; any season error maps to
`RS_ERR_OTHERS`, a period spacing/order/time error maps to `RS_ERR_OTHERS`,
a zero fee maps to `RS_ERR_OTHERS` and a fee above the configured count maps
to `RS_ERR_FEEOV`. Accepted tables keep the existing padding and stored byte
layout.

698 (`0x400C`, `0x4011`, `0x4014/0x4015`, `0x4016/0x4017`): counts use the
domain maxima; holiday day tables use `ND_TARIFF_MAX_DAY_TABLES`; season and
period tables are decoded and validated. `ND_TARIFF_ERR_COUNT` maps to the
existing count DAR of the OAD, `ND_TARIFF_ERR_VALUE` on a fee to
`DAR_TARIFF_NUM_EXCEED` and on a day table to `DAR_RW_REJECT`, and the
remaining errors to `DAR_RW_REJECT`/`DAR_TIME_ZONE_EXCEED` as the channel
already did. The stored legacy layout and padding are unchanged.
