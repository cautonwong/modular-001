#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"


#define CLEAR_WATCH_DOG() 

#define STEP_MONTH_MODE            0x00 /*月阶梯*/
#define STEP_YEAR_MODE             0x01 /*年阶梯*/


// ctrl_block  以下内容放到port.h
#if 1
struct step_bill_day_t
{
    uint8_t hour;   
    uint8_t day;     
    uint8_t mon;  
};

struct step_bill_day_t sbill_day[4]; // 阶梯结算日 MMDDhh

struct tm sbill_store_tm;// 阶梯结算时间存储 
struct tm sbill_last_tm;// 上一次阶梯结算时间 
uint8_t sbill_mode; //当前阶梯结算模式 0 月阶梯   1 年阶梯      待添加


struct tm v_powerdown_tm;// 模拟停电时间  补结算测试用
struct tm powerdown_tm;// 停电时间 


#endif



/*对4个阶梯结算日进行又小到大排序，并返回有效结算日个数*/
static uint8_t sort_step_bill_day(uint8_t *mode, uint8_t *ptr, struct pipeline_t* task,)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;

    uint8_t           i,dat,flag = 0;
    uint32_t          bill;

    *mode = STAIR_YEAR_MODE;
    for(i = 0; i < 4; i++)
    {
        bill = 0;
        memcopy((uint8_t *)&bill, &ctrl_block->sbill_day[i], 3);

        if ((bill != 0x636363)&&(bill != 0x999999)&&(bill != 0xFFFFFF))
        {
            flag |= (uint8_t)1 << i;
            dat = (uint8_t)(bill >> 16);
            if (dat < 1 || dat > 12)
            {
                *mode = STAIR_MONTH_MODE;
            }
        }
        memcopy(ptr + i * 3, (uint8_t *)&bill, 0x03);
    }
    
    return flag;
}




// 每秒调     返回0 表示无阶梯结算。 返回1表示结算。
uint8_t step_bill_pro(struct tm* time,struct pipeline_t* task, struct message_t* mesg)
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
    ctrl_block->sbill_last_tm.tm_sec = 0;
    
    now = rtc_mktime(&now_tm);
    last = rtc_mktime(&ctrl_block->sbill_last_tm);

    for (uint8_t i = 0; i < 4; i++)
    {
        if ((ctrl_block->sbill_day[i].mon == 0x63) && (ctrl_block->sbill_day[i].day == 0x63) && (ctrl_block->sbill_day[i].hour == 0x63)
            || (ctrl_block->sbill_day[i].mon == 0x99) && (ctrl_block->sbill_day[i].day == 0x99) && (ctrl_block->sbill_day[i].hour == 0x99)
            || (ctrl_block->sbill_day[i].mon == 0xFF) && (ctrl_block->sbill_day[i].day == 0xFF) && (ctrl_block->sbill_day[i].hour == 0xFF))
        {
            continue;
        }

        now_tm.tm_mday = ctrl_block->sbill_day[i].day;
        now_tm.tm_hour = ctrl_block->sbill_day[i].hour;
        
        if (ctrl_block->sbill_mode == 1) // 年阶梯
        {
            now_tm.tm_mon = ctrl_block->sbill_day[i].mon;
        }


        bill = rtc_mktime(now_tm);

        if ((now == bill)&&(last != bill))
        {
            memcopy((uint8_t *)&ctrl_block->sbill_last_tm,(uint8_t *)&now_tm, sizeof(now_tm));
            memcopy((uint8_t *)&ctrl_block->sbill_store_tm,(uint8_t *)&now_tm, sizeof(now_tm));
            return 1;  // 返回值1表示 结算
                       // 阶梯结算后清累计电量
        }        
    }
  
    return 0;
}


//  上电调用    ---补阶梯结算处理  
uint8_t step_bill_power_on_pro(struct tm* time, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;
    
    uint8_t cycle, i; 
    uint32  now,pd,last,recently;
    struct tm now_tm,pd_tm;

    uint8_t countday[12];
    struct step_bill_day_t sbill_day_tmp[4];


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
    ctrl_block->sbill_last_tm.tm_sec = 0;
    
    now = rtc_mktime(&now_tm);
    pd  = rtc_mktime(&pd_tm);
    last = rtc_mktime(&ctrl_block->mbill_last_tm);    

    recently  = pd;

    sort_step_bill_day(&ctrl_block->sbill_mode, (uint8_t *)sbill_day_tmp, task);

    while(recently <= now)
    {
        for (i = 0; i < 4; i++)
        {
            if ((ctrl_block->sbill_day[i].mon == 0x63) && (ctrl_block->sbill_day[i].day == 0x63) && (ctrl_block->sbill_day[i].hour == 0x63)
                || (ctrl_block->sbill_day[i].mon == 0x99) && (ctrl_block->sbill_day[i].day == 0x99) && (ctrl_block->sbill_day[i].hour == 0x99)
                || (ctrl_block->sbill_day[i].mon == 0xFF) && (ctrl_block->sbill_day[i].day == 0xFF) && (ctrl_block->sbill_day[i].hour == 0xFF))
            {
                continue;
            }

            pd_tm.tm_min = 0;
            pd_tm.tm_mday = ctrl_block->sbill_day[i].day;
            pd_tm.tm_hour = ctrl_block->sbill_day[i].hour;

            if (ctrl_block->sbill_mode == 1) // 年阶梯
            {
                pd_tm.tm_mon = ctrl_block->sbill_day[i].mon;
            }

            last   =  rtc_mktime(&ctrl_block->sbill_last_tm);  
            recently =  rtc_mktime(&pd_tm); 
            if (recently <= now)
            {
                if ((recently > last)&&(recently >= pd))
                {
                    memcopy((uint8_t *)&ctrl_block->sbill_last_tm,(uint8_t *)&pd_tm, sizeof(pd_tm)); // 刷新最近一次结算时间
                    memcopy((uint8_t *)&ctrl_block->sbill_store_tm,(uint8_t *)&pd_tm, sizeof(pd_tm));// 本条记录存储的时间
                    // 待添加
                    // 记录1次阶梯结算        不跳出  
                    
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

