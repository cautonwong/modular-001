#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"

#define _value(a) a = ((int32_t)a < 0)? (int32_t)(-a) : (int32_t)a



// 上电运行期间 每秒的调用 
// 掉电事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int ZeroLineErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t payload[100];
    uint8_t state = false;
    int result = 0;
    
    int32_t Ia_abs = (ctrl_block->current_l);
    int32_t In_abs = (ctrl_block->current_n);    

    uint8_t duration_para = ctrl_block->ZeroLineErrS_duration_para;
    uint32_t threshold1 = ctrl_block->ZeroLineErrS_Current_threshold / 10; // 电能表零线电流异常事件-电流触发下限
    uint32_t threshold2 = ctrl_block->ZeroLineErrS_rate_threshold;    // 电能表零线电流异常事件 限值
    uint8_t *duration = &ctrl_block->ZeroLineErrS_duration;
    uint32_t *accumulate_time = &ctrl_block->ZeroLineErrS_acc_time;
    uint16_t rate = 0;

    uint16_t RecordId = ZeroLineErrS;

    _value(Ia_abs);
    _value(In_abs);  
    
#if 0
    if (ctrl_block->Imax == 100) /*最大电流小于10A 互感式表不启用*/
    {
        return 0;
    }
#endif 
    
    if (threshold2 == 0) // 限值rate为0
    {
        return 0;
    }

    if((Ia_abs < threshold1) && (In_abs < threshold1))
    {
        return 0;
    }

    if(Ia_abs >= In_abs)
    {
        rate = (Ia_abs - In_abs) * 10000 /Ia_abs;
    }
    else
    {
        rate = (In_abs - Ia_abs) * 10000 /In_abs;
    }
      

    if(rate > threshold2 )
    {
        state = true;
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



