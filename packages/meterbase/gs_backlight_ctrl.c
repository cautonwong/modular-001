#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"




// ctrl_block  以下内容放到port.h
#if 1
uint32_t backlight_switchOn_Hpriority_sec;/*上电或插卡等高优先级 背光持续打开时间,          赋值后每秒递减           */
uint32_t backlight_switchOn_Lpriority_sec;/*背光打开持续时间          赋值后每秒递减 */
uint32_t backlight_control_word;/*背光报警配置字 */


// para
uint32_t backlight_key_time; /*按键背光点亮时间 */

uint8_t lcd_poweron_all_dis_time;  /*上电全显时间 */
uint8_t lcd_autodis_list_num;    /*轮显屏数 */
uint8_t lcd_autodis_keep_sec;    /*轮显持续时间 */
uint16_t lcd_checkdis_keep_sec;/*显示查看显示时间 */
uint16_t lcd_checkdis_all_keep_sec;/*显示查看全屏显示时间 */



uint8_t under_un68; /*所有相电压均低于68% */
uint8_t auxiliary_power;/*辅助电源供电标志 */
uint8_t remote_alarm_flag; /*远程告警命令true-触发告警  */
uint8_t battery1_under2v8; /*时钟电池欠压 */
uint8_t battery2_under4v8; /*停抄电池欠压 */

#endif


struct alarm_t
{
    uint32_t control_bit; /**< 设备报警控制字对应位 */
    uint8_t  activeflag;    /**< 事件发生标志位存储 ID */
};



const struct alarm_t CODE alarm_control[] =  // bit位 待调整  事件名称 待调整
{
    { 0x00000001, ctrl_block->PowerReverseLa },          /**< 有功功率反向 */
    { 0x00000001, ctrl_block->PowerReverseLb },          /**< 有功功率反向 */
    { 0x00000001, ctrl_block->PowerReverseLc },          /**< 有功功率反向 */
    { 0x00000001, ctrl_block->PowerReverseLt },          /**< 有功功率反向 */
                                             
    { 0x00000002, ctrl_block->TerminalCoverOpened },     /**< 开端钮盖 */
    { 0x00000004, 0x00 },                  /**< 预付费报警 */
    { 0x00000008, ctrl_block->battery2_under4v8 }, /**< 抄表电池欠压 */
    { 0x00000010, ctrl_block->LoadUnbalance },           /**< 电流不平衡 */
    { 0x00000020, ctrl_block->remote_alarm_flag }, /**< 远程告警 */
    { 0x00000040, ctrl_block->VoltageUnbalance },        /**< 电压不平衡 */
    { 0x00000080, ctrl_block->BadPowerFactor },          /**< 功率因数超限 */
    { 0x00000100, ctrl_block->ReplayError },             /**< 负荷开关误动作 */
                                             
    { 0x00000200, ctrl_block->UnderVoltageL1 },          /**< 欠压 */
    { 0x00000200, ctrl_block->UnderVoltageL2 },          /**< 欠压 */
    { 0x00000200, ctrl_block->UnderVoltageL3 },          /**< 欠压 */
                                             
    { 0x00000400, ctrl_block->MeterCoverOpened },        /**< 开表盖 */
                                             
    { 0x00000800, ctrl_block->OverLoadL1 },              /**< 过载 */
    { 0x00000800, ctrl_block->OverLoadL2 },              /**< 过载 */
    { 0x00000800, ctrl_block->OverLoadL3 },              /**< 过载 */
    
    { 0x00001000, ctrl_block->Voltagesequencereversal }, /**< 电压逆相序 */
    
    { 0x00002000, ctrl_block->LostCurrentL1 },           /**< 失流 */
    { 0x00002000, ctrl_block->LostCurrentL2 },           /**< 失流 */
    { 0x00002000, ctrl_block->LostCurrentL3 },           /**< 失流 */
                                             
    { 0x00004000, ctrl_block->LostVoltageL1 },           /**< 失压 */
    { 0x00004000, ctrl_block->LostVoltageL2 },           /**< 失压 */
    { 0x00004000, ctrl_block->LostVoltageL3 },           /**< 失压 */
                                             
    { 0x00008000, ctrl_block->MissVoltageL1 },           /**< 断相 */
    { 0x00008000, ctrl_block->MissVoltageL2 },           /**< 断相 */
    { 0x00008000, ctrl_block->MissVoltageL3 },           /**< 断相 */
                                             
    { 0x00010000, ctrl_block->PositiveDemandOver },      /**< 有功需量超限 */
                                             
    { 0x00020000, ctrl_block->OverVoltageL1 },           /**< 过压 */
    { 0x00020000, ctrl_block->OverVoltageL2 },           /**< 过压 */
    { 0x00020000, ctrl_block->OverVoltageL3 },           /**< 过压 */
                                             
    { 0x00040000, ctrl_block->OverCurrentL1 },           /**< 过流 */
    { 0x00040000, ctrl_block->OverCurrentL2 },           /**< 过流 */
    { 0x00040000, ctrl_block->OverCurrentL3 },           /**< 过流 */
                                             
    { 0x00080000, ctrl_block->CurrentReversal },         /**< 电流逆相序 */
    { 0x00100000, ctrl_block->LoadUnbalanceSeverit },    /**< 电流严重不平衡 */
    { 0x00200000, ctrl_block->Powerdirectionchanged },   /**< 总有功功率反向 */
    { 0x00400000, ctrl_block->battery1_under2v8 },/**< 时钟电池欠压 */
    { 0x00800000, ctrl_block->VoltageLostAll }           /**< 全失压事件 */
};


