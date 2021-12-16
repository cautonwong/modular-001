#include "nidavellir.h"
#include "fr_698.h"

#define     F_RemoteNull            0x00    //合闸
#define		F_RemoteRelayOn  	    0x01	//保电
#define		F_RemoteRelayOff 	    0x02    //拉闸
#define		F_RemoteAlarm	  	    0x04    //告警
#define		F_RemoteGaoJing0	    0x08    //预跳闸
#define		F_RemoteGaoJing1	    0x10    //预跳闸
#define     F_RemoteRelayOnAllow    0x20    //合闸允许
#define     F_RemoteRelayOnDirect   0x40    //直接允许

#if 0
//协议中调用，返回：0（成功），1（失败）
unsigned char do_remotecontrol(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    unsigned char result = 0;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    switch(mbase_control_block->cmd)
    {
        case 0x1A:  //跳闸
        case 0x1D:	//代表预跳闸(延时到直接合闸允许)，
        case 0x1E:	//代表预跳闸(延时到合闸允许)，	
            if(mbase_control_block->sta & F_RemoteRelayOn) {result = 1; break;}//保电    
            if(mbase_control_block->cmd == 0x1A)
            {
                mbase_control_block->sta = F_RemoteRelayOff;
                mbase_control_block->limitmin = 0;
            }
            else
            {
                if(mbase_control_block->sta & F_RemoteRelayOff)    {result = 1; break;}    //跳闸后，下发预跳闸失败
                if(mbase_control_block->cmd == 0x1D)   mbase_control_block->sta |= F_RemoteGaoJing0;
                else if(mbase_control_block->cmd == 0x1E)   mbase_control_block->sta |= F_RemoteGaoJing1;
            }
            if()    mbase_control_block->imaxmin = ;   //大电流延时时间赋值
            break;
        case 0x1B:  //合闸允许
            mbase_control_block->sta |= F_RemoteRelayOnAllow;
            break;
        case 0x1C:	//直接合闸
            mbase_control_block->sta &= ~F_RemoteRelayOff;
            break;
        case 0x2A:	//报警
            mbase_control_block->sta |= F_RemoteAlarm;
            break;
        case 0x2B:	//报警解除
            mbase_control_block->sta &= ~F_RemoteAlarm;
            break;
        case 0x3A:	//保电
            mbase_control_block->sta |= F_RemoteRelayOn;
            break;
        case 0x3B:	//保电解除
            mbase_control_block->sta &= ~F_RemoteRelayOn;
            break;
        default:
            result = 1;
            break; 
    }
    return result;
}
#endif

int ACTION_8_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint8_t method,relayon_type=0,password[8]={0};
    uint16_t oad;
    uint32_t OI;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    switch (oad)
    {
        case 0x8000://遥控
            switch(method)//方法
            {
                case 0x7F://告警
                    mbase_cb->sta |= F_RemoteAlarm;
                    break;
                case 0x80://解除告警
                    mbase_cb->sta &= ~F_RemoteAlarm;
                    break;
                case 0x81://拉闸
                    p = p + 5;
                    mem_swap_cpy((uint8_t*)&OI, p, 4);
                    if(OI != 0xF2050201)    return -1;
                    if(mbase_cb->sta & F_RemoteRelayOn)  return -1;//保电   
                    p = p + 5;
                    mbase_cb->alarmmin = (uint32_t)(*p);       //拉闸延时时间
                    mbase_cb->alarmmin = mbase_cb->alarmmin*60;
                    mbase_cb->limitmin = 0;
                    p = p + 2;
                    mem_swap_cpy((uint8_t*)&mbase_cb->limitmin, p, 2);//预跳闸时间
                    mbase_cb->limitmin = mbase_cb->limitmin*60;
                    p = p + 3;
                    relayon_type = *p++;//预跳闸后合闸方式
                    if(mbase_cb->limitmin == 0)//拉闸
                    {
                        mbase_cb->sta |= F_RemoteRelayOff;
                    }
                    else
                    {
                        if(mbase_cb->sta & F_RemoteRelayOff)     return -1;    //跳闸后，下发预跳闸失败
                        if(relayon_type == 0)   mbase_cb->sta |= F_RemoteGaoJing0;
                        else if(relayon_type == 1)   mbase_cb->sta |= F_RemoteGaoJing1;
                    }
                    //if()    mbase_control_block->imaxmin = ;   //大电流延时时间赋值
                    break;
                case 0x82://合闸
                case 0x83://明文合闸
                    p = p + 5;
                    mem_swap_cpy((uint8_t*)&OI, p, 4);
                    p = p + 5;
                    if(OI != 0xF2050201)    return -1;
                    if(method == 0x83)
                    {
                        mem_swap_cpy(password, p+3, 8);//判断密码
                    }
                    if(*p == 0) 
                        mbase_cb->sta |= F_RemoteRelayOnAllow;  //合闸允许
                    else
                        mbase_cb->sta |= F_RemoteRelayOnDirect; //直接合闸
                    mbase_cb->sta &= ~F_RemoteRelayOff;
                    break;
                default:
                    break;
            }
            break;
        case 0x8001://保电
            switch(method)
            {
                case 0x7F://投入保电
                    mbase_cb->sta |= F_RemoteRelayOn;
                    break;
                case 0x80://解除保电
                    mbase_cb->sta &= ~F_RemoteRelayOn;
                    break;
                default:
                    return -1;
                    break;
            }
            break;
        default:
            break;
    }
    return 0x01;
}
