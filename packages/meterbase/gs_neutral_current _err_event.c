#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"

#define _value(a) a = ((int32_t)a < 0)? (int32_t)(-a) : (int32_t)a




// ctrl_block  以下内容放到port.h
#if 1
uint8_t In_err_event_duration_para;/*static:电能表零线电流异常事件-判定延时时间 */ \
uint32_t In_err_event_threshold1;/*电能表零线电流异常事件-电流触发下限 */ \
uint32_t In_err_event_threshold2;/*电能表零线电流异常事件-限值rate */ \

uint8_t In_err_event_duration;    /*dyn:当前零线电流异常状态持续时间*/\
uint8_t In_err_event_activeflag; /*发生标识*/ \
uint8_t In_err_event_recordflag; /*记录标识*/ \
uint8_t In_err_event_resetflag;  /*复位标识*/ \

uint32_t In_err_accumulate_time; /*累计时间*/ \


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

// 上电运行期间 每秒的调用 
// 掉电事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int In_err_event_judge(struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;

    int32_t Ia = _value(ctrl_block->Ia);
    int32_t In = _value(ctrl_block->In);    

    uint8_t duration_para = ctrl_block->In_err_event_duration_para;
    uint32_t threshold1 = ctrl_block->In_err_event_threshold1;     
    uint32_t threshold2 = ctrl_block->In_err_event_threshold2;    
    uint8_t *duration = &ctrl_block->In_err_event_duration;
    uint8_t *activeflag = &ctrl_block->In_err_event_activeflag;
    uint8_t *recordflag = &ctrl_block->In_err_event_recordflag;
    uint32_t *accumulate_time = &ctrl_block->In_err_accumulate_time;


    uint8_t state = false;
    int8_t result = 0;
    uint16 rate = 0;


    if (instance->Imax == 100) /*最电流小于10A 互感式表不启用*/
    {
        return 0;
    }

    if (threshold2 == 0) // 限值rate为0
    {
        return 0;
    }

    if((Ia < threshold1) && (In < threshold1))
    {
        return 0;
    }

    if(Ia >= In)
    {
        rate = (Ia - In) * 10000 /Ia;
    }
    else
    {
        rate = (In - Ia) * 10000 /In;
    }
      

    if(rate > threshold2 )
    {
        state = true;
    }
    
    result = event_duration_pro(duration_para, duration, activeflag, recordflag, state);

    
    if(activeflag == true)
    {
        (*accumulate_time)++;/* 累计时间累加 */
    }
    
    return result;

}


// 清零       返回1:清零                  0:不清零
int In_err_event_reset(struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;
    uint8_t reset_flag = ctrl_block->In_err_event_resetflag;
    uint32_t *accumulate_time = &ctrl_block->In_err_accumulate_time;

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


