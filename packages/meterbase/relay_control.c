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

#define     KeyPortIn           {GPIOA->OEN |=BIT2; GPIOA->IE |=BIT2;}
#define     KeyPortCheck        (GPIOA->STS & BIT2)

extern uint8_t Magnetic_hold_relay_connect(void);
extern uint8_t Magnetic_hold_relay_disconnect();


//每秒调用，告警延时和限电时间递减
void Relay_Sec_Task(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    if(mbase_control_block->alarmmin > 0)  mbase_control_block->alarmmin--;
    if(mbase_control_block->limitmin > 0)  mbase_control_block->limitmin--;
    if(mbase_control_block->imaxmin > 0)   mbase_control_block->imaxmin--;
}

static unsigned char switchon = 1;
static unsigned char lastswitchsta = 0xaa;

//每秒调用
void do_relay(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    Relay_Sec_Task(instance, pipeline);
    
//    if(mbase_control_block->voltage_ac > )    return; //电压过高过低不执行
    
    if((mbase_control_block->sta & F_RemoteRelayOn) || (mbase_control_block->sta == F_RemoteNull))    //保电或正常用电
    {
        switchon = 1;
    }
    else if(mbase_control_block->sta & F_RemoteRelayOnAllow)   //合闸允许
    {
        if(KeyPortCheck == 1)    //按键3秒
        {
            switchon = 1;
            mbase_control_block->sta &= ~F_RemoteRelayOnAllow;
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
    else
    {
        switchon = 1;
    }

    if(lastswitchsta != switchon)   //操作继电器
    {
        if(switchon == 1)
        {
            Magnetic_hold_relay_connect();//合闸
        }
        else 
        {
            Magnetic_hold_relay_disconnect();//拉闸
        }
        lastswitchsta = switchon;
    }
}

