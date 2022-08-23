#include "nidavellir.h"
#include "645id_table.h"
#include "ndvh_rn8209.h"

extern const struct dev_info_t rn8209_dev;
extern int rn8209c_get( struct instance_t* instance, uint8_t *p_rx, uint8_t* p_tx);
extern unsigned char IxRMSOS_Adjust(struct instance_t* instance, unsigned char CH);
extern unsigned char ft_adj_meter(struct instance_t* instance, uint8_t *p_rx);

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
	unsigned long StAddr;
    unsigned char len;
    uint8_t tmpbuf[10] = {0};
    uint32_t s_index,temp=0;

	switch( ID )
	{   
        case 0x02010100:    //电压
            WORD_BCD2(p_tx, (uint16_t)mbase_cb->voltage_ac);
            return 2;
        case 0x0201FF00:    //电压数据块
            WORD_BCD2(p_tx, (uint16_t)mbase_cb->voltage_ac);
            memset(p_tx+2, 0xFF, 4);
            return 6;
            
        case 0x02020100:    //电流
        case 0x0202FF00:    //电流数据块
            if(mbase_cb->current_l < 0)
            {
                temp = ~mbase_cb->current_l + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, mbase_cb->current_l);
            }
            if(ID == 0x0202FF00)
            {
                memset(p_tx+3, 0xFF, 6);
                return 9;
            }
            else
                return 3;
            break;
            
        case 0x02030000:    //总有功功率 
        case 0x02030100:   
        case 0x0203FF00:    //有功功率数据块
            if(mbase_cb->active_power < 0)
            {
                temp = ~mbase_cb->active_power + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, mbase_cb->active_power);
            }
            if(ID == 0x0203FF00)
            {
                memcpy(p_tx+3, p_tx, 3);
                memset(p_tx+6, 0xFF, 6);
                return 12;
            }
            else
                return 3;
            break;
            
        case 0x02050000:    //总视在功率 
        case 0x02050100:   
        case 0x0205FF00:    //视在功率数据块
            HEX_BCD(p_tx, mbase_cb->apparent_power);
            if(ID == 0x0205FF00)
            {
                memcpy(p_tx+3, p_tx, 3);
                memset(p_tx+6, 0xFF, 6);
                return 12;
            }
            else
                return 3;
            break; 
            
        case 0x02060000:    //功率因数
        case 0x02060100:   
        case 0x0206FF00:    //功率因数数据块
            if(mbase_cb->power_factor < 0)
            {
                temp = ~mbase_cb->power_factor + 1;
                WORD_BCD2(p_tx, temp);
                *(p_tx+1) |= 0x80;
            }
            else
            {
                WORD_BCD2(p_tx, mbase_cb->power_factor);
            }
            if(ID == 0x0206FF00)
            {
                memcpy(p_tx+2, p_tx, 2);
                memset(p_tx+4, 0xFF, 4);
                return 8;
            }
            else
                return 2;
            break;
        case 0x02800001:    //零线电流
            if(mbase_cb->current_n < 0)
            {
                temp = ~mbase_cb->current_n + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, mbase_cb->current_n);
            }
            return 3;
        case 0x02800002:  //频率
            HEX_BCD(p_tx, mbase_cb->frequency);
            return 2;
            
            
        case 0xA8000100:
            if(mbase_cb->factor_mode == true)
                *p_tx = 0xaa;
            else
                *p_tx = 0;
            return 1;
		case 0xA601100F:
			StAddr = SoftwareCHK();
			memcpy( (unsigned char*)p_tx, (unsigned char*)&StAddr, 4 );
			return 4;
			break;
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
 		case 0xA8F00051://掉电主动上报的阈值时间(秒)hex  0x0E10
            memcpy( (unsigned char*)p_tx, (uint8_t*)&mbase_cb->active_report_sec_limit, 2 );        
            return 2; 
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
signed char Comm_Read(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len)
{
	unsigned char Ptr,MemType;
    int Err_Byte;
	unsigned long Addr,Info;
	unsigned long ID,addr1;
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
    

	Err_Byte = Direct_Read( instance, ID, p_rx, p_tx);//根据ID,直接读取数据
	if( Err_Byte != -1)
    {
        if(0xA800020C == ID)    //计量万能读
        {
            *len = 9;
        }
        else
        {
            *len = Err_Byte + 4;
        }
        return 0;
    }
    

	Ptr = GetCommTable( ID );//查表
	if( Ptr == 0xFF )
		return -1;
	Addr = CommTab[Ptr].Addr;
	Info = CommTab[Ptr].Info;

	if( (Info & CommData_R) == 0 ) //数据不可读
		return -1;
		
	*len = (unsigned char)(Info & CommDataLen);
	MemType = (unsigned char)((Info>>6) & MEMVALUE);
	if(MemType == Mem_E2PROM)  
	{
		addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Addr);
		nd_get_index(instance, addr1, p_tx, *len);
	}
	else if(MemType == Mem_E2PROM_BP)  
	{
		addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Addr);
		nd_get_index(instance, addr1, p_tx, *len);        
	}
	else
	{
		memcpy( p_tx, (unsigned char*)Addr, *len );
	}
    *len = *len+4;
    
    return 0;
}