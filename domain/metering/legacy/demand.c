/**
 * @ingroup   demand  
 * @brief       
 *
 * @{
 * @file
 *
 */

#include "nidavellir.h"
#include "applications.h"

//每秒执行
uint8_t scond_execute_for_demand(struct mbase_control_block_t* mbase_block)
{
    struct demand_list_t* list = mbase_block->demand_list;
    uint32_t this_power = 0;

    while (NULL != list)
    {
        this_power = *list->import;
        if (this_power < 0)
        {
            this_power = 0 - this_power;
        }
        //这里的90没有什么意义，判断出错使用，只要大于1分钟都是有异常的；
        if (list->info->sec_cnt > 90)  
        {
            list->info->cumu_sec += this_power;
            list->info->sec_cnt++;
        }
        else
        {
            list->info->cumu_sec = 0x00;
            list->info->sec_cnt = 0x00;
        }

        list = list->next;
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
uint8_t minute_execute_for_demand(struct mbase_control_block_t* mbase_block)
{
    struct demand_list_t* list = mbase_block->demand_list;
    uint32_t lngtemp = 0, tlngtemp = 0;
    uint8_t ret = 0x00;

    while (NULL != list)
    {
        //更新分钟累计值
        minute_calculate(list->info);
        //判断滑差周期是否到了
        ret = sliding_period_ok(list->info, list->mode);
        if ((ret == 0x01) || (ret == 0x02))
        {
            update_demand(list->info, list->info->slide_cnt, &lngtemp);  //更新需量
        }

        if ((ret == 0x01) || (ret == 0x03))
        {
            update_demand(list->info, list->info->fee_cnt, &tlngtemp);  //更新费率需量
        }
        list = list->next;
    }

    return ret;
}

//import 为输入及功率，output为输出的OAD，mode是模式：mode0 滑差周期到更新需量，mode1为需量周期到更新需量
int8_t add_demand_action(struct mbase_control_block_t* mbase_block, uint32_t* import, uint32_t output, uint8_t max_slide, uint8_t mode)
{
    struct demand_list_t* p = NULL, * list = mbase_block->demand_list;
    uint16_t s_num = 0x00;

    p = malloc(sizeof(struct demand_list_t));//申请内存

    if (p != NULL)
    {
        p->info = malloc(sizeof(struct demand_info_t));
        if (p->info == NULL)
        {
            return -1;
        }
        p->info->slide_max = max_slide;
        p->info->slide = malloc(sizeof(uint32_t) * max_slide);
        if (p->info->slide == NULL)
        {
            return -1;
        }
        p->import = import;
        p->output_id = output;
        p->mode = mode;
        p->next = NULL;                             

        if (list == NULL)                            
        {
            mbase_block->demand_list = p;                
        }
        else                                         
        {
            while (list->next != NULL)               
            {
                list = list->next;                   
            }
            list->next = p;                         
        }
        return 1;
    }
    return -1;
}

/**@}*/