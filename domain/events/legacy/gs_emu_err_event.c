#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "domain/events/generic.h"

// 上电运行期间 每秒调用
// 事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int EmuErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t RecordId = EmuErrS;

    nd_generic_event_state_t state = {
        .debounce = {
            .duration = ctrl_block->EmuErrS_duration,
            .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
        },
        .accumulated_seconds = ctrl_block->EmuErrS_acc_time,
    };
    uint8_t condition = (ctrl_block->EmuErrS_status == EVENT_SET);

    nd_event_transition_t result =
        nd_generic_event_step(&state, ctrl_block->EmuErrS_duration_para, condition);

    ctrl_block->EmuErrS_duration = state.debounce.duration;
    ctrl_block->EmuErrS_acc_time = state.accumulated_seconds;
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
