/**
 * @defgroup    tariff_plugin Plugin Tariff
 * @ingroup     plugins
 * @see         tou
 * @brief       hfp 费率内销版插件
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "hfp_handy.h"
#include "fs.h"
#include "tou.h"
#include "tariff_api.h"
#include "snapshot.h"
#include "lib_calclib.h"


static struct tariff_state_t               state;
extern uint8 vice_tariff_para_num;
extern const uint16 CODE tariff_notify_fsid[];
extern const struct tariff_para_t CODE     tariff_para[];
extern const struct vice_tariff_tim_t CODE vice_start_para[];

static uint8 check_time_up(struct rtc_t *time)
{
    if ((time->hour == 0x00) && (time->minute == 0x00))
    {
        return true;
    }
    
    return false;
}

uint8 check_out(enum check_out_type_t type, uint8 value)
{
    uint8 flag = false;
    
    switch(type)
    {
        case TYPE_TABLE_ID:
            if (value != state.current_day_tab_id)
            {
                flag = true;
                state.current_day_tab_id = value;
            }
            break;
        case TYPE_TABLE_SEQ:
            if (value != state.table_seq)
            {
                flag = true;
                state.table_seq = value;
            }
            break;
        case TYPE_TABLE_TARIFF:
            if (value != state.last_tariff)
            {
                if (0xFF != state.last_tariff)
                {
                    flag = true;
                    hfp_write(0x6F6D, &flag, 1, __NULL);
                }
                state.last_tariff = value;
            }
            break;
        default:
            break;
    }

    if (flag)
    {
        state.change_out = true;     
        return true;
    }

    return false;
}

static uint8 holiday_tab_check(struct rtc_t *ptime)
{
    uint8 holday_num, cnt, j, num = 0;
    struct access_pattern_t fs_para;
    struct holiday_store_t  *holiday_para;
    uint16 i;
    uint8  buf[100];

    if (true != hfp_read(tariff_para->holiday_num_fsid, &holday_num, 0x01, __NULL))
    {
        return false;
    }
    
    if ((holday_num < 0x01) || (holday_num > tariff_para->holiday_max_num))
    {
        return false;
    }

    hfp_memset((uint8 *)&fs_para, 0x00, sizeof(fs_para));

    for (i = 0; i < holday_num / 20 + 1; i++)
    {
        if (i == holday_num / 20)
        {
            cnt = holday_num % 20;
        }
        else
        {
            cnt = 20;
        }

        if (cnt == 0)
        {
            break;
        }

        fs_para.offset = 5 * num;
        num           += cnt;

        if (true == hfp_read(tariff_para->holiday_data_fsid, (uint8 *)buf, cnt * 5, &fs_para))
        {
            for (j = 0; j < cnt; j++)
            {
                holiday_para = (struct holiday_store_t *)&buf[j * 5];

                if ((ptime->year  == holiday_para->year) &&
                    (ptime->month == holiday_para->month) &&
                    (ptime->day   == holiday_para->day))
                {
                    check_out(TYPE_TABLE_ID, holiday_para->day_tab_id);
                    return true;
                }
            }
        }
    }

    return false;
}

static uint8 season_tab_look(struct rtc_t *ptime)
{
    Word16             time0, time1, time;
    uint8              i, season_num, para[42];
    uint16             cur_data_id;
    struct season_node *buf;

    if (true != hfp_read(tariff_para->season_num_fsid, &season_num, 0x01, __NULL))
    {
        season_num = tariff_para->season_max_num;
    }

    if ((season_num < 0x01) || (season_num > tariff_para->season_max_num))
    {
        return false;
    }
    
    cur_data_id = tariff_para->cur_season_data_fsid;

    if (true != hfp_read(cur_data_id, para, season_num * 3, __NULL))
    {
        return false;
    }

    time.byte[1] = ptime->month;
    time.byte[0] = ptime->day;

    for (i = 0; i < season_num; i++)
    {
        buf           = (struct season_node *)&para[((i + 1) % season_num) * 3];
        time1.byte[1] = buf->month;         
        time1.byte[0] = buf->day;           
        buf           = (struct season_node *)&para[i * 3];
        time0.byte[1] = buf->month;         
        time0.byte[0] = buf->day;             

        if (time0.word < time1.word)
        {
            if ((time.word >= time0.word) && (time.word < time1.word))
            {
                check_out(TYPE_TABLE_ID, buf->day_tab_id);
                return true;
            }
        }
        else
        {
            if ((time.word >= time0.word) || (time.word < time1.word))     
            {
                check_out(TYPE_TABLE_ID, buf->day_tab_id);
                return true;
            }
        }
    }

    return false;
}

static uint8 week_tab_look(struct rtc_t *ptime)
{
    struct week_store_t week;

    if (true != hfp_read(tariff_para->weekday_config_fsid, &week.week_mode, 0x01, __NULL) 
        || true != hfp_read(tariff_para->weedday_tab_fsid, &week.day_tab_id, 0x01, __NULL))
    {
        return false;
    }

    if ((week.week_mode & Bit_Map8[ptime->week]) == 0)
    {
        check_out(TYPE_TABLE_ID, week.day_tab_id);
        return true;
    }

    return false;
}

static uint8 search_day_tab_id(struct rtc_t *ptime)
{
    if (false == holiday_tab_check(ptime))
    {
        if (false == week_tab_look(ptime))
        {
            season_tab_look(ptime);
        }
    }

    return true;
}

static uint8 day_tab_look(struct rtc_t *ptime)
{
    Word16 time0, time1, time;
    uint8  j, day_num, buf[42];
    uint16 cur_data_id;
    struct access_pattern_t fs_para;

    time.byte[0] = ptime->minute;
    time.byte[1] = ptime->hour;

    hfp_memset((uint8 *)&fs_para, 0x00, sizeof(fs_para));

    if (true != hfp_read(tariff_para->daytab_num_fsid, &day_num, 0x01, __NULL))
    {
        day_num = tariff_para->daytab_max_num;
    }
    
    if ((state.current_day_tab_id < 0x01) || (state.current_day_tab_id > day_num))
    {
        state.current_day_tab_id = 0x01;
    }

    if (true != hfp_read(tariff_para->daytou_num_fsid, &day_num, 0x01, __NULL))
    {
        day_num = tariff_para->daytou_max_num;
    }

    cur_data_id = tariff_para->cur_daytab_data_fsid + state.current_day_tab_id - 1;

    if (true == hfp_read(cur_data_id, buf, tariff_para->daytou_max_num * 3, &fs_para))
    {
        for (j = 0; j < day_num; j++)
        {
            time0.byte[1] = buf[3 * j];
            time0.byte[0] = buf[3 * j + 1];
            time1.byte[1] = buf[3 * ((j + 1) % day_num)];
            time1.byte[0] = buf[3 * ((j + 1) % day_num) + 1];

            if (time0.word < time1.word)
            {
                if ((time.word >= time0.word) && (time.word < time1.word))
                {
                    break;
                }
            }
            else
            {
                if ((time.word >= time0.word) || (time.word < time1.word)) 
                {
                    break;
                }
            }
        }

        if (j != day_num)
        {
            state.current_tariff            = buf[3 * j + 2];
            
            state.current_t_start_time.word = time.word;
            
            state.next_t_start_time.word    = time1.word;

            /*检查时段是否发生变化*/
            check_out(TYPE_TABLE_SEQ, j);

            /*检查费率号是否发生变化*/
            check_out(TYPE_TABLE_TARIFF, state.current_tariff);

            /*多功能口脉冲输出*/
            if ((true == state.change_out)
                &&(TARIFFOUT == multifunctional_io_out_way))
            {
                hfp_ctrl(0x1050 , CTRL_RTC_MULT_PLUSE , __NULL);
            }

            state.change_out = false;
            
            return true;
        }
    }

    state.last_tariff    = TARIFF_INDEX_DEFAULT;
    
    state.current_tariff = TARIFF_INDEX_DEFAULT;

    return false;
}

