#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"


// 上电运行期间 每秒调用 
// 事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int CoverOpen_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t payload[100];
    int8_t result = 0;

    uint8_t duration_para       =  ctrl_block->CoverOpen_duration_para;
    uint8_t *duration           = &ctrl_block->CoverOpen_duration;
    uint32_t* accumulate_time   = &ctrl_block->CoverOpen_acc_time;
    uint8_t state               =  ctrl_block->CoverOpen_status;

    uint16_t RecordId           = CoverOpenS;

    result = event_duration_pro(duration_para, duration, state, task, RecordId);

    if (result != 0)
    {
        send_event_mesg_pro(instance, task, mesg, RecordId, NULL, result);
    }

    if (true == get_activeflag(task, RecordId))
    {
        (*accumulate_time)++;/* 累计时间累加 */
    }

    return result;
}


// 事件 清零       返回1:清零                  0:不清零
int CoverOpen_reset(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint32_t *accumulate_time = &ctrl_block->CoverOpen_acc_time;

	*accumulate_time = 0; /* 累计时间清零*/
	return 1;
}


