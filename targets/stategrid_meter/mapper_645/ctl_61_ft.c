#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "targets/stategrid_meter/persistence/meter_base_view.h"
#include "645id_table.h"

signed char ft_adj_set_meter(void *instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len)
{
    uint16_t data = 0;
    uint8_t addr = 0,buf[30]={0,};
    int err = 0;
    
    if(!nd_mb_valid(instance))
    {
        *p_tx = RS_ERR_OTHERS;
        return 1;
    }
    buf[4] = *p_rx;
    switch(buf[4])
    {
        case 0x02:
        case 0x03:
        case 0x05:
        case 0x0A:
        case 0x0E:
        case 0x0F:
        case 0x17:
            mem_swap_cpy(buf, p_rx+1, 2);
            break;
        case 0x07:
            buf[0] = *(p_rx+1);
            break;
        default:
            *p_tx = RS_ERR_OTHERS;
            return -1;
    }
    err = rn8209c_set(instance, buf);
    if(err == 1) return 0;
    else return 1;
}

signed char ft_adj_get_meter(void *instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len)
{
    uint8_t buf[5]={0,};
    uint8_t err = 0;
    
    if(!nd_mb_valid(instance))
    {
        *p_tx = RS_ERR_OTHERS;
        return 1;
    }
    
    err = rn8209c_get(instance, p_rx, buf);
    if(err != 0)
    {
        mem_swap_cpy(p_tx+1, (uint8_t*)&buf[1], err);
    }
    *len = 5;
    
    if(err == 0)
        return -1;
    else
        return 0;
}

