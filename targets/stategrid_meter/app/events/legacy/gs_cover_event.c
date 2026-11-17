#include "targets/stategrid_meter/app/stategrid_context.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "application/adapters/dl698/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "port.h"
#include "domain/common/status.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "domain/events/cover_open.h"
#include "targets/stategrid_meter/app/events/events_applications.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"

#define COVEROPEN_DELAY_PARA 2 //开盖延时sec-1

// 上电运行期间 每秒调用
// 开盖事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int CoverOpen_event_judge(void *instance, void *task, void *mesg)
{
    struct mbase_control_block_t* ctrl_block = ((stategrid_context_t *)instance)->mbase;
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
int CoverOpen_event_LowPower_pro(void *instance, void *task, void *mesg)
{
    struct mbase_control_block_t* ctrl_block = ((stategrid_context_t *)instance)->mbase;
    uint16_t RecordId = CoverOpenS;
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
                ctrl_block->pd_active_flag_update = 0x5A;
                recv_record_process(instance, RecordId, NULL);
            } else {
                ctrl_block->pd_cover_EndRecord_flag = 0x5A;
                ctrl_block->pd_active_flag_update = 0x5A;
                recv_record_process(instance, RecordId + 1, NULL);
            }
        }
    }

    return result;
}
