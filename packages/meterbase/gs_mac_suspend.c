#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"

// ctrl_block  以下内容放到port.h
#if 1
uint8_t mac_err_cnt; // 当日mac错误次数
uint8_t mac_suspend_date[3];// mac错误 挂起日期 yymmdd

uint8_t mac_suspend_state = false;  // mac挂起状态 协议或显示模块可调用。 true为挂起
#endif

// mac挂起处理         每秒调用     过零点清零。
uint8_t mac_suspend_pro(struct tm* time,struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;
    
    uint8_t err_flag = false;
    uint8_t clr = 0;
    
    switch (mesg->id)
    {
        case 0xAAAA:// 待调整 //收到mac错误消息
            assert(mesg->len == 8);
            ringbuffer_getU32(task->std_in, &err_flag);
        break;
    }

    if (err_flag)
    {
        if(ctrl_block->mac_err_cnt < 200)
        {
            ctrl_block->mac_err_cnt++;
            if (ctrl_block->mac_err_cnt == 200)  // 每日超200次
            {
                ctrl_block->mac_suspend_state = true; // 挂起
                ctrl_block->mac_suspend_date[0] = (uint8_t)*time->tm_mday;
                ctrl_block->mac_suspend_date[1] = (uint8_t)*time->tm_mon;
                ctrl_block->mac_suspend_date[2] = (uint8_t)*time->tm_year;
            }
        }
    }

    //时间改变超过天
    if ((ctrl_block->mac_suspend_state == true)
        &&((ctrl_block->mac_suspend_date[0] != (uint8_t)*time->tm_mday) || (ctrl_block->mac_suspend_date[1] != (uint8_t)*time->tm_mon)  || (ctrl_block->mac_suspend_date[2] != (uint8_t)*time->tm_year)))
    {
        clr += 1;
    }

    // 过零点
    if ((*time->tm_hour == 0) && (*time->tm_min == 0) && (*time->tm_sec == 0))
    {
        clr += 1;
    }

    if (clr)  // 挂起清零
    {
        ctrl_block->mac_suspend_state = false; 
        ctrl_block->mac_suspend_date[0] = 0;
        ctrl_block->mac_suspend_date[1] = 0;
        ctrl_block->mac_suspend_date[2] = 0;
        ctrl_block->mac_err_cnt = 0;
    }
}