/*两套时区表、时段表切换冻结*/
static void switch_record(uint8 type)
{
	hfp_write(0xC710, (uint8 *)&hfp_time.second, 0x07, __NULL);

    if(0x00 == type)
    {
        /*时区表切换冻结 */  		
        snapshot_external.battery_process(0x0902);
    }
    else
    {
        /*日时段表切换冻结 */ 
        snapshot_external.battery_process(0x0903);
    }
}

static uint8 vice_tariff_start_look(struct rtc_t *ptime)
{
    uint8                   i, j, value, buf[42];
    uint32                  act_minute, cur_minute;
    struct rtc_t            act_time;
    struct file_stat_t      item;
    struct access_pattern_t fs_para;

    hfp_memset(&fs_para, 0x00, sizeof(fs_para));
    hfp_memset(&item, 0x00, sizeof(item));

    value = false;
    
    for (i = 0; i < vice_tariff_para_num; i++)
    {
        if (hfp_read(vice_start_para[i].start_tim_fsid, (uint8 *)&act_time.minute, 5, __NULL))
        {
            if ((act_time.month > 0) && (act_time.month <= 12))
            {
                if (1 == i)
                {
                    state.switch_flag |= TIME_FRAME_TABLE_SWITCH_WAITING;
                }
                cur_minute = hfp_get_total_minute((uint8 *)&ptime->minute, 0);
                act_minute = hfp_get_total_minute((uint8 *)&act_time.minute, 0);

                if (cur_minute >= act_minute)
                {
                    hfp_buffer_clear((uint8 *)&act_time.minute, 5);
                    
                    hfp_write(vice_start_para[i].start_tim_fsid, (uint8 *)&act_time.minute, 5, __NULL);

                    hfp_stat(vice_start_para[i].vice_data_fsid, &item);

                    for(j = 0; j < vice_start_para[i].table_num; j++)
                    {
                        plugin_machine.clear_watchdog();
                        if(hfp_read(vice_start_para[i].vice_data_fsid + j, buf, item.len, &fs_para))
                        {
                            hfp_write(vice_start_para[i].cur_data_fsid + j, buf, item.len, &fs_para);
                        }
                    }
                    if (1 == i)
                    {
                        state.switch_flag &= ~TIME_FRAME_TABLE_SWITCH_WAITING;
                    }
                    value            = true; 
                    state.change_out = true; /*时段透切信号输出*/
                    switch_record(i);        /*两套时区、时段表切换冻结*/
                }
            }
            else
            {
                if (1 == i)
                {
                    state.switch_flag &= ~TIME_FRAME_TABLE_SWITCH_WAITING;
                }
            }
        }
    }

    return value;
}

