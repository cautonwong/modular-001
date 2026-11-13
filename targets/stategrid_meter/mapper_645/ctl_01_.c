#include "nidavellir.h"
#include "application/adapters/dl645/fr_645_context.h"
//#include "645id_table.h"
#include "gs_record.h"
#include "fr_axdr.h"
#include "fr_698.h"
#include "upgrade_645_97.h"
#include "kernel/legacy_control_block.h"
#include "port.h"



// 返回0 正常应答          ＞0 其他异常应答          -1 不应答
int8_t dlt645_image_read(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret;
    uint8_t *p_err = (p_tx);
	uint8_t tx_apdu_len = 0;

	uint16_t 	id;
	uint8_t result;

	const struct image_read_id_t  *process = __NULL;


    if (ctx->SA == SA_TYPE_GENERAL645)  //通配地址
    {
        //return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
    }
    else if(ctx->SA == SA_TYPE_BROADECAST645)
    {
        return -1;
    }


    id = buf_to_uint16_lsb(p_rx);
    memcpy(p_tx, p_rx, 2);
    p_tx += 2;
    *tx_len = 2;


    process = find_image_read_id_processor(id);

    if (__NULL != process)
    {
        if (__NULL != process->pro_down_ops)
        {
            if (false == process->pro_down_ops(instance, p_rx, p_tx, tx_len, ctx))
            {
                return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
            }
        }

        if (__NULL != process->pro_up_ops)
        {
            if (false == process->pro_up_ops(instance, p_rx, p_tx, tx_len, ctx))
            {
                return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
            }
        }

        *tx_len = (*tx_len) + tx_apdu_len;
        return RS_SUCCESS;
    }

    return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);

}




