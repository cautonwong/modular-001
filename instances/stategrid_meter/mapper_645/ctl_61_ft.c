#include "nidavellir.h"
#include "645id_table.h"

extern int rn8209c_set( struct instance_t* instance, uint8_t *p_rx );

signed char ft_adj_set_meter(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len)
{
    uint16_t data = 0;
    uint8_t addr = 0,buf[30]={0,};
    int err = 0;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if(mbase_cb == NULL)
    {
        *p_tx = RS_ERR_OTHERS;
        return 1;
    }
    buf[16] = *p_rx;
    mem_swap_cpy((uint8_t*)&buf[12], p_rx+1, 2);
    
    err = rn8209c_set(instance, buf);
    return err;
}

