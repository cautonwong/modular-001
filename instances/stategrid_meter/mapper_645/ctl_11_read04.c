#include "nidavellir.h"
#include "645id_table.h"

extern uint8_t dl645rd_chg_data(struct instance_t* instance, uint32_t addr, uint8_t format, uint8_t *pbuf, uint8_t *len);

typedef struct
{
    unsigned short  fsid; //显示项ID
    unsigned long   id645; //oad
    unsigned char   sequ;//序号
} DISPTAB;

const  DISPTAB Display645Tab[] =
{
    //当前时间
    0xA001,     0x04000101,     0x00,   //日期
    0xA002,     0x04000102,     0x00,   //时间
    0xA003,     0x02010100,     0x00,   //电压
    0xA006,     0x02020100,     0x00,   //火线电流
    0xA009,     0x02800001,     0x00,   //零线电流
    0xA00A,     0x02030000,     0x00,   //A相功率
    0xA00B,     0x02030100,     0x00,   //A相功率
    0xA012,     0x02060000,     0x00,   //A相功率因数
    0xA013,     0x02060100,     0x00,   //A相功率因数
    
    0xB001,		0x04000401,     0x00,   //表地址高4位
    0xB002,		0x04000401,     0x01,   //表地址低8位
    0xB003,		0x04000402,     0x00,   //表号高4位
    0xB004,		0x04000402,     0x01,   //表号低8位
    0xB005,		0x0400040E,     0x00,   //客户编号高4位
    0xB006,		0x0400040E,     0x01,   //客户编号低8位
    0xB007,		0x04000409,     0x00,   //电表有功常数  
#if 0    
    //正向电能量
    //0xC0FF,     0xC001~0xC00D:总，费率1~12;   0xC000~0xC180:当前~上24月正向总电能
    //反向电能量
    //0xD0FF,     0xD001~0xD00D:总，费率1~12;   0xD000~0xD180:当前~上24月反向总电能  
    //组合电能量
    //0xE0FF,     0xE001~0xE00D:总，费率1~12;   0xE000~0xE180:当前~上24月组合总电能  
#endif
};


void displayid_to_645id( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    uint8_t IDL1, IDL0, maxnum, i, id[3]={0,};
    uint32_t addr=0, index, id_645=0;
    uint16_t displayid=0;
    
    IDL1 = (uint8_t)((ID & 0x0000FF00) >> 8);
    IDL0 = (uint8_t)(ID & 0x000000FF);
    
    maxnum = sizeof(Display645Tab)/sizeof(DISPTAB);
    
    if(IDL1 == 0x01)    
        addr = AutoDisplayList + IDL0 - 1;
    else if(IDL1 == 0x02)
        addr = KeyDisplayList + IDL0 - 1;
    
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_get_index(instance, index, (uint8_t *)&displayid, 2);
    
    for(i=0; i<maxnum; i++)
    {
        if(displayid == Display645Tab[i].fsid)
        {
            memcpy(p_tx, (uint8_t*)&Display645Tab[i].id645, 4);
            *(p_tx+4) = Display645Tab[i].sequ;
            return;
        }
    }
    
    id[2] = (displayid & 0xF000) >> 8;
    id[1] = (displayid & 0x0FF0) >> 4;
    id[0] = displayid & 0x000F;
    switch(id[2])
    {
        case 0xC0:  //正向
            id_645 = 0x00010000 + ((uint32_t)(id[0]-1) << 8) + id[1];
            break;
        case 0xD0:  //反向
            id_645 = 0x00020000 + ((uint32_t)(id[0]-1) << 8) + id[1];
            break;
        case 0xE0:  //组合
            id_645 = 0x00000000 + ((uint32_t)(id[0]-1) << 8) + id[1];
            break;
    }
    memcpy(p_tx, (uint8_t*)&id_645, 4);
    *(p_tx+4) = 0;
}

