#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "fr_698.h"
#ifndef WIN32
#include "gs_record.h"
#include "fr_645_report.h"
#include "kernel/legacy_control_block.h"
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
    uint8_t channel = 0;
    
    if((p_ctx)&&(p_ctx->channel < chl_NUM))
    {
        channel = p_ctx->channel;
    }
    
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
                p += 4;
                nd_mbase_report_2015state(mbase_cb)[channel] &= ~Temp32A;
                report_stat_reset_from_698(instance,  Temp32A);
                EventOut_698_check(instance);
                return (p-buf);
            }                
            break;
        
        default:
            break;
    }
    return -DAR_RW_REJECT;
}
