/**
 * @defgroup    demand_plugin Plugin Demand
 * @ingroup   plugins
 * @see        demand
 * @brief       hfp 需量内销版插件
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "lib_calclib.h"
#include "demand.h"
#include "fs.h"
#include "snapshot.h"

extern const struct demand_adapter_t CODE demand_adapter;

extern struct demand_info_t    demand_info;

extern uint8 demand_quadrant_change_judge(uint8 reset);

extern uint8 active_power_direction_judge(void);

extern void demand_power_on_hook(void);

extern void demand_output_hook(void);

extern const struct file_format_t CODE file_format[];

extern struct quadrant_judge_t calc_quadrant_judge;

extern struct activepower_direction_judge_t activepower_direction_judge;

extern uint8 demand_dlt645type_transform(int32 dat, uint8 *dest);

extern uint8 combination_reactive_max_demand_get(uint32 id, uint8 *dest, uint16 len, const struct hfp_device_t CODE *device);

/** 当前所有需量复位清零，重新计算 */
void all_demand_reset(struct demand_info_t *info, uint8 clear_word)
{
    uint8 i,seq;

    for (i = 0; i < demand_adapter.item_num; i++)
    {
        if (!(demand_adapter.items[i].attr & clear_word))
        {
            continue;
        }
        if (demand_adapter.items[i].attr & Bit_Map8[ESTABLISH_MEMORY])
        {
            seq = demand_adapter.items[i].memory_seq;
            
            hfp_buffer_clear((uint8 *)&info->demand_run[seq].total_dmd.demand_calc, sizeof(struct demand_calc_t));
            
            hfp_buffer_clear((uint8 *)&info->demand_run[seq].tariff_dmd.demand_calc, sizeof(struct demand_calc_t));
            
            if (true != hfp_read(0x2140, (uint8 *)&info->slider_num, 1, __NULL))
            {
                info->slider_num = 5;
            }
            if (true != hfp_read(0x2141, (uint8 *)&info->sliding, 1, __NULL))
            {
                info->sliding = 1;
            }
            if (true != hfp_read(0x6F61, (uint8 *)&info->tariff_index, 1, __NULL))
            {
                info->tariff_index = 1;
            }
        }
    }
}

/** 当前费率需量复位清零，重新计算 */
void tariff_demand_reset(struct demand_info_t *info, uint8 clear_word)
{
    uint8 i,seq;

    for (i = 0; i < demand_adapter.item_num; i++)
    {
        if (demand_adapter.items[i].attr & Bit_Map8[ESTABLISH_MEMORY])
        {
            seq = demand_adapter.items[i].memory_seq;
            
            hfp_buffer_clear((uint8 *)&info->demand_run[seq].tariff_dmd.demand_calc, sizeof(struct demand_calc_t));

            /* 费率需量切换 */
            if (true != hfp_read(0x6F61, (uint8 *)&info->tariff_index, 1, __NULL))
            {
                info->tariff_index = 1;
            }
        }
    }
}


uint8 demand_adapter_prepare_process(struct demand_unit_t* unit)
{
    uint32 sum_minute, seconds;

    sum_minute = unit->demand_calc.cumu_minute;
    seconds    = unit->demand_calc.cumu_second;

    if (seconds)
    {
        unit->demand_calc.cumu_minute = (sum_minute + seconds / 2) / seconds;
        unit->demand_calc.cumu_second = 0;
    }

    return true;
}


uint32 calc_demand(uint8 num, uint32 *p)
{
    uint8       i;
    uint16      round;
    uint32      *ptr   = p;
    uint64      lngtmp = 0;

    for (i = 0; i < num; i++)
    {
        lngtmp += ptr[i];
    }

    if (num > 0)
    {
        round = (uint16)num * 100;
        return((lngtmp + round / 2) / round);
    }

    return 0x00;
}


