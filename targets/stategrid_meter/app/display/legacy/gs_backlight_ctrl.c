#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "pipeline.h"
#include "port.h"
#include "domain/display/backlight.h"
#include "application/apps/app_config.h"
#include "application/apps/display_applications.h"
#include "kernel/legacy_control_block.h"

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

//时钟电池欠压判断(秒调用)    -状态延时3秒
void check_RtcBat_volt(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t value = 0xFF;
    static uint8_t cnt = 0;


    if(nd_mbase_voltage_battery(mbase_cb) <= RtcBat_Low_Level_Dwn)
    {
        value = 1;
    }
    else  if(nd_mbase_voltage_battery(mbase_cb) >= RtcBat_Low_Level_Up)
    {
        value = 0;
    }
    else
    {
        value = 0xFF;
    }

    if((value != 0xFF) && (nd_mbase_RtcBat_UnderV_state(mbase_cb) != value))
    {
        cnt++;
    }
    else
    {
        cnt = 0;
    }

    if(cnt >= 3)
    {
        nd_mbase_RtcBat_UnderV_state(mbase_cb) = value;
        cnt = 0;
    }

}


// 每秒调用 结合外部消息 触发背光动作            返回1:点亮                  0:关闭
int backlight_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);


    if(nd_mbase_BackLightTmr(mbase_cb) > 0)    nd_mbase_BackLightTmr(mbase_cb)--;
    
    if(nd_mbase_Page_ID(mbase_cb) == 0xFFFF)    //全屏
    {
        nd_backlight_set(1); // 打开背光
        return 1;
    }
    
    if (nd_mbase_sta(mbase_cb) & F_RemoteAlarm)   //远程告警
    {
        nd_backlight_set(1); // 打开背光
        return 1;
    }

    if ((nd_mbase_alarmmin(mbase_cb) > 0) || (nd_mbase_imaxmin(mbase_cb) > 0))   // 跳闸延时
    {
        nd_backlight_set(1); // 打开背光
        return 1;
    }    

    if((nd_mbase_blt_alarm(mbase_cb) & 0x00400000) == 0x00400000)    //电池欠压，开启背光
    {
        if(nd_mbase_RtcBat_UnderV_state(mbase_cb) == true) 
        {
            nd_mbase_BackLightTmr(mbase_cb) = 3;
        }
    }
    
    

    if(nd_mbase_BackLightTmr(mbase_cb) == 0)
    {
        nd_backlight_set(0);
    }
    else
    {
        nd_backlight_set(1); // 打开背光
    }
    return 1;
}

