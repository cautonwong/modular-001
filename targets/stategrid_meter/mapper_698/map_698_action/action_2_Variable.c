#include "targets/stategrid_meter/persistence/meter_record.h"
#include "fr_698.h"
#include "application/adapters/dl698/meter_base_view.h"
#ifndef WIN32
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "fr_645_report.h"
#endif // !WIN32


int ACTION_2_Variable(void *instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct context_698_t* p_ctx = (struct context_698_t*)ctx;
    uint8_t  method;
    uint16_t oad;
    uint8_t *p = buf;
    uint32_t Temp32A;
    uint8_t channel = 0;
    
    if((p_ctx)&&(p_ctx->channel < FR_698_CHL_NUM))
    {
        channel = p_ctx->channel;
    }
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
    if(!nd_mb_valid(instance))
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
                nd_mb_report_2015_state(instance)[channel] &= ~Temp32A;
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
