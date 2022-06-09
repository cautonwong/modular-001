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

void meteric_para_clr(struct instance_t* instance, uint8_t *p_rx)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t s_index=0;
    uint16_t pstart=0;
    uint8_t buf[40]={0,};
    
    memset(buf, 0x00, 8);
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Pgain0);    //火线增益、相位
    nd_set_index(instance, s_index, buf, 8); 
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Pgain1);    //零线增益、相位
    nd_set_index(instance, s_index, buf, 8); 
    
    pstart = 0x0049;
    memcpy(buf, (uint8_t*)&pstart, 2);
    if(p_rx==0)
    {
        buf[2] = 0x6F;
        buf[3] = 0x09;
    }
    else
        memcpy((uint8_t*)&buf[2], p_rx+3, 2);//写入脉冲速率
        
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Other);
    nd_set_index(instance, s_index, buf, 4); 
    
    memset((uint8_t*)&mbase_control_block->k_u, 0x00, 16);
    mbase_control_block->k_u = 0.0137414116;
    mbase_control_block->k_il = 0.016;
    mbase_control_block->k_p = 0.0023294272;
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_K);
    nd_set_index(instance, s_index, (uint8_t*)&mbase_control_block->k_u, 16);//存储至EE
}


unsigned char adj_meter(struct instance_t* instance, uint8_t *p_rx)
{
    uint32_t id=0;
    
    
    switch(*(p_rx-1))
    {
        case 0x07://校表初始化
            memcpy((uint8_t*)&id, p_rx, 3);
            if(id == 0x02FC01)
            {
                meteric_para_clr(instance, p_rx);
            }
            break;
        case 0x11://校表
            break;
        default:
            break;
    }
    return 0;
    
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
    
    if(*(p_rx-1) != 0x04)   //扩展命令——校表
    {
        Err_Byte = adj_meter(instance, p_rx);
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
    

	Err_Byte = Direct_Read( ID , p_tx);//根据ID,直接读取数据
	if( Err_Byte != -1)
    {
        *p_tx = Err_Byte;
        return 1;
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
	else 
	{
		memcpy( p_tx, (unsigned char*)Addr, *len );
	}
    *len = *len+4;
    
    return 0;
}