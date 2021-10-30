#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"

#define PHASE_NUM 1   /*单相或三相 支持1或3 */\


// ctrl_block  以下内容放到port.h
#if 1
uint8_t powerdown_event_duration_para = 4;/*static:电能表掉电事件-判定延时时间 */\
uint8_t powerdown_event_duration;    /*dyn:当前掉电状态持续时间*/\

uint8_t powerdown_event_activeflag; /*发生标识*/
uint8_t powerdown_event_recordflag; /*记录标识*/
uint8_t powerdown_event_resetflag;  /*复位标识*/

uint32_t powerdown_accumulate_time; /*累计时间*/

//外部模块
uint16_t  u_60; /**< 临界电压 60%电压  0.01V */ \

#endif



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

// 上电运行期间 每秒调用 
// 掉电事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int powerdown_event_judge(struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;

    uint16_t *U[PHASE_NUM] = 
    { 
       &ctrl_block->Ua,
#if (PHASE_NUM == 3 ) 
       &ctrl_block->Ub,
       &ctrl_block->Uc
#endif       
    };
    uint16_t *val1;
    uint8_t state = false;
    int8_t result = 0;

    uint8_t duration_para = ctrl_block->powerdown_event_duration_para;
    uint8_t *duration = &ctrl_block->powerdown_event_duration;
    uint8_t *activeflag = &ctrl_block->powerdown_event_activeflag;
    uint8_t *recordflag = &ctrl_block->powerdown_event_recordflag;


    for(int i = 0; i < PHASE_NUM; i++)
    {
        val1 = U[i];
        
        // 所有相均低于60%un
        if((*val1) >= (ctrl_block->u_60))
        {
            break;
        }
        
    }
    
    if(i == PHASE_NUM)
    {
        state = true;
    }

    result = event_duration_pro(duration_para, duration, activeflag, recordflag, state);
    
    return result;

}


/* 停电下第一秒记录一次掉电事件 */
//发送方直接存


/* 累计时间累加 */
/* 下电状态 每秒调用 */
static void powerdown_accumulate_time_add_sec(struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;
    uint32_t *accumulate_time = &ctrl_block->powerdown_accumulate_time;
    uint8_t activeflag = ctrl_block->powerdown_event_activeflag;

    if(activeflag == true)
    {
        (*accumulate_time)++;
    }
}


// 掉电事件 清零       返回1:清零                  0:不清零
int powerdown_event_reset(struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;
    uint8_t reset_flag = ctrl_block->powerdown_event_resetflag;
    uint32_t *accumulate_time = &ctrl_block->powerdown_accumulate_time;

    switch (mesg->id)
    {
	    case 0xAAAA:// 待调整 //获取事件复位标识             标识需由通信触发，记录写完后清零？？？
	        assert(mesg->len == 8);
	        ringbuffer_getU32(task->std_in, &reset_flag);
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