uint8 demand_adapter_calc_slider(struct demand_unit_t* unit, uint8 slider_num, uint8 sliding)
{
    uint8 flag = 0;
    
    /* 需量寄存器刷新 */
    unit->slider[unit->demand_calc.total_register_cnt] = unit->demand_calc.cumu_minute;

    /* 需量寄存器索引加 */
    unit->demand_calc.total_register_cnt++;
    
    if ( (unit->demand_calc.total_register_cnt > slider_num - 1)
        || (unit->demand_calc.total_register_cnt > 59 ) )
    {
        unit->demand_calc.total_register_cnt = 0;
    }

    /* 分钟累计功率值清零 */
    unit->demand_calc.cumu_minute = 0;

    /* 总需量滑差计数器加 */
    unit->demand_calc.total_slider_cnt++;

    /* 判断总需量第一个周期是否已到 */
    if (unit->demand_calc.total_cnt < slider_num)
    {
        unit->demand_calc.total_cnt++;
    }

    if ((unit->demand_calc.total_cnt >= slider_num) &&
        (unit->demand_calc.total_slider_cnt >= sliding))
    {
        flag = 1;
        
        unit->demand_calc.total_slider_cnt = 0;

        /* 更新当前需量 */
        unit->demand_calc.cur_demand = calc_demand(slider_num, unit->slider);
    }
        
    hfp_memcopy((uint8 *)&unit->demand_calc.time, (uint8 *)&hfp_time.minute, 0x05);

    return flag;
}

uint8 demand_adapter_calc_max_demand(struct demand_unit_t* unit, uint16 id, uint8 index)
{
    uint32  s_demand = 0, abs_demand = 0, demand = unit->demand_calc.cur_demand;
    uint16 his_id = id;

    his_id += index;
    demand  = unit->demand_calc.cur_demand;

    abs_demand = demand;
    
    if (true != hfp_read(his_id, (uint8 *)&s_demand, 4, __NULL))
    {
        s_demand = 0;
    }
        
    if (abs_demand > s_demand)
    {
        hfp_write(his_id, (uint8 *)&demand, 4, __NULL);
        
        hfp_write(his_id + 0x1000, (uint8 *)&hfp_time.minute, 0x05, __NULL);
    }

    return true;
}

uint8 demand_adapter_execute_scond(struct demand_info_t *info)
{
    uint8                   i, j, value, seq, index, out_sign = 0;;
    uint16                  fsid;
    int32                   power = 0;
    struct access_pattern_t fs_para;
    struct demand_unit_t    *dmd;

    hfp_memset(&fs_para, 0x00, sizeof(fs_para));

    for (i = 0; i < demand_adapter.item_num; i++)
    {
        if (demand_adapter.items[i].calc_func 
            && (!demand_adapter.items[i].calc_func(demand_adapter.items[i].save_id)))
        {
            continue;
        }

        index = 0;
        
        fsid = demand_adapter.items[i].source_id;

        value = demand_adapter.items[i].data_type + 1;
        
        fs_para.len      = 4;
        fs_para.unit     = file_format[value].unit;    
        fs_para.format   = file_format[value].format;
        fs_para.decimals = file_format[value].decimals;

        hfp_read(fsid, (uint8 *)&power, fs_para.len, &fs_para);

        if (power < 0)
        {
            power = 0 - power;
        }
        
        seq = demand_adapter.items[i].memory_seq;
        dmd = (struct demand_unit_t*)&info->demand_run[seq].total_dmd;
        
        for (j = 0; j < 2; j++)
        {
            if (dmd->demand_calc.cumu_second < 60)
            {
                dmd->demand_calc.cumu_second++;
                dmd->demand_calc.cumu_minute += power;
            }

            if (dmd->demand_calc.cumu_second >= 60)
            {
                if (demand_adapter.ops->prepare_process)
                {
                    demand_adapter.ops->prepare_process(dmd);
                }

                if (demand_adapter.ops->calc_slider
                    && demand_adapter.ops->calc_slider(dmd,demand_info.slider_num,demand_info.sliding))
                {
                    if ((i < 2)&&(j == 0))/* 总有功需量滑差到，输出脉冲信号*/
                    {
                       out_sign = 1; 
                    }
                }

                if (demand_adapter.ops->calc_max_demand && (demand_adapter.items[i].attr & Bit_Map8[SAVE_EEPROM]))
                {
                    demand_adapter.ops->calc_max_demand(dmd, demand_adapter.items[i].save_id, index);
                }
            }

            index = demand_info.tariff_index;
            
            dmd = (struct demand_unit_t*)&info->demand_run[seq].tariff_dmd;
        }
    }

    if (out_sign)
    {
        demand_output_hook();
    }

    return true;
}

