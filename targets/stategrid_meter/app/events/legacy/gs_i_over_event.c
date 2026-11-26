#include "targets/stategrid_meter/app/stategrid_context.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "targets/stategrid_meter/persistence/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "port.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "domain/events/over_current.h"
#include "targets/stategrid_meter/app/events/events_applications.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"

// 上电运行期间 每秒的调用
// 过流事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int ia_over_event_judge(void *instance, void *task, void *mesg)
{
    struct mbase_control_block_t* ctrl_block = ((stategrid_context_t *)instance)->mbase;
    uint16_t RecordId = OverCurrentAS;

    nd_over_current_rule_t rule = {
        .threshold = ctrl_block->OverCurrent_threshold / 10,
        .debounce_seconds = ctrl_block->OverCurrent_duration_para,
    };
    nd_over_current_state_t state = {
        .debounce = {
            .duration = ctrl_block->OverCurrentAS_duration,
            .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
        },
        .accumulated_seconds = ctrl_block->OverCurrentAS_acc_time,
    };

    nd_event_transition_t result = nd_over_current_step(&state, &rule, ctrl_block->current_l);

    ctrl_block->OverCurrentAS_duration = state.debounce.duration;
    ctrl_block->OverCurrentAS_acc_time = state.accumulated_seconds;
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
