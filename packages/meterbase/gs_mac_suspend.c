#include "nidavellir.h"
#include "applications.h"

void mac_err_cnt_add(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb->mac_err_cnt < 200)
    {
        mbase_cb->mac_err_cnt++;
        if (mbase_cb->mac_err_cnt == 200)  // 每日超200次
        {
            mbase_cb->mac_suspend_state = true; // 挂起
            mbase_cb->mac_suspend_date[0] = (uint8_t)instance->time.tm_mday;
            mbase_cb->mac_suspend_date[1] = (uint8_t)instance->time.tm_mon;
            mbase_cb->mac_suspend_date[2] = (uint8_t)instance->time.tm_year;
        }
    }    
}


void mac_suspend_reset(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    mbase_cb->mac_suspend_state = false; 
    mbase_cb->mac_suspend_date[0] = 0;
    mbase_cb->mac_suspend_date[1] = 0;
    mbase_cb->mac_suspend_date[2] = 0;
    mbase_cb->mac_err_cnt = 0;
    
    
    mbase_cb->password_err_cnt = 0;     //密码锁清零
}


void mac_suspend_sec_pro(struct instance_t* instance, struct pipeline_t* task)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);


    if (mbase_cb->mac_err_cnt > 0) 
    {
        if (((0 == instance->time.tm_hour) && (0 == instance->time.tm_min) && (0 == instance->time.tm_sec))
            ||((mbase_cb->mac_suspend_date[0]) && ((mbase_cb->mac_suspend_date[0] != (uint8_t)instance->time.tm_mday) || (mbase_cb->mac_suspend_date[1] != (uint8_t)instance->time.tm_mon)  || (mbase_cb->mac_suspend_date[2] != (uint8_t)instance->time.tm_year))))
        {
            mac_suspend_reset(instance);
        }
    }
    
    mbase_cb->mac_suspend_state = false;
    if (mbase_cb->mac_err_cnt == 200) 
    {
        mbase_cb->mac_suspend_state = true;
    }
}