/* 需量清零 */
void demand_clear_operate(void)
{
	uint32 addr[2];
	uint8 len[2] = { 22,27 };
	uint8 databuff[216],i,j;
	struct file_stat_t file;
	uint8 s_len;
    
	for (j = 0; j < 2; j++)   /* 第一层循环清需量的值，第二层循环清需量发生时间 */
	{
        plugin_machine.clear_watchdog();
        
		hfp_stat(0xA010+0x1000*j, &file);

		addr[0] = file.info.addr;
		addr[1] = file.info.addr_bak;
		
		for (i = 0; i < 8; i++)  /**一次只处理8个需量清零*/
		{
			hfp_memset(&databuff[i * len[j]], 0x00, len[j] - 2);
			databuff[(i + 1)* len[j] - 2] = 0xA5;
			databuff[(i + 1)* len[j] - 1] = 0xA5;
		}

		for (i = 0; i < 2; i++)/* 第一循环清主区的需量，第二循环清备份区的需量 */
		{
			s_len = len[j] * 8;
			if (false == file.info.device->driver->write(addr[i], databuff, s_len, file.info.device))
			{
				return;
			}
		}
	}

    //hfp_write(0x5E50, databuff, 0x01, __NULL);
}

/* 所有需量清零 */
void all_demand_only_clear_no_event_record(struct demand_info_t *info, uint8 clear_word)
{
    all_demand_reset(info, clear_word);/**< 清除当前滑窗需量 */
    
    demand_clear_operate();/**< 清除当前月最大需量 */
}

/* 需量清零并事件记录 */
void demand_clear_and_event_record(struct demand_info_t *info, uint8 clear_word)
{
    /*需量清零事件记录*/
    snapshot_external.battery_process(DemandClear);

    all_demand_only_clear_no_event_record(info, clear_word);/**< 清除当前滑窗需量及当前月最大需量 */
}

const struct demand_notify_t CODE demand_notify[] =
{
    { 0xC500, 0xFF, all_demand_reset    },
    { 0xC503, 0xFF, all_demand_reset    },
    { 0x2140, 0xFF, all_demand_reset    },
    { 0x2141, 0xFF, all_demand_reset    },
    { 0x5E50, 0xFF, all_demand_only_clear_no_event_record  },
    { 0x6F6D, 0xFF, tariff_demand_reset },
    { 0x6F98, 0xFF, demand_clear_and_event_record },

};

const uint8 CODE demand_notify_num = sizeof(demand_notify) / sizeof(struct demand_notify_t);

uint8 demand_adapter_notify(struct demand_info_t *demand, uint16 name)
{
    uint8   i;
    uint8  clear_word;

    for (i = 0; i < demand_notify_num; i++)
    {
        if (name == demand_notify[i].note_id)
        {
            clear_word = demand_notify[i].clear_word;
            demand_notify[i].notify(demand, clear_word);
            break;
        }
    }

    return true;
}

extern uint8 read_card_delay;

uint8 demand_adapter_msg_hander(struct demand_info_t *demand, uint16 msg)
{
    uint8   value, reading = 1;

    if (read_card_delay >= 6)
    {
        if (IO_MSGID_CARD_INSERT == msg)
        {
            if (hfp_read(0x6FF0, &value, 0x01, __NULL) && (SGCC_Prepay_Card == value))
            {
                hfp_write(0x6F4A, &reading, 0x01, __NULL); /**< 卡片插入显示 */
            }
        }
    }

    if (SYS_MSGID_SEC == msg)
    {
        uint8 flag = 0;
        if (active_power_direction_judge())
        {
            flag = 1;
            all_demand_reset(demand, 0xFF);
        }

        if (demand_quadrant_change_judge(flag))
        {
            all_demand_reset(demand, Bit_Map8[QUADRANT_REACTIVE_DEMAND]);
        }
    }
    
    return true;
}


