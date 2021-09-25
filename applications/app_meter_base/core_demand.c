/**
 * @ingroup   demand  
 * @brief       
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "demand.h"
#include "fs.h"
#include "lib_calclib.h"

extern const struct demand_adapter_t CODE demand_adapter;

extern void demand_output_hook(void);

struct demand_info_t    demand_info;

void demand_init(void)
{
    uint8 i,num = 0;

    for (i = 0; i < demand_adapter.item_num; i++)
    {
        if (demand_adapter.items[i].attr & Bit_Map8[ESTABLISH_MEMORY])
        {
            num++;
        }
    }
    
    demand_info.demand_run = (struct demand_run_t *)hfp_heap.malloc((uint16)sizeof(struct demand_run_t) * num, 1);

    if (__NULL != demand_info.demand_run)
    {
        num = 0;
        for (i = 0; i < demand_adapter.item_num; i++)
        {
            if (demand_adapter.items[i].attr & Bit_Map8[ESTABLISH_MEMORY])
            {
                demand_info.demand_run[num].total_dmd.slider  = (uint32 *)hfp_heap.malloc((uint16)demand_adapter.items[i].slider_num << 2, 1);
                demand_info.demand_run[num].tariff_dmd.slider = (uint32 *)hfp_heap.malloc((uint16)demand_adapter.items[i].slider_num << 2, 1);
                num++;
            }
        }
        demand_adapter.ops->init(&demand_info);
    }
}


void demand_power_on(void)
{
    if (demand_adapter.ops->power_on)
    {
        demand_adapter.ops->power_on(&demand_info);
    }
}


void demand_execute_second(void)
{
    if (demand_adapter.ops->execute_second)
    {
        demand_adapter.ops->execute_second(&demand_info);
    }
}

void demand_execute_minute(void)
{
    uint8                   i, j, seq, index, out_sign = 0;
    struct demand_unit_t    *dmd;

    for (i = 0; i < demand_adapter.item_num; i++)
    {
        if (demand_adapter.items[i].calc_func 
            && (!demand_adapter.items[i].calc_func(demand_adapter.items[i].save_id)))
        {
            continue;
        }

        index = 0;
        
        seq = demand_adapter.items[i].memory_seq;

        dmd = (struct demand_unit_t *)&demand_info.demand_run[seq].total_dmd;

        for (j = 0; j < 2; j++)
        {
            if (demand_adapter.ops->prepare_process)
            {
                demand_adapter.ops->prepare_process(dmd);
            }

            if (demand_adapter.ops->calc_slider
                && demand_adapter.ops->calc_slider(dmd,demand_info.slider_num,demand_info.sliding))
            {
                if (j == 0)/* 总需量滑差到，输出脉冲信号*/
                {
                   out_sign = 1; 
                }
            }

            if (demand_adapter.ops->calc_max_demand && (demand_adapter.items[i].attr & Bit_Map8[SAVE_EEPROM]))
            {
                demand_adapter.ops->calc_max_demand(dmd, demand_adapter.items[i].save_id, index);
            }

            index = demand_info.tariff_index;
            dmd = (struct demand_unit_t *)&demand_info.demand_run[seq].tariff_dmd;
        }
    }
    if (out_sign)
    {
        demand_output_hook();
    }
}

void demand_execute_notity(void)
{
    uint16 updata = 0;

    while (0x00 != (updata = hfp_notify(name_demand)))  /**< 关注参数是否被修改 */
    {
        if (demand_adapter.ops->notify)
        {
            demand_adapter.ops->notify(&demand_info, updata);
        }
    }
}

void demand_msg_handler(uint16 msg_id)
{
    if (demand_adapter.ops->msg_hander)
    {
        demand_adapter.ops->msg_hander(&demand_info, msg_id);
    }
}


uint16 demand_power_off(uint8 *buf)
{
    if (demand_adapter.ops->power_off)
    {
        demand_adapter.ops->power_off(&demand_info);
    }

	return 0x00;
}


void demand_get_version(uint8 *data)
{
#if 1
    uint8 version[] = "demand_code_v01";
#endif

    hfp_memcopy(data, version, sizeof(version));
}

                                                                      /*	tick  sec   min	  hour  day   mon   year  week */
const struct standalone_timing_node_t CODE            demand_rtc_second = { 0x03, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99 };
const struct standalone_timing_node_t CODE            demand_rtc_minute = { 0x04, 0x00, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99 };
const struct standalone_timing_node_t CODE            demand_rtc_notity = { 0x05, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99 };
const struct module_execute_t CODE demand_execute_list[] =
{
    { &demand_rtc_second, demand_execute_second },          /**< 每秒钟执行 */
#if 0
    { &demand_rtc_minute, demand_execute_minute },          /**< 每分钟执行 */
#endif
    { &demand_rtc_notity, demand_execute_notity },          /**< 每秒种执行 */
    { __NULL,             __NULL                },          /**< 结束符 */
};

const struct module_t CODE core_demand =
{
    demand_execute_list,
    demand_init,
    __NULL,
    demand_power_on,
    demand_msg_handler,
    __NULL,
    demand_power_off,
    __NULL,
    demand_get_version
};

/**@}*/
