#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
uint8_t test_record_ = 0;
// 上电运行期间 每秒的调用 
// 掉电事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int ia_over_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    //uint8_t payload[100];
    uint8_t state = false;
    int result = 0;

    uint8_t duration_para       = ctrl_block->OverCurrent_duration_para;  
    uint8_t *duration           = &ctrl_block->OverCurrentAS_duration;
    uint32_t* accumulate_time   = &ctrl_block->OverCurrentAS_acc_time;
    
    uint16_t RecordId           = OverCurrentAS;

    int32_t* I                  = &ctrl_block->current_l;
    uint32_t threshold          = ctrl_block->OverCurrent_threshold / 10;

    if (threshold == 0) // 限值为0
    {
        return 0;
    }

    // 所有相均低于60%un
    if((*I) > threshold)
    {
        state = true;
    }

    result = event_duration_pro(duration_para, duration, state, instance, RecordId);

    if (result != 0)
    {
        send_event_mesg_pro(instance, task, RecordId, NULL, result);
    }

    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;/* 累计时间累加 */
    }

    return result;
}



