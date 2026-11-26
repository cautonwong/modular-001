#include "targets/stategrid_meter/app/tariff_lookup.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "targets/stategrid_meter/app/resource_ids.h"
#include "application/ports/resource_access.h"


static uint32_t parameter_index(uint16_t id)
{
    return ((uint32_t)RES_DEV_EEPROM << 24)
           + ((uint32_t)mbase_block_parameter << 16) + id;
}

static int eeprom_read(void *ctx, uint16_t id, uint8_t *buffer, size_t length)
{
    const nd_resource_port_t *port = ctx;

    return nd_get_index_port(port, parameter_index(id), buffer,
                             (uint16_t)length) < 0
               ? -1
               : 0;
}

static int eeprom_write(void *ctx, uint16_t id, const uint8_t *buffer, size_t length)
{
    const nd_resource_port_t *port = ctx;

    return nd_set_index_port(port, parameter_index(id), (uint8_t *)buffer,
                             (uint16_t)length) < 0
               ? -1
               : 0;
}

int tariff_lookup_reload(void *instance)
{
    nd_parameter_store_t store = {
        &((stategrid_context_t *)instance)->resource, eeprom_read, eeprom_write};

    return tariff_lookup_load(&store);
}
