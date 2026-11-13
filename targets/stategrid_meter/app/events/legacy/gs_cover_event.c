#include "nidavellir.h"
#include "domain/common/status.h"
#include "gs_record.h"
#include "domain/events/cover_open.h"
#include "domain/events/legacy/events_applications.h"
#include "kernel/legacy_control_block.h"

#define COVEROPEN_DELAY_PARA 2 //开盖延时sec-1

// 上电运行期间 每秒调用
// 开盖事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int CoverOpen_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t RecordId = CoverOpenS;

    nd_cover_open_rule_t rule = {.debounce_seconds = COVEROPEN_DELAY_PARA};
    nd_cover_open_state_t state = {
        .debounce = {
            .duration = ctrl_block->CoverOpenS_duration,
            .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
        },
        .accumulated_seconds = ctrl_block->CoverOpenS_acc_time,
        .accumulated_low_power_seconds = 0,
    };

    uint8_t cover_open = (ctrl_block->CoverOpenS_status == ND_EVENT_SET);
    nd_event_transition_t result = nd_cover_open_step(&state, &rule, cover_open, 0);

    ctrl_block->CoverOpenS_duration = state.debounce.duration;
    ctrl_block->CoverOpenS_acc_time = state.accumulated_seconds;
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

// 下电运行期间 每秒调用
// 开盖事件
int CoverOpen_event_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t RecordId = CoverOpenS;
    struct message_t mesg_tmp;
    int result = 0;

    if (ctrl_block->pd_cover_EndRecord_flag != 0x5A) //检测周期未完成
    {
        nd_cover_open_rule_t rule = {.debounce_seconds = COVEROPEN_DELAY_PARA};
        nd_cover_open_state_t state = {
            .debounce = {
                .duration = ctrl_block->CoverOpenS_duration,
                .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
            },
            .accumulated_seconds = 0,
            .accumulated_low_power_seconds = ctrl_block->CoverOpenS_time_low_power_cnt,
        };

        uint8_t cover_open = (ctrl_block->CoverOpenS_status == ND_EVENT_SET);
        nd_event_transition_t transition = nd_cover_open_step(&state, &rule, cover_open, 1);

        ctrl_block->CoverOpenS_duration = state.debounce.duration;
        ctrl_block->CoverOpenS_time_low_power_cnt = state.accumulated_low_power_seconds;
        if (transition == ND_EVENT_STARTED) {
            set_activeflag(instance, RecordId);
        } else if (transition == ND_EVENT_ENDED) {
            clr_activeflag(instance, RecordId);
        }

        result = (int)transition;
        if (transition != ND_EVENT_NONE) {
            if (transition == ND_EVENT_STARTED) {
                mesg_tmp.id = RecordId;
            } else {
                mesg_tmp.id = RecordId + 1;
                ctrl_block->pd_cover_EndRecord_flag = 0x5A;
            }
            ctrl_block->pd_active_flag_update = 0x5A;
            recv_record_process(instance, task, &mesg_tmp, NULL);
        }
    }

    return result;
}