uint8 read_tariff(uint32 id, uint8 *dest, uint16 len, const struct hfp_device_t CODE *device)
{
    struct rtc_t            act_time;
    
    switch ((uint16)id)
    {
        case 0x6F5C:
            *dest = state.switch_flag;
            break;
        case 0x6F61:
            *dest = state.current_tariff;
            break;
        case 0x6F4D:
        case 0x6F4E:
            *dest = false;
            if (hfp_read(0x3211, (uint8 *)&act_time.minute, 0x05, __NULL))
            {
                if (act_time.month || (0x6F4D == (uint16)id))
                {
                    *dest = true;
                }
            }
            break;
        default:
            return false;
            break;
    }

    return true;
}

const struct driver_t CODE tariff_mem_driver =
{
    NULL,
    read_tariff,
    NULL,
    NULL,
};

void tariff_scan(struct rtc_t *rtc)
{
    vice_tariff_start_look(rtc);
    search_day_tab_id(rtc);
    day_tab_look(rtc);
}

uint8 tariff_adapter_update(struct rtc_t *rtc)
{
    if (vice_tariff_start_look(rtc) || check_time_up(rtc))
    {
        search_day_tab_id(rtc);
    }
    
    if( true != day_tab_look(rtc))
    {
        state.current_tariff = TARIFF_INDEX_DEFAULT;
    }

    return false;
}

uint8 tariff_adapter_activate(struct rtc_t *rtc)
{
    return false;
}

uint8 tariff_adapter_notify(uint16 name)
{
    uint16                  i;
    const uint16            *p = tariff_notify_fsid;
    struct file_stat_t      item;

    while(*p)
    {
        if(*p == name)
        {
            tariff_scan(&hfp_time);
            return true;
        }
        p++;
    }

    /*公共假日表写判断*/
    if(true == hfp_stat(tariff_para->holiday_data_fsid, &item))
    {
        for(i = 0; i < item.same; i++)
        {
            if(tariff_para->holiday_data_fsid + i == name)
            {
                tariff_scan(&hfp_time);
                
                plugin_machine.clear_watchdog();
                
                return true;
            }
        }
    }

    return false;
}

void  tariff_power_on(struct rtc_t *rtc)
{
    state.table_seq   = 0xFF;
    state.change_out  = false;
    state.last_tariff = 0xFF;
    state.switch_flag = 0;

    tariff_scan(&hfp_time);
}

struct tou_adapter_t tariff_adapter =
{
    __NULL,
    tariff_power_on,
    tariff_adapter_update,
    tariff_adapter_activate,
    tariff_adapter_notify
};
/**@}*/
