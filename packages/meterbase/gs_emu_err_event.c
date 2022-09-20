#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"


// 上电运行期间 每秒调用 
// 事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int EmuErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    //uint8_t payload[100];
    int8_t result = 0;

    uint8_t duration_para       =  ctrl_block->EmuErrS_duration_para;
    uint8_t *duration           = &ctrl_block->EmuErrS_duration;
    uint32_t* accumulate_time   = &ctrl_block->EmuErrS_acc_time;
    uint8_t state               = 0;// ctrl_block->EmuErrS_status;

    uint16_t RecordId           = EmuErrS;


    if (ctrl_block->EmuErrS_status == EVENT_SET)
    {
        state = 1;
    }
    else
    {
        state = 0;
    }

    result = event_duration_pro(duration_para, duration, state, instance, RecordId);

    if (result != 0)
    {
        send_event_mesg_pro(instance, task,  RecordId, NULL, result);
    }

    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;/* 累计时间累加 */
    }

    return result;
}
