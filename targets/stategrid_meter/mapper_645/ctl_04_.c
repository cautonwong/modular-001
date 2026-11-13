#include "nidavellir.h"
#include "application/adapters/dl645/fr_645_context.h"
//#include "645id_table.h"
#include "gs_record.h"
#include "fr_axdr.h"
#include "fr_698.h"
#include "upgrade_645_97.h"
#include "kernel/legacy_control_block.h"
#include "port.h"



static uint8_t image_transfer_passwod_authentic(uint8_t *buff)
{
	uint8_t  image_transfer_passwod[4] = { 0x03 ,0xff,0xff,0xff};
	if(0x00 != mem_compare(buff,image_transfer_passwod,0x04))
	{
		return false;
	}
	return true;
}

static uint8_t process_attr(uint8_t *buff,struct pro_imange_transfer_id_t *process)
{
	uint8_t attr;

	attr = process->attr;
	if(0x01 == (attr&0x01))
	{
		if(false == image_transfer_passwod_authentic(buff))
		{
			return RS_ERR_PASS;
		}
	}
	return RS_SUCCESS;
}

static int8_t process_image_id(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret;
    uint8_t *p_err = (p_tx);
	uint8_t tx_apdu_len = 0;

	uint16_t 	id;
	uint8_t result;
	struct pro_imange_transfer_id_t *process;


    if (ctx->SA == SA_TYPE_GENERAL645)  //通配地址
    {
        return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
    }
    else if(ctx->SA == SA_TYPE_BROADECAST645)
    {
        return -1;
    }


    id = buf_to_uint16_lsb(p_rx);

    memcpy(p_tx, p_rx, 2);
    p_tx += 2;
    *tx_len = 2;

	process = (struct pro_imange_transfer_id_t *)find_image_transfer_process(id);

	if(__NULL == process)
	{
        return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
	}

	if(0xff != process->length)
	{
		if(ctx->apdu_rx_len != process->length)
		{
			return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
		}
		else
		{
		}
	}
	result = process_attr(p_rx + 2,process);
	if(RS_SUCCESS != result)
	{
		return response_x_byte_err_645(p_err, result, 1,  tx_len);
	}
	if(false == process->pro_id(instance, p_rx + 2, p_tx, &tx_apdu_len, ctx))
	{
        return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
	}
	else
	{
        *tx_len = (*tx_len) + tx_apdu_len;
        return RS_SUCCESS;
	}
	
}

static int8_t execute_image_transfer(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    return process_image_id(instance, p_rx, p_tx, tx_len, ctx);
}


// 返回0 正常应答          ＞0 其他异常应答          -1 不应答
int8_t dlt645_image_transfer(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{

    return execute_image_transfer(instance, p_rx, p_tx, tx_len, ctx);
}




