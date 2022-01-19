#include "nidavellir.h"
#include "fr_698.h"
#include "gs_record.h"


int ACTION_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
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
                    p++;
                    return (p-buf);
                    break;
                case 0x05://事件初始化
                    event_clr(instance);// 事件清零
					p++;
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
