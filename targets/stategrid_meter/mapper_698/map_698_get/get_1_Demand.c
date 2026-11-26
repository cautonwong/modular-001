#include <string.h>

#include "fr_698.h"
#include "targets/stategrid_meter/persistence/meter_base_view.h"
#include "components/core/types/utils.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/app/metering/demand_adapter.h"

/*
 * Attribute 2 returns a structure: current value, current fee demand,
 * maximum, maximum fee demand, maximum time, window start time. Raw values
 * are 0.1 W, as the active-power source. Placeholder until the product OAD
 * table lands. A large time of 0 means "never" and is encoded as a zero date.
 */
static uint8_t *put_u32(uint8_t *p, uint32_t value)
{
    mem_swap_cpy(p, (uint8_t *)&value, 4);
    return p + 4;
}

static uint8_t *put_time(uint8_t *p, uint32_t seconds)
{
    struct tm time;

    *p++ = TYPE_date_time_s;
    if (seconds == 0) {
        memset(p, 0, 7);
        return p + 7;
    }
    rtc_localtime(seconds, &time);
    *p++ = (uint8_t)((time.tm_year + 1900) >> 8);
    *p++ = (uint8_t)(time.tm_year + 1900);
    *p++ = (uint8_t)(time.tm_mon + 1);
    *p++ = (uint8_t)time.tm_mday;
    *p++ = (uint8_t)time.tm_hour;
    *p++ = (uint8_t)time.tm_min;
    *p++ = (uint8_t)time.tm_sec;
    return p;
}

int GET_1_Demand(void *instance, uint32_t id, uint8_t* buf, void *ctx)
{
    uint8_t *p = buf;
    uint8_t attribute = get_OAD_Attribute(id);
    uint16_t oi = get_OAD_OI(id);
    stategrid_context_t *context = (stategrid_context_t *)instance;
    const nd_demand_view_t *view;

    (void)ctx;
    if (!nd_mb_valid(instance))
    {
        return -DAR_OTHERS_ERROR;
    }
    if (oi != ND_DEMAND_OI)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }
    if (attribute == 1)
    {
        return fr_698_get_logic_name(p, oi);
    }
    if (attribute == 3)
    {
        *p++ = TYPE_Scaler_Unit;
        *p++ = 0xFF; /* -1: raw unit is 0.1 W */
        *p++ = unit_W;
        return (int)(p - buf);
    }
    if (attribute != 2)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }
    view = nd_demand_adapter_view(context->demand, id);
    if (view == NULL)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }

    *p++ = TYPE_structure;
    *p++ = 6;
    *p++ = TYPE_double_long_unsigned;
    p = put_u32(p, view->value);
    *p++ = TYPE_double_long_unsigned;
    p = put_u32(p, view->fee_value);
    *p++ = TYPE_double_long_unsigned;
    p = put_u32(p, view->max_value);
    *p++ = TYPE_double_long_unsigned;
    p = put_u32(p, view->max_fee_value);
    p = put_time(p, view->max_time);
    p = put_time(p, view->start_time);
    return (int)(p - buf);
}
