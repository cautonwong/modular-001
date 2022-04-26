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

const struct alarm_t alarm_control[] =  
{
    { 0x00000001, PowerReverseL1 },          /**< 有功功率反向 */
    { 0x00000001, PowerReverseL2 },          /**< 有功功率反向 */
    { 0x00000001, PowerReverseL3 },          /**< 有功功率反向 */
    { 0x00000001, PowerReverseL4 },          /**< 有功功率反向 */
                                             
    { 0x00000002, TerminalCoverOpened },     /**< 开端钮盖 */
    { 0x00000004, 0x00 },                  /**< 预付费报警 */
    { 0x00000008, battery2_under4v8 },      /**< 抄表电池欠压 */
    { 0x00000010, LoadUnbalance },           /**< 电流不平衡 */
    { 0x00000020, remote_alarm_flag }, /**< 远程告警 */
    { 0x00000040, VoltageUnbalance },        /**< 电压不平衡 */
    { 0x00000080, BadPowerFactor },          /**< 功率因数超限 */
    { 0x00000100, ReplayError },             /**< 负荷开关误动作 */
                                             
    { 0x00000200, UnderVoltageL1 },          /**< 欠压 */
    { 0x00000200, UnderVoltageL2 },          /**< 欠压 */
    { 0x00000200, UnderVoltageL3 },          /**< 欠压 */
                                             
    { 0x00000400, MeterCoverOpened },        /**< 开表盖 */
                                             
    { 0x00000800, OverLoadL1 },              /**< 过载 */
    { 0x00000800, OverLoadL2 },              /**< 过载 */
    { 0x00000800, OverLoadL3 },             
    { 0x00001000, Voltagesequencereversal }, /**< 电压逆相序 */
    { 0x00002000, LostCurrentL1 },           /**< 失流 */
    { 0x00002000, LostCurrentL2 },           /**< 失流 */
    { 0x00002000, LostCurrentL3 },           /**< 失流 */
                                             
    { 0x00004000, LostVoltageL1 },           /**< 失压 */
    { 0x00004000, LostVoltageL2 },           /**< 失压 */
    { 0x00004000, LostVoltageL3 },           /**< 失压 */
                                             
    { 0x00008000, MissVoltageL1 },           /**< 断相 */
    { 0x00008000, MissVoltageL2 },           /**< 断相 */
    { 0x00008000, MissVoltageL3 },           /**< 断相 */
                                             
    { 0x00010000, PositiveDemandOver },      /**< 有功需量超限 */
                                             
    { 0x00020000, OverVoltageL1 },           /**< 过压 */
    { 0x00020000, OverVoltageL2 },           /**< 过压 */
    { 0x00020000, OverVoltageL3 },           /**< 过压 */
                                             
    { 0x00040000, OverCurrentAS },           /**< 过流 */
    { 0x00040000, OverCurrentL2 },           /**< 过流 */
    { 0x00040000, OverCurrentL3 },           /**< 过流 */
                                             
    { 0x00080000, CurrentReversal },         /**< 电流逆相序 */
    { 0x00100000, LoadUnbalanceSeverit },    /**< 电流严重不平衡 */
    { 0x00200000, Powerdirectionchanged },   /**< 总有功功率反向 */
    { 0x00400000, battery1_under2v8 },/**< 时钟电池欠压 */
    { 0x00800000, VoltageLostAll }           /**< 全失压事件 */
};

/** 事件报警判断---背光*/
int event_alarm_control(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint32_t control_word = ctrl_block->backlight_control_word;
    uint8_t i;//, status;

    for (i = 0; i < sizeof(alarm_control) / sizeof(struct alarm_t); i++)
    {
        if ((control_word & alarm_control[i].control_bit) && (event_flags_get(instance,alarm_control[i].activeflag) == 1))
        {
            return 1;
        }
    }

    return 0;
}

// 每秒调用 结合外部消息 触发背光动作            返回1:点亮                  0:关闭
int backlight_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    
    if( ctrl_block->sta & F_RemoteAlarm )//远程告警
    {
        ctrl_block->BackLightTmr = 0xFFFF;
    }
    
    if(ctrl_block->BackLightTmr == 0xFFFF)   //故障报警
    {
        LCD_BACK_LIGHT_OPEN(); // 打开背光
        return 1;
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

