#include "infra/security/esam/esam_status.h"
#include "targets/stategrid_meter/app/resource_ids.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "application/ports/resource_access.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "application/adapters/dl698/fr_698_env.h"
#include <stddef.h>

static struct mbase_control_block_t *mb(void *instance)
{
    return ((stategrid_context_t *)instance)->mbase;
}

void nd_esam_status_chip_abnormal_set(void *instance,
                                      uint8_t abnormal)
{
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_esam_chip_abnormal(block) = abnormal;
    }
}

void nd_esam_status_key_state_set(void *instance, uint8_t state)
{
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_key_state(block) = state;
    }
}

void nd_esam_status_auth_on_set(void *instance, uint8_t on)
{
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_AuthOn(block) = on;
    }
}

int nd_esam_status_meter_no_update(void *instance,
                                   const uint8_t *meter_no)
{
    const nd_resource_port_t *port = nd_fr698_resource_access(instance);

    struct mbase_control_block_t *block = mb(instance);
    uint32_t addr;
    int i;

    if (block == NULL || meter_no == NULL)
    {
        return 0;
    }
    for (i = 0; i < 6; i++)
    {
        if (nd_mbase_meterNo(block)[i] != meter_no[i])
        {
            break;
        }
    }
    if (i == 6)
    {
        return 0;
    }
    for (i = 0; i < 6; i++)
    {
        nd_mbase_meterNo(block)[i] = meter_no[i];
    }
    addr = ((uint32_t)(RES_DEV_EEPROM << 24)
            + ((uint32_t)mbase_block_static_eprom << 16)
            + nd_mbase_offset_meterNo());
    nd_set_offset_port(port, addr, nd_mbase_meterNo(block), 6);
    return 1;
}

int nd_esam_status_auth_pending(void *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL
           && (nd_mbase_term_auth_timeout(block) != 0
               || nd_mbase_infrared_auth_timeout(block) != 0
               || nd_mbase_auth_timeout_645(block) != 0);
}
