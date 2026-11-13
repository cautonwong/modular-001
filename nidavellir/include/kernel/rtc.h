#ifndef ND_KERNEL_RTC_H
#define ND_KERNEL_RTC_H

#include <stdint.h>
#include <time.h>

#include "object.h"

struct cron_t
{
    uint8_t h;
    uint8_t f;//分和秒的频率
    uint8_t ms;//分钟的开始
    uint8_t ss;//秒的开始
};

struct rtc_context_t
{
    struct object_t object;
    // 0 -- 正常   1--故障 仍未广播对时  2--故障 对对时中
    // todo  重发  状态迁移
    // waiting[跟基表建立通信  跟STA建立通信  跟蓝牙建立通信]
    // 5min < delta < 30min  -->  启动15分钟负荷记录
    // delta > 30min  --> 时段参数 -> 费率电量 -> 15分钟负荷和日冻结
    uint8_t tm_status;
    uint8_t addr[6];//基表的地址
    
    uint8_t CCO;//CCO已对时  过日后清掉
    struct tm meter_poweroff_tm;
    struct tm meter_poweron_tm;
    uint8_t meter_type;
    uint8_t meter_rtc_is_fault;
    uint8_t bauterate;
    uint8_t current;
    uint8_t second;
};

#endif
