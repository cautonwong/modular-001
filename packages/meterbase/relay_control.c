/*
#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"
#include "pin_function.h"
*/
#include "target.h"
#include "string.h"
#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "pin_function_def.h"

#define     KeyPortIn           {GPIOA->OEN |=BIT2; GPIOA->IE |=BIT2;}
#define     KeyPortCheck        (GPIOA->STS & BIT2)

uint16_t relay_off_chk,relay_on_chk; //继电器状态监测
extern uint8_t Magnetic_hold_relay_connect(void);
extern uint8_t Magnetic_hold_relay_disconnect();


//每秒调用，告警延时和限电时间递减
void Relay_Sec_Task(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    if(mbase_control_block->imaxmin > 0)   
    {
        mbase_control_block->imaxmin--;
        if(abs(mbase_control_block->current_l) < (mbase_control_block->I_Prot/10))
        {
            mbase_control_block->imaxmin = 0;
        }
    }
    else    //大电流拉闸延时和拉闸延时时间需要依次递减
    {
        if(mbase_control_block->alarmmin > 0)  mbase_control_block->alarmmin--;
        if(mbase_control_block->limitmin > 0)  mbase_control_block->limitmin--;
    }
}

static unsigned char switchon = 1;
static unsigned char lastswitchsta = 0xaa;
static unsigned char key_relay_on_tick = 0;

//每秒调用
void do_relay(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    Relay_Sec_Task(instance, pipeline);
    
    if((mbase_control_block->voltage_ac < mbase_control_block->volt_78) 
      || (mbase_control_block->voltage_ac > mbase_control_block->volt_120)) return; //电压过高过低不执行
    
    if((mbase_control_block->sta & F_RemoteRelayOn) || (mbase_control_block->sta == F_RemoteNull))    //保电或正常用电
    {
        if((mbase_control_block->sta & F_RemoteRelayOn) && (mbase_control_block->sta & F_RemoteRelayOff))   //拉闸后保电
        {
            if(KeyPortCheck == 0)    //按键3秒
            {
                if(key_relay_on_tick < 3)   key_relay_on_tick++;
                if(key_relay_on_tick >= 3)
                {
                    switchon = 1;
                    mbase_control_block->sta &= ~F_RemoteRelayOff;
                    key_relay_on_tick = 0;
                }
            }
            else
            {
                key_relay_on_tick = 0;
            }
        }
        else
        {
            switchon = 1;
        }
    }
    else if(mbase_control_block->sta & F_RemoteRelayOff)   //拉闸
    {
        if((mbase_control_block->alarmmin == 0) && (mbase_control_block->imaxmin == 0))   switchon = 0;
    }
    else if((mbase_control_block->sta & F_RemoteGaoJing0) || (mbase_control_block->sta & F_RemoteGaoJing1))  //预跳闸
    {
        if(mbase_control_block->imaxmin == 0)  //大电流不拉闸
        {
            switchon = 0;
            if(mbase_control_block->limitmin == 0) //限电时间
            {
                if(mbase_control_block->sta & F_RemoteGaoJing0)    //直接合闸
                {
                    mbase_control_block->sta &= ~F_RemoteGaoJing0;
                    switchon = 1;
                }
                else if(mbase_control_block->sta & F_RemoteGaoJing1)   //合闸允许
                {
                    mbase_control_block->sta &= ~F_RemoteGaoJing1;
                    mbase_control_block->sta |= F_RemoteRelayOnAllow;
                }
            }
        }
    }
    else if(mbase_control_block->sta & F_RemoteRelayOnAllow)   //合闸允许
    {
        if(KeyPortCheck == 0)    //按键3秒
        {
            if(key_relay_on_tick < 3)   key_relay_on_tick++;
            if(key_relay_on_tick >= 3)
            {
                switchon = 1;
                mbase_control_block->sta &= ~F_RemoteRelayOnAllow;
                key_relay_on_tick = 0;
            }
        }
        else
        {
            key_relay_on_tick = 0;
        }
        
        if(mbase_control_block->relay_true_sta == 0)    //检测到合闸
        {
            mbase_control_block->sta &= ~F_RemoteRelayOnAllow;
            key_relay_on_tick = 0;
        }
    }
    else
    {
        switchon = 1;
    }

    if(lastswitchsta != switchon)   //操作继电器
    {
        if(switchon == 1)
        {
            Magnetic_hold_relay_connect();//合闸
            send_event_mesg_pro(instance, NULL, SwitchOnS, NULL, 1);
            mbase_control_block->switch_on_succees = 0x5A;  //要挪到成功 待调整
            relay_off_chk = 0;
            relay_on_chk = 0;
        }
        else 
        {
            Magnetic_hold_relay_disconnect();//拉闸
            send_event_mesg_pro(instance, NULL, SwitchOffS, NULL, 1);
            mbase_control_block->switch_off_succees = 0x5A;  //要挪到成功 待调整
            relay_off_chk = 0;
            relay_on_chk = 0;
        }
        lastswitchsta = switchon;
    }
}


void relay_led_ctl(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
//    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    if(mbase_control_block->Page_ID == 0xFFFF)    //全屏
    {
        RELAY_LED_OPEN();
        return;
    }
    
    if((mbase_control_block->active_power == 0) && (relay_off_chk >= Relay_Ck_Time))//功率为0且继电器检测为断开，拉闸灯亮
    {
        mbase_control_block->relay_true_sta = 1;
    }
    
    if((mbase_control_block->active_power > 0) || (relay_on_chk >= Relay_Ck_Time)) //有电流或检测到继电器为合，灯灭
    {
        mbase_control_block->relay_true_sta = 0;
    }

    if(((mbase_control_block->sta & F_RemoteRelayOnAllow) && (mbase_control_block->relay_true_sta == 1))
      || ((mbase_control_block->sta & F_RemoteRelayOn) && (mbase_control_block->sta & F_RemoteRelayOff)))//合闸允许状态，闪烁
    {
        if(instance->time.tm_sec & 0x01)
        {
            RELAY_LED_OPEN();
        }
        else
        {
            RELAY_LED_CLOSE();
        }
        return;
    }
    
    if(mbase_control_block->relay_true_sta)
    {
        RELAY_LED_OPEN();
    }
    else
    {
        RELAY_LED_CLOSE();
    }
    
    /******************** 误动作检测 ***************/
    if(switchon == 0) //拉闸状态下，检测到继电器合闸或有电流，则误动作
    {
        if((mbase_control_block->relay_true_sta == 0) || (mbase_control_block->current_l > MIN_CURRENT))   //继电器状态合或电流大于启动电流
        {
            mbase_control_block->SwitchErrS_status = 1; //误动作
        }
        else
        {
            mbase_control_block->SwitchErrS_status = 0; 
        }
    }
    else    //合闸状态下，检测到继电器断开
    {
        if(mbase_control_block->relay_true_sta == 1)
        {
            mbase_control_block->SwitchErrS_status = 1; //误动作
        }
        else
        {
            mbase_control_block->SwitchErrS_status = 0; 
        }
    }
    
}