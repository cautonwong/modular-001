#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "targets/stategrid_meter/app/resource_ids.h"
#include "resource_ids.h"
#include "targets/stategrid_meter/persistence/meter_base_view.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "645id_table.h"
#include "cpu_surport.h"
#include "application/adapters/dl645/ctl_other.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "application/adapters/dl645/fr_645_env.h"


/***********************************************************************/
/*  函数名称：Comm_Other                                               */
/*  函数功能：扩展命令                                              */
/*  输出参数：读结果：0为正确,其余为错误                              */
/***********************************************************************/
signed char Comm_Other(void *instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len)
{
    const nd_resource_port_t *port = nd_fr645_resource_access(instance);
    signed char ret=0;
	unsigned char Err_Byte=0, rx_len=0;
    uint32_t s_index=0;
    int16_t err=0;
    if(!nd_mb_valid(instance))
    {
        *p_tx = RS_ERR_OTHERS;
        return 1;
    }
    
    err = Check_para_Pro(instance);

	rx_len = *(p_rx-1);
	
    switch(rx_len)
	{
		case 0x03:
            if((nd_mb_cover_open_s_status(instance) != EVENT_SET) && (*(p_rx+1) == 0x55) && (nd_mb_key_state(instance) == false))  return RS_ERR_PASS;    //开盖或公钥下允许进入
            
			if(*(p_rx+1) == 0x55)
                nd_mb_factor_mode(instance) = true;
            else if(*(p_rx+1) == 0xAA)
                nd_mb_factor_mode(instance) = false;
            else
                Err_Byte = RS_ERR_OTHERS;
            if(Err_Byte == 0)
            {
                s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                            nd_mb_offset_factor_mode());
                nd_set_offset_port(port, s_index, (uint8_t*)&nd_mb_factor_mode(instance), 1);//存储至EE
            }
            break;
            
        case 0x04://参数初始化
            ret = para_init( instance );
            if((ret < 0) || (meter_clr(instance) == 0))
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
