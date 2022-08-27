#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "cpu_surport.h" //只包含事件状态宏



// 上电运行期间 每秒调用 
// 电源异常事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int PowerAbnormal_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    //uint8_t payload[100];
    int result = 0;

    uint8_t duration_para       =  1;//ctrl_block->PowerAbnormalS_duration_para;
    uint8_t *duration           = &ctrl_block->PowerAbnormalS_duration;
    uint32_t* accumulate_time   = &ctrl_block->PowerAbnormalS_acc_time;
    uint8_t state = false;

    uint16_t RecordId           = PowerAbnormalS;

    //上电只结束
    state = 0;
        
    result = event_duration_pro(duration_para, duration, state, instance, RecordId);

    if (result != 0)
    {
        send_event_mesg_pro(instance, task,  RecordId, NULL, result);
    }

    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;// 累计时间累加
    }

    return result;
}


//  下电状态 累计时间累加 
//  下电状态 每秒调用 
void PowerAbnormal_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t RecordId = PowerAbnormalS;

    int result = 0;

    uint8_t duration_para       =  1;//ctrl_block->CoverOpenS_duration_para;
    uint8_t *duration           = &ctrl_block->PowerAbnormalS_duration;
    uint32_t* accumulate_time   = &ctrl_block->PowerAbnormalS_acc_time;
    uint8_t state = 0;


    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;//  累计时间累加 
    }
    else
    {
        if( ( instance->ac_power_off_sec_cnt == 7 ) || ( instance->ac_power_off_sec_cnt == 8 ))
        {
            if (ctrl_block->PowerAbnormalS_status == EVENT_SET)
            {
                state = 1;
            }
            else
            {
                state = 0;
            }
    
            result = event_duration_pro(duration_para, duration, state, instance, RecordId);

            if (result == 1)//(result != 0) //停电只记录发生，上电结束
            {
                send_event_mesg_pro(instance, task, RecordId, NULL, result);
            }
        }
    }


}

