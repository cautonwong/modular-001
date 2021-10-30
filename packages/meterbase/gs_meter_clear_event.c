#include "nidavellir.h"
#include "applications.h"

// 电表清零事件 判断:  返回1:记录发生                0:不记录
int meterclr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;

    //工厂模式下 不记录清零事件
    if (ctrl_block->factor_mode == true)
    {
        return 0;
    }

    switch (mesg->id)
    {
        case 0xAAAA:// 待调整 //获取事件记录标识             标识需由通信触发，记录写完后清零？？？
            assert(mesg->len == 8);
            //ringbuffer_getU32(task->std_in, &ctrl_block->meterclr_event_recordflag);
            break;
    }

    if (ctrl_block->meterclr_event_recordflag == true)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// 电表清零事件 复位  返回1:复位清零                  0:不清零
int meterclr_event_reset(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t reset_flag = ctrl_block->meterclr_event_resetflag;

    switch (mesg->id)
    {
	    case 0xAAAA:// 待调整 //获取事件复位标识             标识需由通信触发，记录写完后清零？？？
	        assert(mesg->len == 8);
	        //ringbuffer_getU32(task->std_in, &reset_flag);
	        break;
    }

	if (reset_flag == true)
	{
		return 1;
	}
	else
	{
		return 0;
	}

}

