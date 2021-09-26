/**
 * @ingroup   demand  
 * @brief       
 *
 * @{
 * @file
 *
 */

#include "kernel\nidavellir.h"
#include "applications.h"

//每秒执行
uint8_t scond_execute_for_demand(struct demand_ctx_t* ctx, uint32_t power,uint8_t index)
{
    struct demand_info_t* info = NULL;

    if (power < 0)
    {
        power = 0 - power;
    }

    if ((NULL != ctx->info) && (index < ctx->pool_size))
    {
        info = &ctx->info[index];

        if (info->sec_cnt > 90)  //这里的90没有什么意义，判断出错使用，只要大于1分钟都是有异常的；
        {
            info->cumu_sec += power;
            info->sec_cnt++;
        }
        else
        {
            info->cumu_sec = 0x00;
            info->sec_cnt = 0x00;
        }
    }

    return 0x01;
}

static uint8_t minute_calculate(struct demand_info_t* info)
{
    if (info->sec_cnt != 0x00)
    {
        info->cumu_min += info->cumu_sec / info->sec_cnt;
        info->cumu_sec = 0x00;
        info->sec_cnt = 0x00;
    }

    return 0x01;
}

//这里有两种模式，模式0为，滑差周期到更新需量，模式1：需量周期到更新需量
uint8_t sliding_period_ok(struct demand_info_t* info, uint8_t mode)
{
    uint8_t ret = 0x00;

    if (info->min_cnt < info->sliding)
    {
        info->min_cnt++;
    }
    else if (0x00 != info->sliding)
    {
        if (info->slide_index >= info->slide_max)
        {
            info->slide[info->slide_index] = info->cumu_min / info->sliding;
        }
        else
        {
            info->slide_index = 0x00;
        }        
        info->min_cnt = 0x00;
        info->cumu_min = 0x00;
        //滑差寄存器索引更新
        info->slide_index++;
        if (info->slide_index > info->slide_max - 1)
        {
            info->slide_index = 0x00;
        }
        //滑差个数更新
        if (info->slide_cnt < info->slide_num)
        {
            info->slide_cnt++;
        }
        //更新费率需量的滑差个数；
        if (info->fee_cnt < info->slide_num)
        {
            info->fee_cnt++;
        }

        if (mode == 0x01)   //ret=01 需量和费率需量都更新，ret=02 只更新需量，ret=03 只更新费率需量
        {
            if (info->slide_cnt == info->slide_num)
            {
                ret = 0x02;   
            }

            if (info->fee_cnt == info->slide_num)
            {
                if (ret == 0x02)
                {
                    ret = 0x01;
                }
                else
                {
                    ret = 0x03;
                }             
            }
        }
        else
        {
            ret = 0x01;
        }
    }

    return ret;
}

static uint8_t update_demand(struct demand_info_t* info, uint8_t cnt, uint32_t* lngtmp)
{
    int8_t i = info->slide_index;
    uint32_t* ptr = info->slide[info->slide_index];

    while (cnt)
    {
        if (i == 0)
        {
            ptr = info->slide + (info->slide_num - 1);
        }
        else
        {
            ptr--;
        }

        *lngtmp += *ptr;
        i--;
        cnt--;
    }

    if (info->slide_num != 0)
    {
        *lngtmp = (*lngtmp / info->slide_num);
    }

    return 0x01;
}

//每分钟执行
uint8_t minute_execute_for_demand(struct demand_ctx_t* ctx, uint8_t index, uint8_t mode)
{
    struct demand_info_t* info = NULL;
    uint32_t lngtemp = 0, tlngtemp = 0;
    uint8_t ret = 0x00;

    if ((NULL != ctx->info) && (index < ctx->pool_size))
    {
        info = &ctx->info[index];
    }
    //更新分钟累计值
    minute_calculate(info);
    //判断滑差周期是否到了
    ret = sliding_period_ok(info, mode);
    if ((ret == 0x01) || (ret == 0x02))
    {
        update_demand(info, info->slide_cnt,&lngtemp);  //更新需量
    }

    if ((ret == 0x01) || (ret == 0x03))
    {
        update_demand(info, info->fee_cnt, &tlngtemp);  //更新费率需量
    }

    return ret;
}


uint8_t active_demand_sec(struct instance_t* instance, uint8_t index)
{
    struct mbase_control_block_t* mbase_block = instance->mbase_cb;
    struct demand_ctx_t* ctx = mbase_block->demand_ctx;
    scond_execute_for_demand(ctx, mbase_block->energy_ctx->pa, index);
    return 0x01;
}

uint8_t active_demand_min(struct instance_t* instance, uint8_t index, uint8_t mode)
{
    struct mbase_control_block_t* mbase_block = instance->mbase_cb;
    struct demand_ctx_t* ctx = mbase_block->demand_ctx;
    uint8_t ret;
    minute_execute_for_demand(ctx,index,mode);
}

uint8_t demand_calculation(struct instance_t* instance,uint8_t cycle)  //cycle = 0代表每秒执行，cycle = 1 代表每分钟执行
{
    struct mbase_control_block_t* mbase_block = instance->mbase_cb;
    struct demand_ctx_t* ctx = mbase_block->demand_ctx;
    uint8_t i = 0x00;

    for (i = 0x00; i < ctx->pool_size; i++)
    {
        if (cycle == 0x00)
        {
            ctx->pool[i]._scond(instance, ctx->pool->parameter_1);
        }
        else
        {
            ctx->pool[i]._scond(instance, ctx->pool->parameter_1, ctx->pool->parameter_2);
        }
    }
}


const struct demand_pool_t demand_pool[] =
{
    { active_demand_sec, active_demand_min, 0x00, 0x01 }
};

uint8_t demand_init(struct instance_t* instance)
{
    uint8_t i;
    struct demand_ctx_t* ctx;
    uint8_t demand_register = sizeof(demand_pool) / sizeof(demand_pool[0]);

    if (instance->mbase_cb != NULL)
    {
        instance->mbase_cb->demand_ctx = malloc(sizeof(struct demand_ctx_t));//内存初始化--todo
        ctx = instance->mbase_cb->demand_ctx;
        ctx->pool = demand_pool;
        ctx->pool_size = demand_register;

        ctx->info = malloc(sizeof(struct demand_info_t) * demand_register);
        for (i = 0; i < demand_register; i++)
        {
            ctx->info->slide_max = 15;
            ctx->info->slide = malloc(sizeof(uint32_t) * ctx->info->slide_max);
            ctx->info++;
        }
    }
}

/**@}*/