#include "nidavellir.h"
#include "645id_table.h"
#include "ndvh_rn8209.h"
    
extern const struct dev_info_t rn8209_dev;
extern unsigned char rn8209c_get( struct instance_t* instance, uint8_t *p_rx, uint8_t* p_tx);
extern unsigned char IxRMSOS_Adjust(struct instance_t* instance, unsigned char CH);
extern unsigned char ft_adj_meter(struct instance_t* instance, uint8_t *p_rx);
extern int read_05( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx, uint8_t *tx_len);
/***********************************************************************/
/*  函数名称：GetCommTable                                             */
/*  函数功能：根据给定的ID,找出其在CommTab里的位置                    */
/*  输入参数：ID:通讯ID                                                */
/*  输出参数：如给定ID正确,则返回其在CommTab里的位置；                */
/*            如错误,则返回0xFF。                                     */
/***********************************************************************/
unsigned char GetCommTable( unsigned long ID )
{
	unsigned char i;

	for( i=0;i<CommTabNum;i++ )
	{
		if( ID == CommTab[i].ID )
			return i;
	}
	return 0xFF;
}

/***********************************************************************/
/*  函数名称：SoftwareCHK                                              */
/*  函数功能：CODE CHECK                                               */
/*  输出参数： 返回累加和                                              */
/***********************************************************************/
unsigned long SoftwareCHK(void)
{
	unsigned long FFFFCHK=0x00;
	unsigned long i;
	
	for (i=0; i<0x1FFFF; i++)
	{
		FFFFCHK += *((const unsigned char  *)(i));
		v85xxp_clear_watchdog();//清看门狗
	}
	return FFFFCHK;

}
/***********************************************************************/
/*  函数名称：Direct_Read                                          		*/
/*  函数功能：特殊数据,直接读取                                       */
/*  输入参数：ID:通讯ID                                                */
/*  输出参数：1为特殊数据,其它为非特殊数据                            */
/***********************************************************************/
int Direct_Read( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    unsigned char len;
    uint8_t tmpbuf[10] = {0};
    uint32_t s_index,temp=0;

	switch( ID )
	{   
        case 0xA800020C://微调与补偿
            len = rn8209c_get(instance, p_rx, p_tx);
            return len;
            break;
  		case 0xA800020F://复位次数
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ResetCount);
            nd_get_index(instance, s_index, tmpbuf, 4);   
            memcpy( (unsigned char*)p_tx, tmpbuf, 4 );
            return 4;
            break;                 
        case 0xA8F00053://模拟停电时间（补冻结用）
            (*(p_tx+5)) = BYTE_BCD(mbase_cb->v_powerdown_tm.tm_year + 1900 - 2000);
            (*(p_tx+4)) = BYTE_BCD(mbase_cb->v_powerdown_tm.tm_mon + 1);
            (*(p_tx+3)) = BYTE_BCD(mbase_cb->v_powerdown_tm.tm_mday);
            (*(p_tx+2)) = BYTE_BCD(mbase_cb->v_powerdown_tm.tm_hour);
            (*(p_tx+1)) = BYTE_BCD(mbase_cb->v_powerdown_tm.tm_min);
            (*(p_tx+0)) = BYTE_BCD(mbase_cb->v_powerdown_tm.tm_sec);
            return 6;
            break;  

		default:
			return -1;
			break;
	}
}


/***********************************************************************/
/*  函数名称：Comm_Read                                                */
/*  函数功能：通讯读                                                   */
/*  输出参数：读结果：0为正确,其余为错误                              */
/***********************************************************************/
signed char Comm_Read(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len, struct context_645_t* ctx)
{
    unsigned char IDH1=0, IDL0 = 0;
    int Err_Byte = 0;
	unsigned long ID;
    struct mbase_control_block_t* mbase_cb;
	
	mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    if((*(p_rx-1) == 0x07) || (*(p_rx-1) == 0x11))   //扩展命令——校表
    {
        Err_Byte = ft_adj_meter(instance, p_rx);
        if(Err_Byte != 0)
        {
            *p_tx = Err_Byte;
            return 1;
        }
        else
            return 0;
    }

    memcpy(p_tx, p_rx, 4);
    p_tx = p_tx+4;
	memcpy((unsigned char*)&ID, p_rx, 4);
    IDH1 = (ID & 0xFF000000) >> 24;
    IDL0 = (uint8_t)(ID & 0x000000FF);


    if(false == Search_DL645_Prepay_IDTab(instance, ID, 0x11))
    {
        *len = 1;
        return RS_ERR_ID;
    }

    if (false == Dl645_IR_chl_Check(instance, ID, ctx->channel))
    {
        *len = 1;
        return RS_ERR_PASS;        
    }
    
    switch(IDH1)
    {
        case 0x00:  //电能
            if (IDL0 == 0)
            {
                Err_Byte = read_00(instance, ID, p_rx, p_tx);
            }
            else  //历史电能
            {
                Err_Byte = read_05(instance, ID, p_rx, p_tx, len);
                return (int8_t)Err_Byte;
            }
            break;
            
        case 0x02:  //瞬时量
            Err_Byte = read_02(instance, ID, p_rx, p_tx);
            break;
            
            
        case 0x04:  //参数
        case 0xA8:
        case 0xA6:
            Err_Byte = read_04(instance, ID, p_rx, p_tx);
            break;

        case 0x03:
        case 0x1D: 
        case 0x1E: 
        case 0x19:  //过流           
        case 0x05:  //冻结
            Err_Byte = read_05(instance, ID, p_rx, p_tx, len);
            return (int8_t)Err_Byte;

            break;    
        case 0x06:  //负荷
            Err_Byte = read_06(instance, ID, p_rx, p_tx, len, ctx);
            return (int8_t)Err_Byte;
            break;            
        default:
            Err_Byte = -RS_ERR_ID;
            *len = 1;
            return RS_ERR_ID;
            break;
    }
    if( Err_Byte < 0) //异常应答
    {
        *len = 1; 
        return 1;
    }
    
    if(0xA800020C == ID)    //计量万能读
        *len = 9;
    else
        *len = Err_Byte + 4;
    
    return 0;
    
}
