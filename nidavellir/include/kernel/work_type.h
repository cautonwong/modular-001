#ifndef ND_KERNEL_WORK_TYPE_H
#define ND_KERNEL_WORK_TYPE_H


#define NIDAVELLIR_EPOCH 2000


enum work_type_t 
{
    PERIOD_SEC = 0,//秒变化
    PERIOD_MIN,//分变化
    PERIOD_HOUR,//时变化
    PERIOD_MDAY,//日变化
    PERIOD_MON,//月变化
    PERIOD_WDAY,//周变化
    PERIOD_YEAR,//年变化
    //以上部分放在rtc里面去做吧
    TIMER,//定时器到期  用于超时之类的   在定时器或计数器里做
    DATA_ARRIVE,//数据到达     串口中断
    DATA_PUSH,//数据到达     主动发送请求
    //POWER_CHANGE,//电源状态发生改变
};

#endif
