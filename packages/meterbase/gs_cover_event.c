#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "cpu_surport.h" //只包含事件状态宏

#define COVEROPEN_DELAY_PARA 2 //开盖延时sec-1

// 上电运行期间 每秒调用 
// 开盖事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int CoverOpen_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    //uint8_t payload[100];
    int result = 0;

    uint8_t duration_para       =  COVEROPEN_DELAY_PARA;//ctrl_block->CoverOpenS_duration_para;
    uint8_t *duration           = &ctrl_block->CoverOpenS_duration;
    uint32_t* accumulate_time   = &ctrl_block->CoverOpenS_acc_time;
    uint8_t state               /*=  ctrl_block->CoverOpenS_status*/;

    uint16_t RecordId           = CoverOpenS;

    if (ctrl_block->CoverOpenS_status == EVENT_SET)
    {
        state = 1;
    }
    else
    {
        state = 0;
    }
    
    result = event_duration_pro(duration_para, duration, state, instance, RecordId);

    if (result != 0)
    {
        send_event_mesg_pro(instance, task, RecordId, NULL, result);
    }

    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;/* 累计时间累加 */
    }

    return result;
}





// 下电运行期间 每秒调用 
// 开盖事件
int CoverOpen_event_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    //uint8_t payload[100];
    int result = 0;

    uint8_t duration_para       =  COVEROPEN_DELAY_PARA;//ctrl_block->CoverOpenS_duration_para;
    uint8_t *duration           = &ctrl_block->CoverOpenS_duration;
    uint32_t* accumulate_time   = &ctrl_block->CoverOpenS_time_low_power_cnt;//&ctrl_block->CoverOpenS_acc_time;
    uint8_t state               /*=  ctrl_block->CoverOpenS_status*/;

    uint16_t RecordId           = CoverOpenS;
    struct message_t mesg_tmp;

    if (ctrl_block->pd_cover_EndRecord_flag != 0x5A) //检测周期未完成
    {
        if (ctrl_block->CoverOpenS_status == EVENT_SET)
        {
            state = 1;
        }
        else
        {
            state = 0;
        }
        
        result = event_duration_pro(duration_para, duration, state, instance, RecordId);

        if (result != 0)
        {
            //send_event_mesg_pro(instance, task, RecordId, NULL, result);
            if (result == 1) //start
            {
                mesg_tmp.id = RecordId;
            }
            else //end
            {
                mesg_tmp.id = RecordId + 1;
                ctrl_block->pd_cover_EndRecord_flag = 0x5A;
            }

            ctrl_block->pd_active_flag_update = 0x5A;   //停电下 保存标志
            
            //s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + 
            //            (offsetof(struct mbase_control_block_t, active_flag[0]) -  offsetof(struct mbase_control_block_t, persistent_section_start)) );
            //nd_set_offset(instance, s_index, (uint8_t*)&ctrl_block->active_flag[0], sizeof(ctrl_block->active_flag));               
            recv_record_process(instance, task, &mesg_tmp, NULL);
        }

    }



    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;/* 累计时间累加 */
    }

    return result;
}



