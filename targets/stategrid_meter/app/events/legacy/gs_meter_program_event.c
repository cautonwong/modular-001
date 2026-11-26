#include "targets/stategrid_meter/app/stategrid_context.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "targets/stategrid_meter/persistence/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "port.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "application/ports/resource_access.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "domain/events/program_list.h"
#include "application/adapters/dl698/event_oad_view.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"


//编程事件 


// 编程列表清零
int MeterWr_obj_list_clear(void *instance, void *task, void *mesg)
{
    const nd_resource_port_t *port = nd_fr698_resource_access(instance);

    uint8_t tempbuf[52];
    uint32_t index = 0;
    int ret;

    memset(tempbuf, 0xff, sizeof(tempbuf));
    tempbuf[0] = 0x01;
    tempbuf[1] = 0;

    index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr_List);;
    ret = nd_set_index_port(port, index, &tempbuf[0], sizeof(tempbuf));
    return ret;
}

// 更新编程列表
int MeterWr_obj_list_add(void *instance, void *task, void *mesg, uint32_t oad, uint8_t tag)
{
    const nd_resource_port_t *port = nd_fr698_resource_access(instance);

    struct mbase_control_block_t* ctrl_block  = ((stategrid_context_t *)instance)->mbase;
    uint8_t payload[100];
    uint8_t ofset, tempbuf[52];
    uint32_t index = 0;
    int ret;

    memset(tempbuf, 0, sizeof(tempbuf));

    index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr_List);;
    ret = nd_get_index_port(port, index, &tempbuf[0], sizeof(tempbuf));

    if (ret < 0)
    {
        MeterWr_obj_list_clear(instance, task, mesg);
        tempbuf[0] = 0x01;
        tempbuf[1] = 0;
    }

    tempbuf[1] = nd_program_list_insert_count(tempbuf[1]);

    ndv_memcopy(&tempbuf[7], &tempbuf[2], sizeof(tempbuf) - 7);

    ofset = 2;
    tempbuf[ofset] = tag;

    uint32_to_buf_msb(oad, tempbuf + ofset + 1);
    uint8_t first = nd_program_list_is_first(tempbuf[1]);
    tempbuf[1]++;

    index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr_List);;
    ret = nd_set_index_port(port, index, &tempbuf[0], sizeof(tempbuf));

    if (first) //
    {
        ctrl_block->MeterWr_SetTime_flag = 0;
        if (nd_program_list_sets_time_698(oad))  //若第一个编程为校时,则编程开始时间取校时前时间
        {
            ctrl_block->MeterWr_SetTime_flag = 0x5A;
        }
        record_event_no_mesg_pro(instance, task, MeterWrS, payload, 1);
    }
    else
    {
        record_event_no_mesg_pro(instance, task, MeterWrS, payload, -1);
    }

    return 1;
}


// 编程列表清零
int MeterWr645_obj_list_clear(void *instance, void *task, void *mesg)
{
    const nd_resource_port_t *port = nd_fr698_resource_access(instance);

    uint8_t tempbuf[52];
    uint32_t index = 0;
    int ret;

    memset(tempbuf, 0xff, sizeof(tempbuf));
    tempbuf[0] = 0x01;
    tempbuf[1] = 0;

    index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr645_List);;
    ret = nd_set_index_port(port, index, &tempbuf[0], sizeof(tempbuf));
    return ret;
}

// 更新编程列表
int MeterWr645_obj_list_add(void *instance, void *task, void *mesg, uint32_t oad, uint8_t tag)
{
    const nd_resource_port_t *port = nd_fr698_resource_access(instance);

    struct mbase_control_block_t* ctrl_block  = ((stategrid_context_t *)instance)->mbase;
    uint8_t payload[100];
    uint8_t ofset, tempbuf[52];
    uint32_t index = 0;
    int ret;

    memset(tempbuf, 0, sizeof(tempbuf));

    index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr645_List);;
    ret = nd_get_index_port(port, index, &tempbuf[0], sizeof(tempbuf));

    if (ret < 0)
    {
        MeterWr645_obj_list_clear(instance, task, mesg);
        tempbuf[0] = 0x01;
        tempbuf[1] = 0;
    }

    tempbuf[1] = nd_program_list_insert_count(tempbuf[1]);
    
    ndv_memcopy(&tempbuf[7], &tempbuf[2], sizeof(tempbuf) - 7);

    ofset = 2;
    tempbuf[ofset] = tag;

    uint32_to_buf_msb(oad, tempbuf + ofset + 1);
    uint8_t first = nd_program_list_is_first(tempbuf[1]);
    tempbuf[1]++;

    index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr645_List);;
    ret = nd_set_index_port(port, index, &tempbuf[0], sizeof(tempbuf));

    if (first) //
    {
        ctrl_block->MeterWr_SetTime_flag = 0;
        if (nd_program_list_sets_time_645(oad))  //若第一个编程为校时,则编程开始时间取校时前时间
        {
            ctrl_block->MeterWr_SetTime_flag = 0x5A;
        }
        record_event_no_mesg_pro(instance, task, MeterWr645S, payload, 1);
    }
    else
    {
        record_event_no_mesg_pro(instance, task, MeterWr645S, payload, -1);
    }

    return 1;
}


// 645 编程事件
int trigger_MeterWr645_record(void *instance, uint32_t DI645, uint8_t tag)
{
    struct mbase_control_block_t* ctrl_block  = ((stategrid_context_t *)instance)->mbase;

    if(ctrl_block->factor_mode)
    {
        return 1; //厂内不记录 645编程事件
    }
    if ((DI645 == 0xA8000100))   return 1;  //进出工厂ID 不记录

    MeterWr645_obj_list_add(instance, NULL, NULL, DI645, 0);
    if(ctrl_block->metertype & F_Password_En) //明文设参下        每条只记录1个ID    
        MeterWr645_obj_list_clear(instance, NULL, NULL);
    return 0;
}
