#include "nidavellir.h"
#include "fr_698.h"

int ACTION_8_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *pctx)
{   
    struct context_698_t* ctx = (struct context_698_t*)pctx;
    
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
        return -DAR_OTHERS_ERROR;
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
                    if((1 != ctx->timeTag_valid) || (ctx->timeTag[0] == 0)){
                        return -DAR_TIME_TAG_INVALID;
                    }
                    p = p + 5;
                    mem_swap_cpy((uint8_t*)&OI, p, 4);
                    if(OI != 0xF2050201)    return -DAR_OTHERS_ERROR;
                    if(mbase_cb->sta & F_RemoteRelayOn)  return -DAR_OTHERS_ERROR;//保电   
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
                        if(mbase_cb->sta & F_RemoteRelayOff)     return -DAR_OTHERS_ERROR;    //跳闸后，下发预跳闸失败
                        if(relayon_type == 0)   mbase_cb->sta |= F_RemoteGaoJing0;
                        else if(relayon_type == 1)   mbase_cb->sta |= F_RemoteGaoJing1;
                    }
                    
                    return (p-buf);
                    //if()    mbase_control_block->imaxmin = ;   //大电流延时时间赋值
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
                        mem_swap_cpy(password, p+3, 8);//判断密码
                        p = p + 3;
                    }
                    if(*p == 0) 
                        mbase_cb->sta |= F_RemoteRelayOnAllow;  //合闸允许
                    else
                        mbase_cb->sta |= F_RemoteRelayOnDirect; //直接合闸
                    p++;
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
                    return 1;
                    break;
                case 0x80://解除保电
                    mbase_cb->sta &= ~F_RemoteRelayOn;
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
