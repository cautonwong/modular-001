#include "nidavellir.h"
#include "645id_table.h"

extern int16_t Check_para_Pro(struct instance_t* instance);

/***********************************************************************/
/*  函数名称：Comm_Other                                               */
/*  函数功能：扩展命令                                              */
/*  输出参数：读结果：0为正确,其余为错误                              */
/***********************************************************************/
signed char Comm_Other(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len)
{
    signed char ret=0;
	unsigned char Err_Byte=0, rx_len=0;
    uint32_t s_index=0;
	struct mbase_control_block_t* mbase_cb;
    int16_t err=0;
	
	mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        *p_tx = RS_ERR_OTHERS;
        return 1;
    }
    
    err = Check_para_Pro(instance);

	rx_len = *(p_rx-1);
	
    switch(rx_len)
	{
		case 0x03:
            if((mbase_cb->CoverOpenS_status != EVENT_SET) && (*(p_rx+1) == 0x55) && (mbase_cb->key_state == false))  return RS_ERR_PASS;    //开盖或公钥下允许进入
            
			if(*(p_rx+1) == 0x55)
                mbase_cb->factor_mode = true;
            else if(*(p_rx+1) == 0xAA)
                mbase_cb->factor_mode = false;
            else
                Err_Byte = RS_ERR_OTHERS;
            if(Err_Byte == 0)
            {
                s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                            offsetof(struct mbase_control_block_t, factor_mode));
                nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->factor_mode, 1);//存储至EE
            }
            break;
            
        case 0x04://参数初始化
            ret = para_init( instance );
            meter_clr(instance);
            if(ret < 0) 
                Err_Byte = RS_ERR_OTHERS;
            break;
            
        default:
            Err_Byte = RS_ERR_OTHERS;
            break;
	}
    
    if(Err_Byte == 0)
    {
        *len = 0;
        return 0;
    }
    else
    {
        *p_tx = Err_Byte;
        *len = 1;
        return 1;
    }
}