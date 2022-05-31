#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"



// 上电运行期间 每秒调用 
// 负荷开关误动作 事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int SwitchErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t payload[100];
    uint8_t state = false;
    int result = 0;

    uint8_t duration_para = 5;//ctrl_block->SwitchErrS_duration_para;
    uint8_t *duration = &ctrl_block->SwitchErrS_duration;

    uint8_t err_flag = ctrl_block->SwitchErrS_status;
    uint8_t enable_flag = ctrl_block->SwitchErrS_Record_En;

    uint32_t* accumulate_time = &ctrl_block->SwitchErrS_acc_time;

    uint16_t RecordId = SwitchErrS;


    if((err_flag == true) && (enable_flag == true))
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
        (*accumulate_time)++;// 累计时间累加
    }

    return result;

}
