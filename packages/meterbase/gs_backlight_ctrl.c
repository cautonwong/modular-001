#include "nidavellir.h"
#include "pipeline.h"
#include "applications.h"
#include "port.h"

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
    uint32_t *switchOn_H_sec = &ctrl_block->backlight_switchOn_Hpriority_sec;
    uint32_t *switchOn_L_sec = &ctrl_block->backlight_switchOn_Lpriority_sec;
    //uint8_t *check_enabled = &ctrl_block->backlight_enabled;
    
    switch (mesg->id)
    {
	    case 0://上电 待调整 
	        //上电立即打开背光 待调整 
            *switchOn_H_sec = ctrl_block->lcd_poweron_all_dis_time; //上电全显期间不受其他控制
	        break;

        case 1://插卡信号 待调整 
            if((ctrl_block->auxiliary_power == true) || (ctrl_block->under_un68 == false))
            {
                *switchOn_H_sec = 60; 
            }
            break;

        case 2://收到红外信号       两个轮显周期 待调整 
            *switchOn_L_sec = 60;
            *switchOn_L_sec = 2* (ctrl_block->lcd_autodis_list_num) * (ctrl_block->lcd_autodis_keep_sec);
            break;

        case 3://收到按键信号        待调整 
            *switchOn_L_sec = ctrl_block->backlight_key_time;
            break;   

        case 4://显示查看唤醒背光         待调整 
            *switchOn_L_sec = 10; // 参数无效时？？
            *switchOn_L_sec = ctrl_block->lcd_checkdis_keep_sec;
            break;
            
        case 5://显示查看全屏命令唤醒背光 待调整 
            *switchOn_L_sec = 5;
            *switchOn_L_sec = ctrl_block->lcd_checkdis_all_keep_sec;
            break;

        case 6://收到实时背光点亮信号(跳闸延时期间/余额不足报警等)   待调整 
            *switchOn_L_sec = 5;
            break;
            
        default:
            break;

    }

    if (*switchOn_H_sec > 0)
    {
        *switchOn_H_sec--;
    }

    if (*switchOn_L_sec > 0)
    {
        *switchOn_L_sec--;
    }
    // 
    if((ctrl_block->auxiliary_power == false) && (ctrl_block->under_un68 == true))
    {
        *switchOn_L_sec = 0;
        if(*switchOn_H_sec == 0) 
        {
            return 0;
        }
    }

    if ((*switchOn_L_sec > 0) || (*switchOn_H_sec > 0))
    {
        return 1; // 打开背光
    }
    else
    {
        return event_alarm_control(instance, task, mesg);
    }

}