uint8 demand_change_para(struct demand_info_t *info)
{
    return true;
}


void demand_adapter_init(struct demand_info_t *info)
{
    all_demand_reset(info, 0xFF);
}


uint8 demand_adapter_power_on(struct demand_info_t *info)
{
    all_demand_reset(info, 0xFF);

    demand_power_on_hook();
    
    return true;
}


uint8 demand_adapter_power_off(struct demand_info_t *info)
{
    return true;
}


const struct demand_ops_t CODE demand_ops =
{
    demand_adapter_init,
    demand_adapter_execute_scond,
    demand_adapter_prepare_process,
    demand_adapter_calc_slider,
    demand_adapter_calc_max_demand,
    demand_adapter_msg_hander,
    demand_adapter_power_on,
    demand_adapter_power_off,
    demand_adapter_notify,
};

uint8 read_demand(uint32 id, uint8 *dest, uint16 len, const struct hfp_device_t CODE *device)
{
    uint8  i;
    uint8  buf[5];
    uint16 demand_id, tim_id;
    int32  s_demand = 0;
    uint8  id3  = (uint8)((id & 0x0000F000) >> 8);
    uint8  id0  = (uint8)(id & 0x000000FF);
    
    struct log_query_t *fs_para = (struct log_query_t *)device;

    /*运行状态字*/
    if(0x6000 == id)
    {
        *dest = (demand_info.slider_num == demand_info.sliding) ? (uint8)1 : (uint8)0;
        return true;
    }

    /* 组合无功1(2)最大需量、发生时间 */
    if (combination_reactive_max_demand_get(id, dest, len, device))
    {
        return true;
    }

    /*当前需量(698/645)*/
    if((0x60 == id3) && (id0 < 0x10))
    {
        demand_id = 0xA000 + (id0 << 4);
        
        for (i = 0; i < demand_adapter.item_num; i++)
        {
            if (demand_id == demand_adapter.items[i].save_id)
            {
                if (demand_adapter.items[i].calc_func 
                    && (!demand_adapter.items[i].calc_func(demand_id)))
                {
                    hfp_buffer_clear(dest, 0x04);
                    return true;
                }
                s_demand = demand_info.demand_run[demand_adapter.items[i].memory_seq].total_dmd.demand_calc.cur_demand;
                if (demand_id == 0xA0C0)
                {
                    #if 0
                    uint8 value = 0;
                    hfp_read(0x6F30, (uint8 *)&value, 0x01, __NULL);
                    if (value)
                    #endif
                    if(calc_quadrant_judge.quadrant > 2)
                    {
                        s_demand = 0 - s_demand;
                    }
                }
                else if (demand_id == 0xA0B0)
                {
                    if (activepower_direction_judge.direction)
                    {
                        s_demand = 0 - s_demand;
                    }
                }
                if(FORM_BCD == fs_para->parent.format)
                {
                    demand_dlt645type_transform(s_demand,dest);
                }
                else
                {
                    hfp_memcopy(dest, (uint8 *)&s_demand, 0x04);
                }
                return true;
            }
        }
        return false;
    }

    /*dlt/645-2007当前月最大需量+发生日期时间*/
    if ((id >= 0x6010) && (id < 0x60B0))
    {
        demand_id = id + 0x4000;
        tim_id    = id + 0x5000;

        if (true != hfp_read(demand_id, (uint8 *)&s_demand, 4, __NULL))
        {
            return false;
        }

        demand_dlt645type_transform(s_demand,dest);

        if (true != hfp_read(tim_id, buf, 5, __NULL))
        {
            return false;
        }

        hfp_hex_to_bcd_array(buf, dest + 3, 5);

        return true;
    }

    return false;
}

uint8 write_demand(uint32 id, uint8 *dest, uint16 len, const struct hfp_device_t CODE *device)
{
    switch(id)
    {
        case 0x6FBF:
            demand_clear_and_event_record(&demand_info, 0xFF);
            break;
        default:
            break;
    }
    return true;
}

const struct driver_t CODE demand_mem_driver =
{
    NULL,
    read_demand,
    write_demand,
    NULL,
};