/**
 * @ingroup    tou 
 * @brief       tou
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "tou.h"
#include "fs.h"
#include "lib_calclib.h"


static void tou_init(void)
{
    uint8 i;

    for (i = 0; i < tou_cfg_num; i++)
    {
        if (tou_cfg[i].tou_adapter->init)
        {
            tou_cfg[i].tou_adapter->init();
        }
    }
}


static void tou_min_update(void)
{
    uint8 i;

    for (i = 0; i < tou_cfg_num; i++)
    {
        if (tou_cfg[i].tou_adapter->update)
        {
            tou_cfg[i].tou_adapter->update(&hfp_time);
        }

		if (tou_cfg[i].tou_adapter->activate)
		{
			tou_cfg[i].tou_adapter->activate(&hfp_time);
		}
    }

}

static void tou_sec_activate(void)
{
    uint8  i;
    uint16 note_id;

    while (0x00 != (note_id = hfp_notify(name_clock)))
    {
        for (i = 0; i < tou_cfg_num; i++)
        {
            if (tou_cfg[i].tou_adapter->notify)
            {
                if(true == tou_cfg[i].tou_adapter->notify(note_id))
                {
                    break;
                }
            }
        }
    }

}


static void tou_power_on(void)
{
    uint8 i;

    for (i = 0; i < tou_cfg_num; i++)
    {
        if (tou_cfg[i].tou_adapter->power_on)
        {
            tou_cfg[i].tou_adapter->power_on(&hfp_time);
        }
    }
}


static void tou_version(uint8 *data)
{
    uint8 version[] = "tariff_core_v01";

    hfp_memcopy(data, version, sizeof(version));
}


/*	tick    sec   min	hour  day   mon   year  week */
const struct standalone_timing_node_t CODE tou_rtc_minute = { 0x01, 0x00, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99 };
const struct standalone_timing_node_t CODE tou_rtc_lap = { 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99 };

const struct module_execute_t CODE tou_execute_list[] =
{
    { &tou_rtc_lap, tou_sec_activate    }, /**< 每循环检测副费率表是否需要激活 */
    { &tou_rtc_minute, tou_min_update   }, /**< 每分钟的00秒检测费率是否需要更新 */
    { __NULL,          __NULL           }  /**< 结束标示符 */
};

const struct module_t CODE core_tou =
{
    tou_execute_list,
    tou_init,
    __NULL,
    tou_power_on,
    __NULL,
    __NULL,
    __NULL,
    __NULL,
    tou_version,
};
/** @} */
