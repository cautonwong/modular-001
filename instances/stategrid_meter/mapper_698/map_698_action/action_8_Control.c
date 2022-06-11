#include "nidavellir.h"
#include "fr_698.h"

int ACTION_8_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *pctx)
{   
    struct context_698_t* ctx = (struct context_698_t*)pctx;
    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint8_t method,relayon_type=0,password[8]={0},password_des[8]={0};
    uint16_t oad,Prot_time=0;
    uint32_t OI,addr,I_Prot=0;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }
    switch (oad)
    {
        case 0x8000://遥控
            if(method != 0x83)  //明文合闸除外
            {
                if( mbase_cb->mac_suspend_state == true )   //挂起
                {
                    mbase_cb->cmd_errstate = 0x8000;    //挂起
                    return  -DAR_METER_SUSPEND;
                }
            }
            if(mbase_cb->authen_tmr == 0)   //安全认证超时
            {
                mbase_cb->cmd_errstate = 0x1000;    //安全认证超时
                return  -DAR_SECURITY_AUTH_FAILED;
            }
            switch(method)//方法
            {
                case 0x7F://告警
                    mbase_cb->sta |= F_RemoteAlarm;
                    mbase_cb->cmd_dostate = 0x0020;  //bit10;
                    return 1;
                    break;
                case 0x80://解除告警
                    mbase_cb->sta &= ~F_RemoteAlarm;
                    mbase_cb->cmd_dostate = 0x0010;  //bit11;
                    return 1;
                    break;
                case 0x81://拉闸
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        return -DAR_TIME_TAG_INVALID;
                    }
                    p = p + 5;
                    mem_swap_cpy((uint8_t*)&OI, p, 4);
                    if(OI != 0xF2050201)    return -DAR_OTHERS_ERROR;
                    p = p + 5;
                    mbase_cb->alarmmin = (uint32_t)(*p);       //拉闸延时时间
                    mbase_cb->alarmmin = mbase_cb->alarmmin*60;
                    mbase_cb->limitmin = 0;
                    p = p + 2;
                    mem_swap_cpy((uint8_t*)&mbase_cb->limitmin, p, 2);//预跳闸时间
                    mbase_cb->limitmin = mbase_cb->limitmin*60;
                    p = p + 3;
                    relayon_type = *p++;//预跳闸后合闸方式
                    if(mbase_cb->sta & F_RemoteRelayOn)  
                    {
                        if(mbase_cb->limitmin == 0) 
                            mbase_cb->cmd_errstate = 0x0400;//bit5 跳闸失败（保电）
                        else
                            mbase_cb->cmd_errstate = 0x0200;//bit6 跳闸自动恢复失败（保电）
                        return -DAR_OTHERS_ERROR;//保电   
                    }
                    if(mbase_cb->limitmin == 0)//拉闸
                    {
                        mbase_cb->sta |= F_RemoteRelayOff;
                        if(mbase_cb->alarmmin == 0) //直接跳闸
                            mbase_cb->cmd_dostate = 0x2000;  //bit2;
                        else    //延时跳闸
                            mbase_cb->cmd_dostate = 0x1000;  //bit3;
                    }
                    else
                    {
                        if(mbase_cb->sta & F_RemoteRelayOff)     
                        {
                            mbase_cb->cmd_errstate = 0x0080;  //bit8;
                            return -DAR_OTHERS_ERROR;    //跳闸后，下发预跳闸失败
                        }
                        if(relayon_type == 0)   mbase_cb->sta |= F_RemoteGaoJing0;
                        else if(relayon_type == 1)   mbase_cb->sta |= F_RemoteGaoJing1;
                        mbase_cb->cmd_dostate = 0x0800;  //bit4;
                    }
                    //大电流延时时间赋值
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                    nd_get_index(instance, addr, password, 4); 
                    mem_swap_cpy((uint8_t*)&I_Prot, password, 4);
                    I_Prot = I_Prot / 10;
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                    nd_get_index(instance, addr, (uint8_t*)&password, 2); 
                    mem_swap_cpy((uint8_t*)&Prot_time, password, 2);
                    if(mbase_cb->current_l > I_Prot)    
                    {
                        mbase_cb->imaxmin = Prot_time;   
                        mbase_cb->imaxmin = mbase_cb->imaxmin*60;
                        mbase_cb->cmd_dostate = 0x0400;  //bit5;
                    }
                    return (p-buf);
                    break;
                case 0x82://合闸
                case 0x83://明文合闸
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        return -DAR_TIME_TAG_INVALID;
                    }
                    
                    p = p + 5;
                    mem_swap_cpy((uint8_t*)&OI, p, 4);
                    p = p + 5;
                    if(OI != 0xF2050201)    return -DAR_OTHERS_ERROR;

                    if(method == 0x83)
                    {
                        memcpy(password, p+3, 8);//判断密码
                        //p = p + 3;
                        
                        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Password);
                        nd_get_index(instance, addr, password_des, 8); 
                        
                        if(mem_compare(password_des, password, 8)!=0)
                        {
                            mbase_cb->cmd_errstate = 0x2000;//bit2 密码错未授权
                            return -DAR_UNAUTHORIZED;
                        }
                    }
                    if(*p == 0) 
                    {
                        mbase_cb->sta |= F_RemoteRelayOnAllow;  //合闸允许
                        mbase_cb->cmd_dostate = 0x4000;  //bit1;
                    }
                    else
                    {
                        mbase_cb->sta |= F_RemoteRelayOnDirect; //直接合闸
                        mbase_cb->cmd_dostate = 0x8000;  //bit0;
                    }
                    p++;
                    if(method == 0x83) p = p+10;
                    mbase_cb->sta &= ~F_RemoteRelayOff;
                    
                    return (p-buf);
                    break;
                default:
                    break;
            }
            break;
        case 0x8001://保电
            switch(method)
            {
                case 0x7F://投入保电
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        return -DAR_TIME_TAG_INVALID;
                    }
                    mbase_cb->sta |= F_RemoteRelayOn;
                    mbase_cb->cmd_dostate = 0x0080;  //bit8;
                    return 1;
                    break;
                case 0x80://解除保电
                    mbase_cb->sta &= ~F_RemoteRelayOn;
                    mbase_cb->cmd_dostate = 0x0040;  //bit9;
                    return 1;
                    break;
                default:
                    return -DAR_OTHERS_ERROR;
                    break;
            }
            break;
        default:
            break;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
