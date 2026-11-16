#include "nidavellir.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "application/adapters/dl698/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "targets/stategrid_meter/persistence/meter_record.h"

uint32_t do_checksum(uint8_t *buf, uint16_t len)
{
    uint16_t i, checksum = 0xA5A5;

    for (i = 0; i < len; i++)
    {
        checksum += *(buf+i);
    }
    return checksum;
}


uint32_t do_block_static_checksum(void *instance)
{
    uint8_t *buff;
    uint16_t len;
    struct mbase_control_block_t* mbase_cb;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    buff = (uint8_t *)mbase_cb + nd_mbase_offset_static_section_start() + 1;

    len = (uint16_t)(nd_mbase_offset_static_chksum()
                     - nd_mbase_offset_static_section_start() - 1);

    return do_checksum(buff, len);
}
