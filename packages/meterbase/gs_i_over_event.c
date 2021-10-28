#include "kernel\nidavellir.h"
#include "applications.h"

static int event_duration_pro(uint8_t duration_para,uint8_t *duration, uint8_t *activeflag, uint8_t *recordflag, uint8_t state)
{
    if(state) // 发生
    {
        if (true == *activeflag)
        {
            *duration = duration_para;
        }
        
        if (*duration < duration_para)
        {
            (*duration)++;
        }
        else if (false == *activeflag)
        {
            if(activeflag)
            {
                *activeflag = true;
            }
            
            *recordflag = true;
            return 1; // 记录发生
        }
    }
    else // 结束
    {
        if (false == *activeflag)
        {
            *duration = 0;
        }
        
        if (*duration > 0)
        {
            (*duration)--;
        }
        else if (true == *activeflag)
        {
            if(activeflag)
            {
                *activeflag = false;
            }
            
            *recordflag = true;
            return -1; // 记录结束
        }
    }

    return 0; // 不记录
}

// 上电运行期间 每秒的调用 
// 掉电事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int ia_over_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;

    uint32_t *I = &ctrl_block->Ia;
    uint8_t state = false;
    int8_t result = 0;

    uint8_t duration_para = ctrl_block->i_over_event_duration_para;
    uint32_t threshold = ctrl_block->i_over_event_threshold;     
    uint8_t *duration = &ctrl_block->ia_over_event_duration;
    uint8_t *activeflag = &ctrl_block->ia_over_event_activeflag;
    uint8_t *recordflag = &ctrl_block->ia_over_event_recordflag;
    uint32_t *accumulate_time = &ctrl_block->ia_over_accumulate_time;

    
    if (threshold == 0) // 限值为0
    {
        return 0;
    }

    // 所有相均低于60%un
    if((*I) > threshold)
    {
        state = true;
    }

    result = event_duration_pro(duration_para, duration, activeflag, recordflag, state);

    if(*activeflag == true)
    {
        (*accumulate_time)++;/* 累计时间累加 */
    }
    
    return result;

}



// 清零       返回1:清零                  0:不清零
int ia_over_event_reset(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t reset_flag = ctrl_block->ia_over_event_resetflag;
    uint32_t *accumulate_time = &ctrl_block->ia_over_accumulate_time;

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


