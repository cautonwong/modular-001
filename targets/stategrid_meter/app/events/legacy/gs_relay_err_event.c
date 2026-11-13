#include "nidavellir.h"
#include "gs_record.h"
#include "domain/events/switch_err.h"
#include "domain/events/legacy/events_applications.h"
#include "kernel/legacy_control_block.h"

// 上电运行期间 每秒调用
// 负荷开关误动作 事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int SwitchErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t RecordId = SwitchErrS;
    uint8_t err_flag = ctrl_block->SwitchErrS_status;
    nd_event_transition_t result = ND_EVENT_NONE;

    nd_switch_err_state_t state = {
        .event = {
            .debounce = {
                .duration = ctrl_block->SwitchErrS_duration,
                .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
            },
            .accumulated_seconds = ctrl_block->SwitchErrS_acc_time,
        },
    };

    result = nd_switch_err_step(&state, 3, err_flag,
                                ctrl_block->relayerr_record != 0x5A);

    ctrl_block->SwitchErrS_duration = state.event.debounce.duration;
    ctrl_block->SwitchErrS_acc_time = state.event.accumulated_seconds;
    if (result == ND_EVENT_STARTED) {
        set_activeflag(instance, RecordId);
    } else if (result == ND_EVENT_ENDED) {
        clr_activeflag(instance, RecordId);
    }

    if (result != ND_EVENT_NONE) {
        send_event_mesg_pro(instance, task, RecordId, NULL, (int)result);
        ctrl_block->relayerr_record = 0;
    }

    return (int)result;
}
