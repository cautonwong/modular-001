#ifndef ND_TARGET_STATEGRID_LEGACY_TARIFF_PARSER_H
#define ND_TARGET_STATEGRID_LEGACY_TARIFF_PARSER_H

#include "domain/common/parameter_store.h"
#include "domain/tariff/tariff.h"

int legacy_tariff_load(const nd_parameter_store_t *store, nd_tariff_table_t *table);

#endif
