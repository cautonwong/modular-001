#include "targets/stategrid_meter/app/tariff_lookup.h"

#include "nidavellir.h"

static uint32_t parameter_index(uint16_t id)
{
    return ((uint32_t)RES_DEV_EEPROM << 24)
           + ((uint32_t)mbase_block_parameter << 16) + id;
}

static int eeprom_read(void *ctx, uint16_t id, uint8_t *buffer, size_t length)
{
    struct instance_t *instance = ctx;

    return nd_get_index(instance, parameter_index(id), buffer, (uint16_t)length) < 0
               ? -1
               : 0;
}

static int eeprom_write(void *ctx, uint16_t id, const uint8_t *buffer, size_t length)
{
    struct instance_t *instance = ctx;

    return nd_set_index(instance, parameter_index(id), (uint8_t *)buffer,
                        (uint16_t)length) < 0
               ? -1
               : 0;
}

int tariff_lookup_reload(struct instance_t *instance)
{
    nd_parameter_store_t store = {instance, eeprom_read, eeprom_write};

    return tariff_lookup_load(&store);
}
