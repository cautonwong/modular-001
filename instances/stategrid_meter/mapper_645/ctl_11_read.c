#include "nidavellir.h"
#include "645id_table.h"

extern const struct dev_info_t rn8209_dev;

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
typedef struct 
{
    uint8_t address;
    uint8_t len;
}meterchip;
const  meterchip  Mtable[8]=
{
    0x20,   2,  //快速有功脉冲计数器
    0x22,   3,  //通道A电流的有效值
    0x23,   3,  //通道B电流的有效值
    0x24,   3,  //电压有效值
    0x26,   4,  //有功功率A
    0x27,   4,  //有功功率B
    0x28,   4,  //无功功率
    0x2d,   3,  //计量状态及校验和寄存器
};


int rn8209c_get( struct instance_t* instance, uint8_t *p_rx, uint8_t* p_tx)
{
    uint8_t addr=0,lenth=0,i;
    int16_t tmp[15]={0,};
    uint32_t s_index=0,data=0;
    //uint8_t* buf = p_tx;
    
    addr = *(p_rx+4);
    *p_tx = addr;
    memset( p_tx+1,0x00,4);
    if(addr >= 0xA0)    //自热影响量 0.5L
    {
        if(addr >= 0xAF)	return false;
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_05L);
        nd_get_index(instance, s_index, (uint8_t*)&tmp[0], 30); //读取EE中的补偿值
        memcpy( (p_tx+1), (uint8_t*)&tmp[addr-0x90], 2);
        return 2;
    }
    else if(addr >= 0x90)//自热影响量 1.0L
    {
        if(addr >= 0x9F)	return false;

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_10L);
        nd_get_index(instance, s_index, (uint8_t*)&tmp[0], 30); //读取EE中的补偿值
        memcpy( (p_tx+1), (uint8_t*)&tmp[addr-0x90], 2);
        return 2;
    }
    else if(addr >= 0x80)//电压影响量
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
        nd_get_index(instance, s_index, (uint8_t *)&tmp[0], 8); //读取EE中的补偿值
        switch(addr)
        {
            case 0x80://80%
                memcpy( p_tx+1, (uint8_t*)&tmp[0], 2);
                break;
            case 0x81://90%
                memcpy( p_tx+1, (uint8_t*)&tmp[1], 2);
                break;
            case 0x82://110%
                memcpy( p_tx+1, (uint8_t*)&tmp[2], 2);
                break;
            case 0x83://115%
                memcpy( p_tx+1, (uint8_t*)&tmp[3], 2);
                break;
            default:
                return false;
                break;
        }
        return 2;
    }
    else if(addr <= 0x17)
    {
        if((addr==0x07)||(addr == 0x08))
            lenth = 1;
        else
            lenth = 2;
        rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , addr , lenth , &data);
        
        if(addr == 0x17)
        {
            data = data>>8;
        }
        memcpy( p_tx+1, (uint8_t*)&data, lenth);
        
        return lenth;
    }
    else
    {
        for(i=0; i<8; i++)
        {
            if(addr == Mtable[i].address)
            {
                rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , addr , Mtable[i].len , &data);
                memcpy( p_tx+1, (uint8_t*)&data, lenth);
                return lenth;
            }
        }
    }
    return -1;
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
    uint32_t s_index;

	switch( ID )
	{
		case 0xA8000104:
			StAddr = SoftwareCHK();
			memcpy( (unsigned char*)&p_tx, (unsigned char*)&StAddr, 4 );
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
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);    //火线增益、相位
    nd_set_index(instance, s_index, buf, 8); 
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain1);    //零线增益、相位
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
        
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Other);
    nd_set_index(instance, s_index, buf, 4); 
    
    memset((uint8_t*)&mbase_control_block->k_u, 0x00, 16);
    mbase_control_block->k_u = 0.0137414116;
    mbase_control_block->k_il = 0.016;
    mbase_control_block->k_p = 0.0023294272;
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
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
    
    if((*(p_rx-1) != 0x04) && (*(p_rx-1) != 0x05))   //扩展命令——校表
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
    

	Err_Byte = Direct_Read( instance, ID, p_rx, p_tx);//根据ID,直接读取数据
	if( Err_Byte != -1)
    {
        *len = 9;
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