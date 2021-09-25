#include "nidavellir.h"

#define POWER_ON 0
#define RTC_ERROR  1

struct route_t 
{
    uint32_t state;//上电时  时钟正常   时钟故障时  掉电时
    uint32_t num;
    struct object_t* objects;
    struct fifo_t* fifo;//目的fifo    
};

struct router_t
{
    uint8_t state;
    struct route_t *routes[]
};

struct fifo_t* get_fifo(struct instance_t* instance, uint32_t state, struct object_t* object)
{

}

//上电时的任务 
{ 0, pipe_rtc, 0x04000401, pipe_msta_tx_to_meter }，
{ 0, pipe_rtc, 0x04000401, pipe_msta_tx_to_meter }，
{ 0, pipe_rtc, 0x04000401, pipe_msta_tx_to_meter }，

struct route_t routes[] =
{
    {0, 0x0400}
};

struct route_t route_baterate =
{
    0,//上电
    1,
    0x04000401,//基表地址   用于波特率自适应  成功后才能正常工作
    &pipe_msta_tx_to_meter,
};
