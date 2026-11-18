#include "targets/stategrid_meter/app/relay_control_adapter.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include <stdbool.h>
#include "fr_698.h"
#include "application/adapters/dl698/meter_base_view.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/app/resource_blocks.h"



//设置、操作 时间标签错误时，置位相关状态字
void app_handle_timeTag_invalid(void *instance, uint32_t oad)
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
void app_handle_mac_suspend(void *instance, uint32_t oad)
{

    uint16_t oi = get_OAD_OI(oad);

    switch (oi)
    {
        case 0x8000://遥控
        case 0x8001://保电
            nd_mb_cmd_errstate(instance) = 0x8000;    //挂起 2041 错误信息字
            nd_mb_cmd_dostate(instance) = 0;   
            break;
        default:
        break;
    }

}


int ACTION_8_Parameter(void *instance, uint32_t id, uint8_t* buf, void *pctx)
{   
    struct context_698_t* ctx = (struct context_698_t*)pctx;
    
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
    
    if(!nd_mb_valid(instance))
    {
        return -DAR_OTHERS_ERROR;
    }
    switch (oad)
    {
        case 0x8000://遥控
            if(method != 0x83)  //明文合闸除外
            {
                if( nd_mb_mac_suspend_state(instance) == true )   //挂起
                {
                    nd_mb_cmd_errstate(instance) = 0x8000;    //挂起
                    nd_mb_cmd_dostate(instance) = 0; 
                    return  -DAR_METER_SUSPEND;
                }
            }
            /*  如果开启 需要判断 安全模式字
            if((fr_698_current_connect_status_get(instance, NULL, client_index)== false) && (nd_mb_factor_mode(instance) != true))   //安全认证超时
            {
                nd_mb_cmd_errstate(instance) = 0x1000;    //安全认证超时
                return  -DAR_SECURITY_AUTH_FAILED;
            }
            */
            switch(method)//方法
            {
                case 0x7F://告警
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(nd_mb_factor_mode(instance) == false)
                            return -DAR_TIME_TAG_INVALID;
                    }
                    nd_mb_sta(instance) |= F_RemoteAlarm;
                    nd_mb_cmd_errstate(instance) = 0;
                    nd_mb_cmd_dostate(instance) = 0x0020;  //bit10;
                    return 1;
                    break;
                case 0x80://解除告警
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(nd_mb_factor_mode(instance) == false)
                            return -DAR_TIME_TAG_INVALID;

                    }
                    nd_mb_sta(instance) &= ~F_RemoteAlarm;
                    nd_mb_cmd_errstate(instance) = 0;
                    nd_mb_cmd_dostate(instance) = 0x0010;  //bit11;
                    return 1;
                    break;
                case 0x81://拉闸
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(nd_mb_factor_mode(instance) == false)
                            return -DAR_TIME_TAG_INVALID;

                    }
                    nd_mb_cmd_dostate(instance) = 0; //先清零
                    p = p + 5;
                    mem_swap_cpy((uint8_t*)&OI, p, 4);
                    if(OI != 0xF2050201)    return -DAR_OTHERS_ERROR;
                    p = p + 5;
                    Alarm_min = (uint32_t)(*p);       //拉闸延时时间  nd_mb_alarmmin(instance)
                    Alarm_min = Alarm_min*60;
                    nd_mb_limitmin(instance) = 0;
                    p = p + 2;
                    mem_swap_cpy((uint8_t*)&nd_mb_limitmin(instance), p, 2);//预跳闸时间
                    nd_mb_limitmin(instance) = nd_mb_limitmin(instance)*60;
                    p = p + 3;
                    relayon_type = *p++;//预跳闸后合闸方式
                    if(nd_mb_sta(instance) & F_RemoteRelayOn)  
                    {
                        nd_mb_cmd_dostate(instance) = 0;  
                        if(nd_mb_limitmin(instance) == 0) 
                            nd_mb_cmd_errstate(instance) = 0x0400;//bit5 跳闸失败（保电）
                        else
                            nd_mb_cmd_errstate(instance) = 0x0200;//bit6 跳闸自动恢复失败（保电）
                           
                        return -DAR_OTHERS_ERROR;//保电   
                    }
                    if(nd_mb_limitmin(instance) == 0)//拉闸
                    {
                        if(nd_mb_sta(instance) & (F_RemoteRelayOff + F_RemoteRelayOnAllow + F_RemoteGaoJing0 + F_RemoteGaoJing1)) //已经在拉闸情况下，清延时，避免拉闸字符闪烁，
                        {
                            Alarm_min = 0;
                        }
                        nd_mb_sta(instance) |= F_RemoteRelayOff;
                        if(nd_mb_sta(instance) & F_RemoteRelayOnAllow)    //合闸允许状态下发拉闸命令，取消合闸允许状态
                        {
                            nd_mb_sta(instance) &= ~F_RemoteRelayOnAllow;
                        }
                        if(Alarm_min == 0) //直接跳闸
                            nd_mb_cmd_dostate(instance) = 0x2000;  //bit2;
                        else    //延时跳闸
                            nd_mb_cmd_dostate(instance) = 0x1000;  //bit3;
                        
                    }
                    else  //跳闸自动恢复
                    {
                        if((nd_mb_sta(instance) & F_RemoteRelayOff) //本地跳闸 待添加
                        ||(nd_mb_sta(instance) & F_RemoteRelayOnAllow))    
                        {
                            nd_mb_cmd_errstate(instance) = 0x0080;  //bit8;
                            nd_mb_cmd_dostate(instance) = 0;  
                            return -DAR_OTHERS_ERROR;   //跳闸后，下发预跳闸失败
                        }
                        Alarm_min = 0; //预调闸命令 清延时
                        nd_mb_sta(instance) &= ~(F_RemoteGaoJing1 + F_RemoteGaoJing0);
                        if(relayon_type == 0)   nd_mb_sta(instance) |= F_RemoteGaoJing1;
                        else if(relayon_type == 1)   nd_mb_sta(instance) |= F_RemoteGaoJing0;
                        nd_mb_cmd_dostate(instance) = 0x0800;  //bit4;
                    }
                    nd_mb_alarmmin(instance) = Alarm_min;
                    //大电流延时时间赋值
                    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                    nd_get_index(instance, addr, password, 4); 
                    mem_swap_cpy((uint8_t*)&nd_mb_i_prot(instance), password, 4);
                    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                    nd_get_index(instance, addr, (uint8_t*)password, 2); 
                    mem_swap_cpy((uint8_t*)&nd_mb_prot_time(instance), password, 2);
                    if((abs(nd_mb_current_l(instance)) > (nd_mb_i_prot(instance) / 10)) && (nd_mb_i_prot(instance) != 0))
                    {
                        nd_mb_imaxmin(instance) = nd_mb_prot_time(instance);   
                        nd_mb_imaxmin(instance) = nd_mb_imaxmin(instance)*60;
                        if(nd_mb_cmd_dostate(instance) != 0x1000) //大电流延时和延时跳闸同时在，报延时跳闸
                        {
                            nd_mb_cmd_dostate(instance) = 0x0400;  //bit5;
                        }
                    }
                    else
                    {
                        nd_mb_imaxmin(instance) = 0;
                    }
                    nd_mb_cmd_errstate(instance) = 0;
                    nd_mb_relay_do(instance) = 0xA5; 
                    save_relay_state(instance);
                    return (p-buf);
                    break;
                case 0x82://合闸
                case 0x83://明文合闸
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(nd_mb_factor_mode(instance) == false)
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
                        
                        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Password);
                        nd_get_index(instance, addr, password_des, 8); 
                        
                        if((mem_compare(password_des, password, 8)!=0) || (nd_mb_password_err_cnt(instance) >= 3))
                        {
                            if(nd_mb_password_err_cnt(instance) < 3)  nd_mb_password_err_cnt(instance)++;
                            nd_mb_cmd_errstate(instance) = 0x2000;//bit2 密码错未授权
                            nd_mb_cmd_dostate(instance) = 0;
                            return -DAR_UNAUTHORIZED;
                        }
                        else //成功后清0
                        {
                            nd_mb_password_err_cnt(instance) = 0;
                        }
                    }
                    if(*p == 0) 
                    {
                        nd_mb_sta(instance) |= F_RemoteRelayOnAllow;  //合闸允许
                        nd_mb_cmd_dostate(instance) = 0x4000;  //bit1;
                    }
                    else
                    {
                        nd_mb_sta(instance) |= F_RemoteRelayOnDirect; //直接合闸
                        nd_mb_sta(instance) &= ~F_RemoteRelayOnAllow;
                        nd_mb_cmd_dostate(instance) = 0x8000;  //bit0;
                    }
                    nd_mb_sta(instance) &= ~(F_RemoteGaoJing0 + F_RemoteGaoJing1);
                    p++;
                    if(method == 0x83) p = p+10;
                    nd_mb_sta(instance) &= ~F_RemoteRelayOff;
                    
                    if(nd_mb_imaxmin(instance) > 0)   nd_mb_imaxmin(instance) = 0;   //大电流延时，清除大电流延时时间
                    
                    if(nd_mb_alarmmin(instance) > 0)  nd_mb_alarmmin(instance) = 0;     //跳闸延时，清零

                    nd_mb_cmd_errstate(instance) = 0;

                    nd_mb_relay_do(instance) = 0xA5; 
                    save_relay_state(instance);
                    return (p-buf);
                    break;
                default:
                    break;
            }
            break;
        case 0x8001://保电
            if( nd_mb_mac_suspend_state(instance) == true )   //挂起
            {
                nd_mb_cmd_errstate(instance) = 0x8000;    //挂起
                return  -DAR_METER_SUSPEND;
            }
            switch(method)
            {
                case 0x7F://投入保电
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(nd_mb_factor_mode(instance) == false)
                            return -DAR_TIME_TAG_INVALID;

                    }
                    //if((nd_mb_sta(instance) & F_RemoteRelayOff) && (nd_mb_alarmmin(instance) > 0))//拉闸延时过程中，保电后拉闸字符消失，灯灭
                    {
                        nd_mb_alarmmin(instance) = 0;
                        nd_mb_imaxmin(instance) = 0;
                        nd_mb_limitmin(instance) = 0;
                    }
                    
                    nd_mb_sta(instance) |= F_RemoteRelayOn;

                    if(nd_mb_metertype(instance) & (F_RelayonAuto_En + F_ExternRelay_En))  //合闸允许自动合闸   、外置表自动合闸
                    {
                        nd_mb_sta(instance) |= F_RemoteRelayOnDirect; //直接合闸
                        nd_mb_sta(instance) &= ~F_RemoteRelayOnAllow;
                    }
                    else
                    {
                        if((nd_mb_sta(instance) & (F_RemoteRelayOff + F_RemoteGaoJing1 + F_RemoteGaoJing0))) //只有在拉闸状态下才需要合闸允许
                            nd_mb_sta(instance) |= F_RemoteRelayOnAllow;  //合闸允许
                    }
                    
                    nd_mb_sta(instance) &= ~(F_RemoteRelayOff + F_RemoteGaoJing1 + F_RemoteGaoJing0);//最后取消拉闸命令

                    nd_mb_cmd_errstate(instance) = 0;
                    nd_mb_cmd_dostate(instance) = 0x0080;  //bit8;
                    save_relay_state(instance);
                    return 1;
                    break;
                case 0x80://解除保电
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        if(nd_mb_factor_mode(instance) == false)
                            return -DAR_TIME_TAG_INVALID;

                    }
                    nd_mb_sta(instance) &= ~F_RemoteRelayOn;
                    nd_mb_cmd_errstate(instance) = 0;
                    nd_mb_cmd_dostate(instance) = 0x0040;  //bit9;
                    save_relay_state(instance);
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
