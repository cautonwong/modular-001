#include "nidavellir.h"
#include "645id_table.h"

extern void  v85xxp_clear_watchdog(void);
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
	unsigned long FFFFCHK;
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
int Direct_Read( unsigned long ID , uint8_t* p_tx)
{
	unsigned long StAddr;

	switch( ID )
	{
		case 0xA8000104:
			StAddr = SoftwareCHK();
			memcpy( (unsigned char*)&p_tx, (unsigned char*)&StAddr, 4 );
			return 4;
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
unsigned char Comm_Read(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t len)
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

	memcpy((unsigned char*)&ID, p_rx, 4);

	Err_Byte = Direct_Read( ID , p_tx);//根据ID,直接读取数据
	if( Err_Byte != -1)
    {
        len = Err_Byte;
        return 0;
    }
    

	Ptr = GetCommTable( ID );//查表
	if( Ptr == 0xFF )
		return 1;
	Addr = CommTab[Ptr].Addr;
	Info = CommTab[Ptr].Info;

	if( (Info & CommData_R) == 0 ) //数据不可读
		return 1;
		
	len = (unsigned char)(Info & CommDataLen);
	MemType = (unsigned char)((Info>>6) & MEMVALUE);
	if(MemType == Mem_E2PROM)
	{
		addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Addr);
		nd_get_index(instance, addr1, p_tx, len);
	}
	else 
	{
		memcpy( p_tx, (unsigned char*)Addr, len );
	}

    return 0;
}