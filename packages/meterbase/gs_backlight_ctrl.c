#include "nidavellir.h"
#include "pipeline.h"
#include "applications.h"
#include "port.h"
#include "pin_function_def.h"

struct alarm_t
{
    uint32_t control_bit; /**< 设备报警控制字对应位 */
    uint8_t  activeflag;    /**< 事件发生标志位存储 ID */
};

/** 事件报警判断---背光*/
int event_alarm_control(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint32_t control_word = ctrl_block->backlight_control_word;
    uint8_t i;//, status;
#if 0
    for (i = 0; i < sizeof(alarm_control) / sizeof(struct alarm_t); i++)
    {
        if ((control_word & alarm_control[i].control_bit) && (event_flags_get(instance,alarm_control[i].activeflag) == 1))
        {
            return 1;
        }
    }
#endif
    return 0;
}

// 每秒调用 结合外部消息 触发背光动作            返回1:点亮                  0:关闭
int backlight_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    
    if(ctrl_block->sta & F_RemoteAlarm)   //远程告警
    {
        LCD_BACK_LIGHT_OPEN(); // 打开背光
        return 1;
    }
    
    if((mbase_cb->blt_alarm & 0x00400000) == 0x00400000)    //电池欠压，开启背光
    {
        if(mbase_cb->voltage_battery < Bat_Low_Level)
        {
            ctrl_block->BackLightTmr = 3;
        }
    }
    
    if(ctrl_block->BackLightTmr > 0)    ctrl_block->BackLightTmr--;

    if(ctrl_block->BackLightTmr == 0)
    {
        LCD_BACK_LIGHT_CLOSE();
    }
    else
    {
        LCD_BACK_LIGHT_OPEN(); // 打开背光
    }
    return 1;
}

