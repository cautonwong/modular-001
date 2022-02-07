#include "nidavellir.h"
#include "fr_698.h"
#include "gs_record.h"


int ACTION_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    struct pipeline_t* pipeline;
    uint8_t  method,dar=0;
    uint16_t oad;
    uint8_t *p = buf;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    pipeline = get_pipeline(instance, pid_meter_base);
    if(pipeline == NULL)
    {
        return -1;
    }  
    
    switch (oad)
    {
        case 0x4300:
            switch(method)
            {
                case 0x04://出厂参数初始化
                    para_init( instance );
                    //后面替换为meter_clr
                    freeze_clr(instance); //冻结清零
                    event_clr(instance);  // 事件清零
					p = p + 7;
                    return (p-buf);
                    break;
                case 0x03://数据初始化（电表清零）

                    // 时间标签 标志  待调整
#if 0
                    if ((mbase_cb->time_tag == false) && (mbase_cb->factor_mode == false))
                    {
                        return (1-DAR_TIME_TAG_INVALID);
                    }
#endif
#if 0
                    if ((mbase_cb->SecretKey_state == 0) && (mbase_cb->factor_mode == false))//0私钥，1公钥，私钥不允许清零
                    {
                        return (1 - DAR_UNAUTHORIZED);
                    }
#endif
                    p++;
                    send_event_mesg_pro(instance, NULL, MeterClrS, NULL, 1);
                    return (p-buf);
                    break;
                case 0x05://事件初始化
#if 0
                    if ((mbase_cb->time_tag == false) && (mbase_cb->factor_mode == false))
                    {
                        return (1 - DAR_TIME_TAG_INVALID);
                    }
#endif
                    event_clear_omd_add(instance, id, 0 /*req->channel */); //待调整    如何在map  case中  取到通道号
                    event_clr(instance);// 事件清零
					p++;
                    send_event_mesg_pro(instance, NULL, EventClrS, NULL, 1);
                    return (p-buf);
                    break;
                default:
                    dar = DAR_OBJECT_NOT_EXIST;
                    break;
            }
            break;
        default:
            break;
    }
    return 0x01;
}