/** 事件报警判断---背光*/
uint8 event_alarm_control(struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;
    uint32_t control_word = ctrl_block->backlight_control_word;
    uint8 i, status;

    for (i = 0; i < sizeof(event_alarm_control) / sizeof(struct event_alarm_t); i++)
    {
        if ((control_word & event_alarm_control[i].control_bit) && (true == event_alarm_control[i].activeflag))
        {
            return 1;
        }
    }

    return 0;
}




// 每秒调用 结合外部消息 触发背光动作            返回1:点亮                  0:关闭
int backlight_pro(struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->control_block;
    uint32_t *switchOn_H_sec = &ctrl_block->backlight_switchOn_Hpriority_sec;
    uint32_t *switchOn_L_sec = &ctrl_block->backlight_switchOn_Lpriority_sec;
    uint8_t *check_enabled = &ctrl_block->backlight_enabled;
    
    switch (mesg->id)
    {
	    case 0xAAAA://上电 待调整 
	        //上电立即打开背光 待调整 
            *switchOn_H_sec = ctrl_block->lcd_poweron_all_dis_time; //上电全显期间不受其他控制
	        break;

        case 0xAAAA://插卡信号 待调整 
            if((ctrl_block->auxiliary_power == true) || (ctrl_block->under_un68 == false))
            {
                *switchOn_H_sec = 60; 
            }
            break;

        case 0xAAAA://收到红外信号       两个轮显周期 待调整 
            *switchOn_L_sec = 60;
            *switchOn_L_sec = 2* (ctrl_block->lcd_autodis_list_num) * (ctrl_block->lcd_autodis_keep_sec);
            break;

        case 0xAAAA://收到按键信号        待调整 
            *switchOn_L_sec = ctrl_block->backlight_key_time;
            break;   

        case 0xAAAA://显示查看唤醒背光         待调整 
            *switchOn_L_sec = 10; // 参数无效时？？
            *switchOn_L_sec = ctrl_block->lcd_checkdis_keep_sec;
            break;
            
        case 0xAAAA://显示查看全屏命令唤醒背光 待调整 
            *switchOn_L_sec = 5;
            *switchOn_L_sec = ctrl_block->lcd_checkdis_all_keep_sec;
            break;

        case 0xAAAA://收到实时背光点亮信号(跳闸延时期间/余额不足报警等)   待调整 
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

