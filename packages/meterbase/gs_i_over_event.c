#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
uint8_t test_record_ = 0;
// 上电运行期间 每秒的调用 
// 掉电事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int ia_over_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t payload[100];
    uint8_t state = false;
    int8_t result = 0;

    uint8_t duration_para       = ctrl_block->OverCurrent_duration_para;  
    uint8_t *duration           = &ctrl_block->OverCurrentAS_duration;
    uint32_t* accumulate_time   = &ctrl_block->OverCurrentAS_acc_time;
    
    uint16_t RecordId           = OverCurrentAS;

    int32_t* I                  = &ctrl_block->Ia;
    uint32_t threshold          = ctrl_block->OverCurrent_threshold;

    if (threshold == 0) // 限值为0
    {
        return 0;
    }

    // 所有相均低于60%un
    if((*I) > threshold)
    {
        state = true;
    }

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

// 清零       返回1:清零                  0:不清零
int ia_over_event_reset(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t reset_flag = ctrl_block->OverCurrentAS_resetflag;
    uint32_t *accumulate_time = &ctrl_block->OverCurrentAS_acc_time;

    switch (mesg->id)
    {
	    case 0xAAAA:// 待调整 //获取事件复位标识             标识需由通信触发，记录写完后清零？？？
	        assert(mesg->len == 8);
	        //ringbuffer_getU32(task->std_in, &reset_flag);
	        break;
    }

	if (reset_flag == true)
	{
	    *accumulate_time = 0; /* 累计时间清零*/
		return 1;
	}
	else
	{
		return 0;
	}
}


