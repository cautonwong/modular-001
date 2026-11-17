#ifndef ND_TARIFF_LOOKUP_H
#define ND_TARIFF_LOOKUP_H

#include <stdint.h>
#include "domain/common/calendar.h"
#include "domain/common/parameter_store.h"
int tariff_lookup_load(const nd_parameter_store_t *store);
int tariff_lookup_reload(void *instance);
uint8_t tariff_lookup_day_table(const nd_datetime_t *now);
uint8_t tariff_lookup_fee(uint8_t day_table, const nd_datetime_t *now,
                          uint8_t *period_changed);

#endif
