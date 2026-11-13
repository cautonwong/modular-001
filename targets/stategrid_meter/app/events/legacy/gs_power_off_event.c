#include "nidavellir.h"
#include "gs_record.h"
#include "domain/events/power_off.h"
#include "domain/events/legacy/events_applications.h"
#include "kernel/legacy_control_block.h"

#define PHASE_NUM 1   //单相或三相 支持1或3

// 上电运行期间 每秒调用
// 掉电事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int PowerOff_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t RecordId = PowerOffS;

    uint32_t voltages[PHASE_NUM] = {ctrl_block->voltage_ac};
    nd_power_off_rule_t rule = {.debounce_seconds = 4};
    nd_power_off_state_t state = {
        .debounce = {
            .duration = ctrl_block->PowerOffS_duration,
            .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
        },
        .accumulated_seconds = ctrl_block->PowerOffS_acc_time,
        .accumulated_low_power_seconds = 0,
    };

    nd_event_transition_t result = nd_power_off_step(&state, &rule, voltages, PHASE_NUM,
                                                     ctrl_block->volt_60, 0);

    ctrl_block->PowerOffS_duration = state.debounce.duration;
    ctrl_block->PowerOffS_acc_time = state.accumulated_seconds;
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
void PowerOff_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t* accumulate_time = &ctrl_block->PowerOffS_time_low_power_cnt;
    uint16_t RecordId = PowerOffS;
    struct message_t mesg_tmp;

    if (instance->ac_power_off_sec_cnt == 1) {
        nd_event_duration_t debounce = {
            .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
        };

        if (nd_event_duration_start(&debounce) == ND_EVENT_STARTED) {
            set_activeflag(instance, RecordId);
            ctrl_block->pd_active_flag_update = 0x5A;

            mesg_tmp.id = RecordId;
            recv_record_process(instance, task, &mesg_tmp, NULL);
        }
    }

    if (true == get_activeflag(instance, RecordId)) {
        (*accumulate_time)++;
    }
}
