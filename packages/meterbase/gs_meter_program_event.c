#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "resource_tool.h"


//编程事件 


// 编程列表清零
int MeterWr_obj_list_clear(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    uint8_t tempbuf[52];
    uint32_t index = 0;
    int ret;

    memset(tempbuf, 0xff, sizeof(tempbuf));
    tempbuf[0] = 0x01;
    tempbuf[1] = 0;

    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr_List);;
    ret = nd_set_index(instance, index, &tempbuf[0], sizeof(tempbuf));
    return ret;
}

// 更新编程列表
int MeterWr_obj_list_add(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint32_t oad, uint8_t tag)
{
    struct mbase_control_block_t* ctrl_block  = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t payload[100];
    uint8_t ofset, tempbuf[52];
    uint32_t index = 0;
    int ret;

    memset(tempbuf, 0, sizeof(tempbuf));

    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr_List);;
    ret = nd_get_index(instance, index, &tempbuf[0], sizeof(tempbuf));

    if (ret < 0)
    {
        MeterWr_obj_list_clear(instance, task, mesg);
        tempbuf[0] = 0x01;
        tempbuf[1] = 0;
    }

    if (tempbuf[1] > 10)
    {
        tempbuf[1] = 0;
    }

    if (tempbuf[1] == 10)
    {
        tempbuf[1] = 9;
    }
    
    ndv_memcopy(&tempbuf[7], &tempbuf[2], sizeof(tempbuf) - 7);

    ofset = 2;
    tempbuf[ofset] = tag;

    uint32_to_buf_msb(oad, tempbuf + ofset + 1);
    tempbuf[1]++;

    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr_List);;
    ret = nd_set_index(instance, index, &tempbuf[0], sizeof(tempbuf));

    if (tempbuf[1] <= 1) //
    {
        ctrl_block->MeterWr_SetTime_flag = 0; 
        if (oad == 0x40000200)  //若第一个编程为校时,则编程开始时间取校时前时间
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
int MeterWr645_obj_list_clear(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    uint8_t tempbuf[52];
    uint32_t index = 0;
    int ret;

    memset(tempbuf, 0xff, sizeof(tempbuf));
    tempbuf[0] = 0x01;
    tempbuf[1] = 0;

    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr645_List);;
    ret = nd_set_index(instance, index, &tempbuf[0], sizeof(tempbuf));
    return ret;
}

// 更新编程列表
int MeterWr645_obj_list_add(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint32_t oad, uint8_t tag)
{
    struct mbase_control_block_t* ctrl_block  = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t payload[100];
    uint8_t ofset, tempbuf[52];
    uint32_t index = 0;
    int ret;

    memset(tempbuf, 0, sizeof(tempbuf));

    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr645_List);;
    ret = nd_get_index(instance, index, &tempbuf[0], sizeof(tempbuf));

    if (ret < 0)
    {
        MeterWr645_obj_list_clear(instance, task, mesg);
        tempbuf[0] = 0x01;
        tempbuf[1] = 0;
    }

    if (tempbuf[1] > 10)
    {
        tempbuf[1] = 0;
    }

    if (tempbuf[1] == 10)
    {
        tempbuf[1] = 9;
    }
    
    ndv_memcopy(&tempbuf[7], &tempbuf[2], sizeof(tempbuf) - 7);

    ofset = 2;
    tempbuf[ofset] = tag;

    uint32_to_buf_msb(oad, tempbuf + ofset + 1);
    tempbuf[1]++;

    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr645_List);;
    ret = nd_set_index(instance, index, &tempbuf[0], sizeof(tempbuf));

    if (tempbuf[1] <= 1) //
    {
        ctrl_block->MeterWr_SetTime_flag = 0; 
        if ((oad == 0x04000101) || (oad == 0x04000102) || (oad == 0x0400010C))  //若第一个编程为校时,则编程开始时间取校时前时间
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
int trigger_MeterWr645_record(struct instance_t* instance, uint32_t DI645, uint8_t tag)
{
    struct mbase_control_block_t* ctrl_block  = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    if(ctrl_block->factor_mode)
    {
        //return 1; //厂内不记录 645编程事件
    }
    if ((DI645 == 0xA8000100))   return 1;  //进出工厂ID 不记录

    MeterWr645_obj_list_add(instance, NULL, NULL, DI645, 0);
    //if(ctrl_block->metertype & ) //明文设参下        每条只记录1个ID                  待调整    带认证情况下编程记录   未完成。
    MeterWr645_obj_list_clear(instance, NULL, NULL);
}
