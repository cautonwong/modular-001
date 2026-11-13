#include "nidavellir.h"
#include "domain/common/status.h"
#include "gs_record.h"
#include "domain/events/generic.h"
#include "domain/events/legacy/events_applications.h"
#include "kernel/legacy_control_block.h"



// 上电运行期间 每秒调用 
// 电源异常事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int PowerAbnormal_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t RecordId = PowerAbnormalS;

    nd_generic_event_state_t state = {
        .debounce = {
            .duration = ctrl_block->PowerAbnormalS_duration,
            .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
        },
        .accumulated_seconds = ctrl_block->PowerAbnormalS_acc_time,
    };

    //上电运行只结束
    nd_event_transition_t result = nd_generic_event_step(&state, 1, 0);

    ctrl_block->PowerAbnormalS_duration = state.debounce.duration;
    ctrl_block->PowerAbnormalS_acc_time = state.accumulated_seconds;
    if (result == ND_EVENT_STARTED) {
        set_activeflag(instance, RecordId);
    } else if (result == ND_EVENT_ENDED) {
        clr_activeflag(instance, RecordId);
    }

    if (result != ND_EVENT_NONE) {
        send_event_mesg_pro(instance, task, RecordId, NULL, (int)result);
    }
    return (int)result;
}


//  下电状态 累计时间累加 
//  下电状态 每秒调用 
void PowerAbnormal_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t RecordId = PowerAbnormalS;
    uint32_t* accumulate_time   = &ctrl_block->PowerAbnormalS_time_low_power_cnt;//&ctrl_block->PowerAbnormalS_acc_time;
    struct message_t mesg_tmp;



    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;//  累计时间累加
    }
    else
    {
        if(( ctrl_block->power_abnormal_tick == 5 ))
        {
            if (ctrl_block->PowerAbnormalS_status == ND_EVENT_SET)
            {
                nd_event_duration_t debounce = { 0 };

                if (nd_event_duration_start(&debounce) == ND_EVENT_STARTED)
                {
                    set_activeflag(instance, RecordId); //停电下同时刷新e2p

                    ctrl_block->pd_active_flag_update = 0x5A;   //停电下 保存标志

                    mesg_tmp.id = RecordId;
                    recv_record_process(instance, task, &mesg_tmp, NULL);
                    (*accumulate_time)++;//  累计时间累加
                }
            }
        }
    }


}

