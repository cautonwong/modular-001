#include "nidavellir.h"
#include "fr_698.h"


//设置、操作 时间标签错误时，置位相关状态字
void app_handle_timeTag_invalid(struct instance_t* instance, uint32_t oad)
{
    uint16_t oi = get_OAD_OI(oad);

    switch (oi)
    {
        case 0x8000://遥控
        case 0x8001://保电

            break;
        
    }

}

//挂起时，置位相关状态字
void app_handle_mac_suspend(struct instance_t* instance, uint32_t oad)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint16_t oi = get_OAD_OI(oad);

    switch (oi)
    {
        case 0x8000://遥控
        case 0x8001://保电
            mbase_cb->cmd_errstate = 0x8000;    //挂起 2041 错误信息字
            break;
        default:
        break;
    }

}


int ACTION_8_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *pctx)
{   
    struct context_698_t* ctx = (struct context_698_t*)pctx;
    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint8_t method,relayon_type=0,password[8]={0},password_des[8]={0},client_index = 0;
    uint16_t oad;
    uint32_t OI,addr=0,Alarm_min=0;


    if (CA_TYPE_TER(ctx->CA))//终端
    {
        client_index = 1;
    }
    else
    {
        client_index = 0;
    }

    
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
            /*  如果开启 需要判断 安全模式字
            if((fr_698_current_connect_status_get(instance, NULL, client_index)== false) && (mbase_cb->factor_mode != true))   //安全认证超时
            {
                mbase_cb->cmd_errstate = 0x1000;    //安全认证超时
                return  -DAR_SECURITY_AUTH_FAILED;
            }
            */
            switch(method)//方法
            {
                case 0x7F://告警
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(mbase_cb->factor_mode == false)
                            return -DAR_TIME_TAG_INVALID;
                    }
                    mbase_cb->sta |= F_RemoteAlarm;
                    mbase_cb->cmd_errstate = 0;
                    mbase_cb->cmd_dostate = 0x0020;  //bit10;
                    return 1;
                    break;
                case 0x80://解除告警
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(mbase_cb->factor_mode == false)
                            return -DAR_TIME_TAG_INVALID;

                    }
                    mbase_cb->sta &= ~F_RemoteAlarm;
                    mbase_cb->cmd_errstate = 0;
                    mbase_cb->cmd_dostate = 0x0010;  //bit11;
                    return 1;
                    break;
                case 0x81://拉闸
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(mbase_cb->factor_mode == false)
                            return -DAR_TIME_TAG_INVALID;

                    }
                    p = p + 5;
                    mem_swap_cpy((uint8_t*)&OI, p, 4);
                    if(OI != 0xF2050201)    return -DAR_OTHERS_ERROR;
                    p = p + 5;
                    Alarm_min = (uint32_t)(*p);       //拉闸延时时间  mbase_cb->alarmmin
                    Alarm_min = Alarm_min*60;
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
                        if(mbase_cb->sta & F_RemoteRelayOnAllow)    //合闸允许状态下发拉闸命令，取消合闸允许状态
                        {
                            mbase_cb->sta &= ~F_RemoteRelayOnAllow;
                        }
                        if(Alarm_min == 0) //直接跳闸
                            mbase_cb->cmd_dostate = 0x2000;  //bit2;
                        else    //延时跳闸
                            mbase_cb->cmd_dostate = 0x1000;  //bit3;
                        
                    }
                    else
                    {
                        if((mbase_cb->sta & F_RemoteRelayOff) //本地跳闸 待添加
                        ||(mbase_cb->sta & F_RemoteRelayOnAllow))    
                        {
                            mbase_cb->cmd_errstate = 0x0080;  //bit8;
                            return -DAR_OTHERS_ERROR;   //跳闸后，下发预跳闸失败
                        }
                        Alarm_min = 0; //预调闸命令 清延时
                        if(relayon_type == 0)   mbase_cb->sta |= F_RemoteGaoJing1;
                        else if(relayon_type == 1)   mbase_cb->sta |= F_RemoteGaoJing0;
                        mbase_cb->cmd_dostate = 0x0800;  //bit4;
                    }
                    mbase_cb->alarmmin = Alarm_min;
                    //大电流延时时间赋值
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                    nd_get_index(instance, addr, password, 4); 
                    mem_swap_cpy((uint8_t*)&mbase_cb->I_Prot, password, 4);
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                    nd_get_index(instance, addr, (uint8_t*)password, 2); 
                    mem_swap_cpy((uint8_t*)&mbase_cb->Prot_time, password, 2);
                    if((abs(mbase_cb->current_l) > (mbase_cb->I_Prot / 10)) && (mbase_cb->I_Prot != 0))
                    {
                        mbase_cb->imaxmin = mbase_cb->Prot_time;   
                        mbase_cb->imaxmin = mbase_cb->imaxmin*60;
                        mbase_cb->cmd_dostate = 0x0400;  //bit5;
                    }
                    else
                    {
                        mbase_cb->imaxmin = 0;
                    }
                    mbase_cb->cmd_errstate = 0;
                    mbase_cb->relay_do = 0xA5; 
                    return (p-buf);
                    break;
                case 0x82://合闸
                case 0x83://明文合闸
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(mbase_cb->factor_mode == false)
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
                        
                        if((mem_compare(password_des, password, 8)!=0) || (mbase_cb->password_err_cnt >= 3))
                        {
                            if(mbase_cb->password_err_cnt < 3)  mbase_cb->password_err_cnt++;
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
                        mbase_cb->sta &= ~F_RemoteRelayOnAllow;
                        mbase_cb->cmd_dostate = 0x8000;  //bit0;
                    }
                    mbase_cb->sta &= ~(F_RemoteGaoJing0 + F_RemoteGaoJing1);
                    p++;
                    if(method == 0x83) p = p+10;
                    mbase_cb->sta &= ~F_RemoteRelayOff;
                    
                    if(mbase_cb->imaxmin > 0)   mbase_cb->imaxmin = 0;   //大电流延时，清除大电流延时时间
                    
                    if(mbase_cb->alarmmin > 0)  mbase_cb->alarmmin = 0;     //跳闸延时，清零

                    mbase_cb->relay_do = 0xA5; 
                    return (p-buf);
                    break;
                default:
                    break;
            }
            break;
        case 0x8001://保电
            if( mbase_cb->mac_suspend_state == true )   //挂起
            {
                mbase_cb->cmd_errstate = 0x8000;    //挂起
                return  -DAR_METER_SUSPEND;
            }
            switch(method)
            {
                case 0x7F://投入保电
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(mbase_cb->factor_mode == false)
                            return -DAR_TIME_TAG_INVALID;

                    }
                    //if((mbase_cb->sta & F_RemoteRelayOff) && (mbase_cb->alarmmin > 0))//拉闸延时过程中，保电后拉闸字符消失，灯灭
                    {
                        mbase_cb->alarmmin = 0;
                        mbase_cb->imaxmin = 0;
                        mbase_cb->sta &= ~F_RemoteRelayOff;
                    }
                    
                    mbase_cb->sta |= F_RemoteRelayOn;
                    if(mbase_cb->sta & (F_RemoteGaoJing1 + F_RemoteGaoJing0))
                    {
                        mbase_cb->sta &= ~(F_RemoteGaoJing1 + F_RemoteGaoJing0);
                        mbase_cb->limitmin = 0;
                        mbase_cb->sta |= F_RemoteRelayOnAllow;  //合闸允许
                    }
                    mbase_cb->cmd_errstate = 0;
                    mbase_cb->cmd_dostate = 0x0080;  //bit8;
                    return 1;
                    break;
                case 0x80://解除保电
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(mbase_cb->factor_mode == false)
                            return -DAR_TIME_TAG_INVALID;

                    }
                    mbase_cb->sta &= ~F_RemoteRelayOn;
                    mbase_cb->cmd_errstate = 0;
                    mbase_cb->cmd_dostate = 0x0040;  //bit9;
                    return 1;
                    break;
                default:
                    return -DAR_RW_REJECT;
                    break;
            }
            break;
        default:
            break;
    }
    return -DAR_RW_REJECT;
}
