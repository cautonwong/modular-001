#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"

#define PHASE_NUM 1   //单相或三相 支持1或3 



// 上电运行期间 每秒调用 
// 掉电事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int PowerOff_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t payload[100];
    int result = 0;
    uint8_t state = false;

    uint8_t duration_para = 4;//ctrl_block->PowerOffS_duration_para;
    uint8_t* duration = &ctrl_block->PowerOffS_duration;
    uint32_t* accumulate_time = &ctrl_block->PowerOffS_acc_time;
    uint16_t RecordId = PowerOffS;
    uint8_t i;

    uint32_t* U[PHASE_NUM] =
    {
       &ctrl_block->voltage_ac,
#if (PHASE_NUM == 3 ) 
       &ctrl_block->voltage_ac,
       &ctrl_block->voltage_ac
#endif       
    };
    uint32_t* val1;


    for (i = 0; i < PHASE_NUM; i++)
    {
        val1 = U[i];

        // 所有相均低于60%un
        if ((*val1) >= (ctrl_block->volt_60)*100)
        {
            break;
        }

    }

    if (i == PHASE_NUM)
    {
        state = true;
    }

    result = event_duration_pro(duration_para, duration, state, instance, RecordId);

    if (result != 0)
    {
        send_event_mesg_pro(instance, task,  RecordId, NULL, result);
    }

    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;//  累计时间累加 
    }

    return result;
}


//  下电状态 累计时间累加 
//  下电状态 每秒调用 
void PowerOff_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint32_t* accumulate_time = &ctrl_block->PowerOffS_acc_time;
    uint16_t RecordId = PowerOffS;


     // 是否为停电下第一秒 // 待调整   //  停电下第一秒记录一次掉电事件 
    if ( ctrl_block->power_down_sec_accum == 1 )
    {
        if (false == get_activeflag(instance, RecordId))
        {
            send_event_mesg_pro(instance, task,  RecordId, NULL, 1);
        }
    }

    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;//  累计时间累加 
    }
}