uint8_t get_645_Seasonday_table( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    uint32_t addr=0, s_index;
    uint8_t season_num=0, day_num=0, temp[42]={0,}, i, IDL0=0;
    uint16_t IDH = 0;
    
    IDH = (ID & 0xFFFF0000) >> 16;
    IDL0 = ID & 0x000000FF;
    
    if(IDL0 == 0)
    {
        if(ID == 0x04010000)    //第一套时区
            addr = Season_Table0;
        else if(ID == 0x04020000)   //第二套时区
            addr = Season_Table1;
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);
        nd_get_index(instance, s_index, &season_num, 1);
        if(season_num > MaxSeasonNum) season_num = MaxSeasonNum;

        for(i=0; i<season_num; i++)
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + i);
            nd_get_index(instance, s_index, temp, 3);
            *p_tx++ = BYTE_BCD(temp[2]);
            *p_tx++ = BYTE_BCD(temp[1]);
            *p_tx++ = BYTE_BCD(temp[0]);
        }
        return season_num*3;
    }
    else
    {
        if(IDH == 0x0401)   //第一套时段
            addr = Day_Table00 + IDL0 - 1;
        else if(IDH == 0x0402)   //第二套时段
            addr = Day_Table10 + IDL0 - 1;
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para+2);
        nd_get_index(instance, s_index, &day_num, 1);
        if(day_num > MaxTouNum) day_num = MaxTouNum;
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
        nd_get_index(instance, s_index, temp, 42);
        
        for(i=0; i<day_num; i++)
        {
            *p_tx++ = BYTE_BCD(temp[2 + 3*i]);
            *p_tx++ = BYTE_BCD(temp[1 + 3*i]);
            *p_tx++ = BYTE_BCD(temp[0 + 3*i]);
        }
        return day_num*3;
    }
}

int8_t get_645_holiyday_table( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    uint32_t s_index;
    uint8_t IDL0=0, temp[6] = {0,};
    uint16_t year=0;
    
    IDL0 = ID & 0x000000FF;
    if(IDL0 > MaxHolidayNum)    return -1;
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + HoliDay_Table + IDL0 - 1);
    nd_get_index(instance, s_index, temp, 6);
    *p_tx++ = BYTE_BCD(temp[5]);
    *p_tx++ = BYTE_BCD(temp[3]);
    *p_tx++ = BYTE_BCD(temp[2]);
    mem_swap_cpy((uint8_t*)&year, temp, 2);
    if(year == 0xffff)  
    {
        *p_tx++ = 0;
    }
    else
    {
        year = year - 2000;
        *p_tx++ = BYTE_BCD((uint8_t)year);
    }
    return 4;
}

int read_04( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    uint8_t MemType,buf[200] = {0,}, Ptr,len=0;
    unsigned short Format=0;
    unsigned long Addr,Info,addr1,IDx=0;
    
    IDx = ID & 0xFFFFFF00;
    
    switch(IDx) //显示、时区、时段、公共假日参数特殊处理
    {
        case 0x04040100:    //自动循环显示
        case 0x04040200:    //按键循环显示
            displayid_to_645id(instance, ID, p_rx, p_tx);
            return 5;
            break;
        case 0x04010000:    //第一套时区、时段表数据
        case 0x04020000:    //第二套时区、时段表数据
            len = get_645_Seasonday_table(instance, ID, p_rx, p_tx);
            return len;
            break;
        case 0x04030000:    //公共假日表
            return get_645_holiyday_table(instance, ID, p_rx, p_tx);
            break;
    }
    
    Ptr = GetCommTable( ID );//查表
	if( Ptr == 0xFF )
		return -1;
	Addr = CommTab[Ptr].Addr;
    Format = CommTab[Ptr].format;
	Info = CommTab[Ptr].Info;

	if( (Info & CommData_R) == 0 ) //数据不可读
		return -1;
		
	len = (unsigned char)(Info & CommDataLen);
	MemType = (unsigned char)((Info>>6) & MEMVALUE);
    
    if(Format != FORMAT_00)
    {
        if(ID == 0xA800020C)    //读计量芯片
        {
            Addr = *(p_rx+4);
        }
        if( dl645rd_chg_data(instance, Addr, Format, p_tx, &len) == 0)
            return len;
    }
    
	if(MemType == Mem_E2PROM)  
	{
		addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Addr);
		nd_get_index(instance, addr1, buf, len);
        mem_swap_cpy(p_tx, buf, len);
	}
	else if(MemType == Mem_E2PROM_BP)  
	{
		addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Addr);
		nd_get_index(instance, addr1, p_tx, len);        
	}
	else
	{
		memcpy( p_tx, (unsigned char*)Addr, len );
	}
    
    return len;
}
