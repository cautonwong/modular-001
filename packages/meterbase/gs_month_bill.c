#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"


#define CLEAR_WATCH_DOG() 

// ctrl_block  以下内容放到port.h
#if 1
struct mbill_day_t
{
    uint8_t hour;   
    uint8_t day;     
};

struct mbill_day_t mbill_day[3]; // 结算日 DDhh

struct tm mbill_store_tm;// 结算时间存储 
struct tm mbill_last_tm;// 上一次结算时间 

struct tm v_powerdown_tm;// 模拟停电时间  补结算测试用
struct tm powerdown_tm;// 停电时间 


#endif


// 每秒调     返回0 表示无月结算。 返回1，2，3表示1，2，3月结算。
uint8_t mbill_pro(struct tm* time,struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;

    uint32_t now,last,bill;
    struct tm now_tm;
    
    if (time->tm_min != 0)
    {
        return 0;
    }

    memcopy((uint8_t *)&now_tm,(uint8_t *)time, sizeof(now_tm));
    now_tm.tm_sec = 0;
    ctrl_block->mbill_last_tm.tm_sec = 0;
    
    now = rtc_mktime(&now_tm);
    last = rtc_mktime(&ctrl_block->mbill_last_tm);

    for (uint8_t i = 0; i < 3; i++)
    {
        if ((ctrl_block->mbill_day[i].day == 0x63) && (ctrl_block->mbill_day[i].hour == 0x63)
            || (ctrl_block->mbill_day[i].day == 0x99) && (ctrl_block->mbill_day[i].hour == 0x99)
            || (ctrl_block->mbill_day[i].day == 0xFF) && (ctrl_block->mbill_day[i].hour == 0xFF))
        {
            continue;
        }

        now_tm.tm_mday = ctrl_block->mbill_day[i].day;
        now_tm.tm_hour = ctrl_block->mbill_day[i].hour;

        bill = rtc_mktime(now_tm);

        if ((now == bill)&&(last != bill))
        {
            memcopy((uint8_t *)&ctrl_block->mbill_last_tm,(uint8_t *)&now_tm, sizeof(now_tm));
            memcopy((uint8_t *)&ctrl_block->mbill_store_tm,(uint8_t *)&now_tm, sizeof(now_tm));
            return (i+1);  // 返回值表示 第1-3个结算
                           // 第1结算日过后，最大需量清零                是否需要发送消息？             待添加
                           // 第2，3结算日，需量填空。
        }        
    }
  
    return 0;
}


//  上电调用    ---补月结算处理  
uint8_t mbill_power_on_pro(struct tm* time, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;
    
    uint8_t cycle, i; 
    uint32  now,pd,last,recently;
    struct tm now_tm,pd_tm;


    memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->v_powerdown_tm), sizeof(ctrl_block->v_powerdown_tm));

    if (false == rtc_tm_valid(&ctrl_block->v_powerdown_tm))    // 模拟停电时间无效          取上次停电时间
    {
        if (false == rtc_tm_valid(&ctrl_block->powerdown_tm))   // 停电时间无效       退出
        {
            return 0;
        }
        memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->powerdown_tm), sizeof(ctrl_block->powerdown_tm));
    }

    memcopy((uint8_t *)&now_tm,(uint8_t *)time, sizeof(now_tm));
    now_tm.tm_sec = 0;
    pd_tm.tm_sec = 0;
    ctrl_block->mbill_last_tm.tm_sec = 0;
    
    now = rtc_mktime(&now_tm);
    pd  = rtc_mktime(&pd_tm);
    last = rtc_mktime(&ctrl_block->mbill_last_tm);    

    recently  = pd;

    while(recently <= now)
    {
        for (i = 0; i < 3; i++)
        {
            if ((ctrl_block->mbill_day[i].day == 0x63) && (ctrl_block->mbill_day[i].hour == 0x63)
                || (ctrl_block->mbill_day[i].day == 0x99) && (ctrl_block->mbill_day[i].hour == 0x99)
                || (ctrl_block->mbill_day[i].day == 0xFF) && (ctrl_block->mbill_day[i].hour == 0xFF))
            {
                continue;
            }

            pd_tm.tm_min = 0;
            pd_tm.tm_mday = ctrl_block->mbill_day[i].day;
            pd_tm.tm_hour = ctrl_block->mbill_day[i].hour;

            last   =  rtc_mktime(&ctrl_block->mbill_last_tm);  
            recently =  rtc_mktime(&pd_tm); 
            if (recently <= now)
            {
                if ((recently > last)&&(recently >= pd))
                {
                    memcopy((uint8_t *)&ctrl_block->mbill_last_tm,(uint8_t *)&pd_tm, sizeof(pd_tm)); // 刷新最近一次结算时间
                    memcopy((uint8_t *)&ctrl_block->mbill_store_tm,(uint8_t *)&pd_tm, sizeof(pd_tm));// 本条记录存储的时间
                    // 待添加
                    // 记录1次结算       需根据i判断需量结算是否填空，
                    // 第1结算日过后，最大需量清零                是否需要发送消息？             待添加
                }
            }
            else
            {
                return 0;
            }
        }    

        CLEAR_WATCH_DOG();

        if (pd_tm.tm_mon < 0x0c)
        {
            pd_tm.tm_mon++;
        }
        else
        {
            pd_tm.tm_mon  = 0x01;
            pd_tm.tm_year  += 0x01;
        }

        /*最多补5年的数据*/
        if (++cycle > 60)
        {
            return 0;
        }  
    }

    return 0; 
}

