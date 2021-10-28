#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"
#include "pin_function.h"

#define     F_RemoteNull            0x00    //合闸
#define		F_RemoteRelayOn  	    0x01	//保电
#define		F_RemoteRelayOff 	    0x02    //拉闸
#define		F_RemoteAlarm	  	    0x04    //告警
#define		F_RemoteGaoJing0	    0x08    //预跳闸
#define		F_RemoteGaoJing1	    0x10    //预跳闸
#define     F_RemoteRelayOnAllow    0x20    //合闸允许


//掉电均要保存，上电后读取
typedef struct 
{
	unsigned char cmd;	    //控制命令  0x1A , 0x1B....
    unsigned char mode;     //拉合闸方式（直接、允许）
	unsigned char sta;      //命令状态
    unsigned long alarmmin; //跳闸延时(min)
    unsigned long limitmin; //限电时间(min)
    unsigned long imaxmin;  //大电流延时跳闸（min）
}REMOTECTR;

extern REMOTECTR   RemoteCtr;

//每秒调用，告警延时和限电时间递减
void RemoteTick_Sec(void)
{
    if(RemoteCtr.alarmmin > 0)  RemoteCtr.alarmmin--;
    if(RemoteCtr.limitmin > 0)  RemoteCtr.limitmin--;
    if(RemoteCtr.imaxmin > 0)   RemoteCtr.imaxmin--;
}

//协议中调用，返回：0（成功），1（失败）
unsigned char do_remotecontrol(void)
{
    unsigned char result = 0;
    
    RemoteTick_Sec();
    
    switch(RemoteCtr.cmd)
    {
        case 0x1A:  //跳闸
        case 0x1D:	//代表预跳闸(延时到直接合闸允许)，
        case 0x1E:	//代表预跳闸(延时到合闸允许)，	
            if(RemoteCtr.sta & F_RemoteRelayOn) {result = 1; break;}//保电    
            if(RemoteCtr.cmd == 0x1A)
            {
                RemoteCtr.sta = F_RemoteRelayOff;
                RemoteCtr.limitmin = 0;
            }
            else
            {
                if(RemoteCtr.sta & F_RemoteRelayOff)    {result = 1; break;}    //跳闸后，下发预跳闸失败
                if(RemoteCtr.cmd == 0x1D)   RemoteCtr.sta |= F_RemoteGaoJing0;
                else if(RemoteCtr.cmd == 0x1E)   RemoteCtr.sta |= F_RemoteGaoJing1;
            }
            if()    RemoteCtr.imaxmin = ;   //大电流延时时间赋值
            break;
        case 0x1B:  //合闸允许
            RemoteCtr.sta |= F_RemoteRelayOnAllow;
            break;
        case 0x1C:	//直接合闸
            RemoteCtr.sta &= ~F_RemoteRelayOff;
            break;
        case 0x2A:	//报警
            RemoteCtr.sta |= F_RemoteAlarm;
            break;
        case 0x2B:	//报警解除
            RemoteCtr.sta &= ~F_RemoteAlarm;
            break;
        case 0x3A:	//保电
            RemoteCtr.sta |= F_RemoteRelayOn;
            break;
        case 0x3B:	//保电解除
            RemoteCtr.sta &= ~F_RemoteRelayOn;
            break;
        default:
            result = 1;
            break; 
    }
    return result;
}

static unsigned char switchon = 1;
static unsigned char lastswitchsta = 0xaa;

//每秒调用
void do_relay(void)
{
    if()    return; //电压过高过低不执行
    
    RemoteTick_Sec();
    
    if((RemoteCtr.sta & F_RemoteRelayOn) || (RemoteCtr.sta == F_RemoteNull))    //保电或正常用电
    {
        switchon = 1;
    }
    else if(RemoteCtr.sta & F_RemoteRelayOnAllow)   //合闸允许
    {
        if()    //按键3秒
        {
            switchon = 1;
            RemoteCtr.sta &= ~F_RemoteRelayOnAllow;
        }
    }
    else if(RemoteCtr.sta & F_RemoteRelayOff)   //拉闸
    {
        if((RemoteCtr.alarmmin == 0) && (RemoteCtr.imaxmin == 0))   switchon = 0;
    }
    else if((RemoteCtr.sta & F_RemoteGaoJing0) || (RemoteCtr.sta & F_RemoteGaoJing1))  //预跳闸
    {
        if(RemoteCtr.imaxmin == 0)  //大电流不拉闸
        {
            switchon = 0;
            if(RemoteCtr.limitmin == 0) //限电时间
            {
                if(RemoteCtr.sta & F_RemoteGaoJing0)    //直接合闸
                {
                    RemoteCtr.sta &= ~F_RemoteGaoJing0;
                    switchon = 1;
                }
                else if(RemoteCtr.sta & F_RemoteGaoJing1)   //合闸允许
                {
                    RemoteCtr.sta &= ~F_RemoteGaoJing1;
                    RemoteCtr.sta |= F_RemoteRelayOnAllow;
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
            //合闸
        }
        else 
        {
            //拉闸
        }
        lastswitchsta = switchon;
    }
}

