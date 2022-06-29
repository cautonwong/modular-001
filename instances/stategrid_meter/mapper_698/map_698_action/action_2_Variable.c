#include "nidavellir.h"
#include "fr_698.h"
#ifndef WIN32
#include "gs_record.h"
#endif // !WIN32


int ACTION_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct context_698_t* p_ctx = (struct context_698_t*)ctx;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct pipeline_t* pipeline;
    uint8_t  method;
    uint16_t oad;
    uint8_t *p = buf;
    uint32_t Temp32A;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
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
        case 0x2015:
            if(method == 127)// 上报状态确认
            {
                if ((p[0] != TYPE_bit_string) || (p[1] != 32))
                {
                    return -DAR_TYPE_NOT_MATCH;
                }   
                p += 2;

                Temp32A = buf_to_uint32_msb(p);
                mbase_cb->report_2015state[p_ctx->channel] &= ~Temp32A;
                EventOut_698_check(instance);
                return (p-buf);
            }                
            break;
        
        default:
            break;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
