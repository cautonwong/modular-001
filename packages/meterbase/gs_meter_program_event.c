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
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
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
    
    memcpy(&tempbuf[7], &tempbuf[2], sizeof(tempbuf) - 7);

    ofset = 2;
    tempbuf[ofset] = tag;

    uint32_to_buf_msb(oad, tempbuf + ofset + 1);
    tempbuf[1]++;

    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr_List);;
    ret = nd_set_index(instance, index, &tempbuf[0], sizeof(tempbuf));

    if (tempbuf[1] <= 1)
    {
        if (oad == 0x40000200)  //若第一个编程为校时,则编程开始时间取校时前时间
        {
            ctrl_block->MeterWr_SetTime_flag = 0x5A; //待调整
        }
        send_event_mesg_pro(instance, task, MeterWrS, payload, 1);
    }
    else
    {
        send_event_mesg_pro(instance, task, MeterWrS, payload, -1);
        //programing_event_pro(buf);  //编程对象如何改写上一条记录
    }

    return 1;
}
