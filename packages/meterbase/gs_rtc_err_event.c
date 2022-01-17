#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"


// 上电运行期间 每秒调用 
// 事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int RTCErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t payload[100];
    int8_t result = 0;

    uint8_t duration_para       = 0; //ctrl_block->RTCErrS_duration_para;
    uint8_t *duration           = &ctrl_block->RTCErrS_duration;
    //uint32_t* accumulate_time   = &ctrl_block->RTCErrS_acc_time;
    uint8_t state               =  ctrl_block->RTCErrS_status;

    uint16_t RecordId           = RTCErrS;

    result = event_duration_pro(duration_para, duration, state, instance, RecordId);

    if (result != 0)
    {
        send_event_mesg_pro(instance, task,  RecordId, NULL, result);
    }

#if 0
    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;// 累计时间累加 
    }
#endif

    return result;
}
