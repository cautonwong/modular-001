#ifndef __APPLICATIONS_H__
#define __APPLICATIONS_H__

//当前系统中所有的application注册表容器

struct energy_ctx_t
{
    uint32_t pa;
};

struct demand_pool_t
{
    uint8_t(*_scond)(struct instance_t* instance, uint8_t index);
    uint8_t(*_minute)(struct instance_t* instance, uint8_t index, uint8_t mode);
    uint8_t parameter_1;
    uint8_t parameter_2;
};

struct demand_info_t
{
    uint32_t  cumu_demand;  //当前需量(需量周期到了更新)
    uint32_t  cumu_min;     //每分钟累计 滑差周期到了清0
    uint32_t  cumu_sec;     //每秒钟累计 分钟到了清0
    uint32_t* slide;        //滑差寄存器
    uint32_t  sec_cnt;      //秒计数器，计算每分钟的平均值，每个滑差周期到了清0
    uint8_t   min_cnt;      //分钟计数器，计数每个滑差周期内的平均值
    uint8_t   slide_max;    //滑差寄存器最大值,初始化的时候赋值
    uint8_t   slide_num;    //滑差个数
    uint8_t   sliding;      //滑差周期，单位是分钟
    uint8_t   slide_index;  //滑差寄存器的索引
    uint8_t   slide_cnt;    //滑差寄存器的个数
    uint8_t   fee_cnt;      //费率需量的滑差寄存器的计数器；
    uint8_t   cur_fee;      //当前的费率，这里需要去掉，直接去instance拿，todo
    uint8_t   pre_fee;      //上一个费率
};

struct demand_ctx_t
{
    const struct demand_pool_t* pool;
    uint8_t pool_size;
    struct demand_info_t* info;
};

struct mbase_control_block_t
{
    int id;
    //电量数据
    //瞬时量数据
    //需量数据
    //事件负荷冻结数据
    //费率数据
    //费用数据
    struct energy_ctx_t* energy_ctx;
    struct demand_ctx_t* demand_ctx;
};

uint8_t demand_calculation(struct instance_t* instance, uint8_t cycle);
uint8_t active_demand_sec(struct instance_t* instance, uint8_t index);
uint8_t active_demand_min(struct instance_t* instance, uint8_t index, uint8_t mode);

#endif
