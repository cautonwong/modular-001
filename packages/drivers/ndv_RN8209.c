#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
#include "ndvh_rn8209.h"



unsigned char ReadRn8209WithChk(unsigned char ChipAddr, unsigned char* Buf, unsigned char length, const struct dev_info_t* dev)
{

    unsigned char i, j;
    unsigned char rx_buffer[6] = { 0, };
    unsigned char check_sum;
    unsigned long timeout = 8000;
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;
    uint8_t index = bus_ops->bus_index;
      
    for (j = 0; j < 3; j++)
    {
        check_sum = ChipAddr;
        bus_ops->tx_enable(2 , index);
        mcu_delay_us(500);
        bus_ops->tx(ChipAddr , index);

        while ((false == bus_ops->tx_statue(index)) && (timeout > 1))
        {
            timeout--;
        }
        bus_ops->rx_enable(2 , index);
        bus_ops->tx_enable(0 , index);
        for (i = 0; i < (length + 1); i++)
        {
            timeout = 5000;
            while ((false == bus_ops->rx_statue(index)) && (timeout > 1))
            {
                timeout--;
            }
            if (timeout <= 1)
            {
                timeout = 5000;
            }
            if (true == bus_ops->rx(&rx_buffer[i] , index))
            {
                if (i < (length))
                {
                    check_sum += rx_buffer[i];
                }
                else
                {
                    check_sum = ~check_sum;
                    if (check_sum == rx_buffer[i])
                    {
                        mem_swap_cpy(Buf, (unsigned char*)(rx_buffer), length);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}



unsigned char WriteRn8209(unsigned char ChipAddr, unsigned char* Buf, unsigned char length , const struct dev_info_t* dev)
{
    unsigned char i, j;
    unsigned char tx_buffer[6] = { 0 };
    unsigned char check_sum = 0;
    unsigned short timeout;
    const struct bus_ops_t *dev_bus_ops = dev->dev_bus_ops;
    uint8_t index = dev_bus_ops->bus_index;

    tx_buffer[0] = ChipAddr | 0x80;
    mem_swap_cpy(&tx_buffer[1], Buf, length);

    for (i = 0; i < (length + 1); i++)
    {
        check_sum += tx_buffer[i];
    }
    tx_buffer[length + 1] = ~check_sum;
    dev_bus_ops->tx_enable(2 , index);
    for (j = 0; j < 3; j++)
    {
        for (i = 0; i < length + 2; i++)
        {
            timeout = 2000;
            dev_bus_ops->tx(tx_buffer[i] , index);
            while ((false == dev_bus_ops->tx_statue(index)) && (timeout > 1))
            {
                timeout--;
            }
        }
        dev_bus_ops->tx_enable(0 , index);
        return true;
    }
    return false;
}

unsigned char WriteRn8209Check(unsigned char ChipAddr, unsigned char* Buf, unsigned char length , const struct dev_info_t* dev)
{
    unsigned char i;
    unsigned char buffer[12];
    unsigned char scr[12];


    memcpy(scr, Buf, length);

    for (i = 0; i < 10; i++)
    {

        if (true == WriteRn8209(ChipAddr, Buf, length , dev))
        {
            if (true == ReadRn8209WithChk(ChipAddr, buffer, length , dev))
            {
                if (0 == mem_compare(scr, buffer, length))
                {
                    return true;
                }
            }
        }
        mcu_delay_us(5000);
    }
    return false;
}

unsigned char CmdRn8209(unsigned char cmd  , const struct dev_info_t* dev)
{
    unsigned long temp32 = cmd;

    return WriteRn8209(0xEA, (unsigned char*)&temp32, 1 , dev);
}



unsigned char Rn8209InitConfig(const struct dev_info_t* dev)
{
    //RN8209初始化配置的参数，直接在这里修改配置。
    unsigned short SysCon = 0x1643;
    unsigned short EmuCon = 0x0001;
    unsigned short Hfconst = 0x064a;
    unsigned short Pstart = 0x0049;
    unsigned char regIE = 0x01;
    unsigned long s_index=0;
    unsigned char buf[4]={0,};
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Other);//写入脉冲速率
    //nd_get_index(instance, s_index, buf, 4);
    memcpy((unsigned char*)&Pstart, buf, 2);
    memcpy((unsigned char*)&Hfconst, buf+2, 2);
    
    if((Pstart == 0) || (Pstart == 0xFFFF))   Pstart = 0x0049;
    if((Hfconst == 0) || (Hfconst == 0xFFFF))   Hfconst = 0x064a;
    mcu_delay_us(5000);
    CmdRn8209(CMD_WR_EN , dev);
    mcu_delay_us(5000);

    if (false == WriteRn8209Check(SYSCON, (unsigned char*)&SysCon, 2 , dev))
    {
        return false;
    }
    if (false == WriteRn8209Check(EMUCON, (unsigned char*)&EmuCon, 2 , dev))
    {
        return false;
    }
    if (false == WriteRn8209Check(HFCONST, (unsigned char*)&Hfconst, 2 , dev))
    {
        return false;
    }
    if (false == WriteRn8209Check(PSTART, (unsigned char*)&Pstart, 2 , dev))
    {
        return false;
    }
    if (false == WriteRn8209Check(IE, (unsigned char*)&regIE, 1 , dev))
    {
        return false;
    }
    mcu_delay_us(5000);
    CmdRn8209(CMD_WR_DIS , dev);
    mcu_delay_us(5000);
    
    return true;
}


void ResetRn8209(void)
{
    PIN_RN8209_RX_OUT_MODE();
    PIN_RN8209_RX_OUT_HIGH();
    mcu_clear_watchdog();
    mcu_delay_us(30000);
    PIN_RN8209_RX_OUT_LOW();
    mcu_clear_watchdog();
    mcu_delay_us(30000);
    PIN_RN8209_RX_OUT_HIGH();
    mcu_delay_us(10000);
    PIN_RN8209_RX_IN_MODE();
    mcu_clear_watchdog();
    mcu_delay_us(50000);
    mcu_clear_watchdog();
}

unsigned char Rn8209DeviceIDCheck(const struct dev_info_t* dev)
{
    unsigned long Rn8209DevID = 0;

    if (true == ReadRn8209WithChk(DEVICEID, (unsigned char*)&Rn8209DevID, 3 , dev))
    {
        if (Rn8209DevID == 0x00820900)
        {
            return true;
        }
    }
    return false;
}




unsigned char Rn8209AdjParaSet(struct Rn8209AdjSet_t* Set , const struct dev_info_t* dev)
{

    CmdRn8209(CMD_WR_EN , dev );
    mcu_delay_us(5000);
    mcu_delay_us(5000);

    if (false == WriteRn8209Check(GPQA, (unsigned char*)&Set->Gpq, 2 , dev ))
    {
        return false;
    }
    if (false == WriteRn8209Check(PHSA, (unsigned char*)&Set->Phs, 1, dev))
    {
        return false;
    }
    if (false == WriteRn8209Check(APOSA, (unsigned char*)&Set->Apos, 2, dev))
    {
        return false;
    }
    if (false == WriteRn8209Check(IARMSOS, (unsigned char*)&Set->Irmsos, 2, dev))
    {
        return false;
    }

    CmdRn8209(CMD_WR_DIS, dev);

    return true;
}

int Rn8209_initialize(const struct dev_info_t* dev)
{
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;

    //return bus_ops->init((size_t)dev->cfg);
    return 0;
}

int Rn8209_control(const struct dev_info_t* dev , uint8_t ctrl , void* ctrl_parameter)
{
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;
    
    struct Rn8209AdjSet_t *set_test= (struct Rn8209AdjSet_t*)ctrl_parameter;

    
    switch (ctrl)
    {
        case AC_POWER_ON:
            ResetRn8209();
            PIN_RN8209_TX_OUT_MODE();
            PIN_RN8209_TX_OUT_HIGH();
            
            bus_ops->init((size_t)dev->cfg);
            if (true == Rn8209DeviceIDCheck(dev))
            {
                Rn8209InitConfig(dev);
            }
            
            Rn8209AdjParaSet(set_test , dev);
            break;
        case AC_POWER_OFF:
            ResetRn8209();
            break;
        default:
            break;

    }
    
    return 0;
}

int Rn8209_read(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void *src)
{
    uint8_t lenth=0;
    int16_t data=0,tmp[15]={0};
    uint32_t s_index=0;
    uint8_t* buf = src;
/*        
    if(addr == 0x1091)//万能读
    {
        if(*buf >= 0xA0)////频率影响量
        {
            ;;//暂时不加
        }
        else if(*buf >= 0x90)//自热影响量
        {
            if(*buf >= 0x9F)	return false;

            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Heat_Value_10L);
            nd_get_index(instance, s_index, tmp, 30); //读取EE中的补偿值
            memset((buf+1),0x00,4);
            memcpy((buf+1), (uint8_t*)&tmp[*buf-0x90], 2);
            return true;
        }
        else if(*buf >= 0x80)//电压影响量
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + U80_Value);
            nd_get_index(instance, s_index, (uint8_t *)&tmp[0], 8); //读取EE中的补偿值
            memset((buf+1),0x00,4);
            switch(*buf)
            {
                case 0x80://80%
                    memcpy((buf+1), (uint8_t*)&tmp[0], 2);
                    break;
                case 0x81://90%
                    memcpy((buf+1), (uint8_t*)&tmp[1], 2);
                    break;
                case 0x82://110%
                    memcpy((buf+1), (uint8_t*)&tmp[2], 2);
                    break;
                case 0x83://115%
                    memcpy((buf+1), (uint8_t*)&tmp[3], 2);
                    break;
                default:
                    return false;
                    break;
            }
            return true;
            
        }
        else if(*buf <= 0x17)
        {
            if((*buf==0x07)||(*buf == 0x08))
                lenth = 1;
            else
                lenth = 2;
            if(true == ReadRn8209WithChk(addr, (uint8_t*)&data , lenth ,dev ))
            {
                memset((buf+1),0x00,4);
                if(*buf == 0x17)
                {
                    data = data>>8;
                }
                memcpy((buf+1), (uint8_t*)&data, lenth);
            }
            return true;
        }
        else if((*buf == 0x20) || (*buf == 0x26))
        {
            if(*buf == 0x20)
                lenth = 2;
            else if(*buf == 0x26)
                lenth = 4;
            if(true == ReadRn8209WithChk(addr, (uint8_t*)&data , lenth ,dev ))
            {
                memset((buf+1),0x00,4);
                memcpy((buf+1), (uint8_t*)&data, lenth);
            }
            return true;
        }
        else 
        {
            return false;
        }
    }
    else*/
    {
        return ReadRn8209WithChk(addr, buf , len ,dev );
    }
    return false;
}


int Rn8209_write(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void *src)
{
    int result = true;
	uint8_t lenth=0,temp_buffer[8]={0};
	int16_t tmp=0,data=0,temp[30]={0};
    uint32_t s_index = 0;
    uint8_t* buf = src;
 /*   
    if(addr < 3)
    {
        CmdRn8209(CMD_WR_EN , dev );
        mcu_delay_us(5000);
        mcu_delay_us(5000);
        result = WriteRn8209Check(addr, buf, len , dev );
        CmdRn8209(CMD_WR_DIS, dev);
        return result;
    }
    else if(addr == 0x1091) //万能写
    {
        addr = *(buf+4);//地址
        memcpy((uint8_t*)&data, (uint8_t*)buf, 2);//数据
        
        if((addr==0x07)||(addr==0x08))//相位寄存器
            lenth = 1;
        else
            lenth = 2;
        
        if( addr <= 0x17 )//00~17H,需要效验
        {
            CmdRn8209(CMD_WR_EN , dev );
            mcu_delay_us(5000);
            mcu_delay_us(5000);
            if(addr == 0x17)
            {
                ReadRn8209WithChk(EMUCON2, (uint8_t*)&tmp, 2 , dev);
                mcu_delay_us(5000);
                mcu_delay_us(5000);
                if(data == 0x01)
                {
                    data = tmp | 0x0100;
                }
                else
                {
                    data = tmp & 0xFEFF;
                }
            }
            result = WriteRn8209Check(addr, (uint8_t*)&data, lenth , dev );
            memcpy((uint8_t*)&tmp, (uint8_t*)&data, lenth);
            CmdRn8209(CMD_WR_DIS, dev);
            
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Pgain0);
            nd_get_index(instance, s_index, (uint8_t*)&temp_buffer[0], 6); //读取EE中的补偿值
            switch(addr)//存入EEPROM
            {
                case 0x05://增益
                    temp_buffer[0] = (uint8_t)data;
                    temp_buffer[1] = (uint8_t)(data>>8);
                    break;
                case 0x07://相位
                    temp_buffer[4] = (uint8_t)data;
                    break;
                case 0x0A://功率偏置
                    temp_buffer[2] = (uint8_t)data;
                    temp_buffer[3] = (uint8_t)(data>>8);
                    break;
                case 0x17://相位微调
                    temp_buffer[5] = (uint8_t)(data>>8);
                    break;
            }
            nd_set_index(instance, s_index, (uint8_t*)&temp_buffer[0], 6); 
        }
        else if(addr>=0x90)//自热影响
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Heat_Value_10L);
            nd_get_index(instance, s_index, temp, 30); //读取EE中的补偿值
            memcpy((uint8_t*)&temp[addr-0x90], (uint8_t*)(buf), 2);
            nd_set_index(instance, s_index, (uint8_t*)temp, 30);
        }
        else if(addr>=0x80)//电压影响量
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + U80_Value);
            nd_get_index(instance, s_index, (uint8_t *)&temp[0], 8); //读取EE中的补偿值
            switch(addr)
            {
                case 0x80:
                    memcpy((uint8_t*)&temp[0], (uint8_t*)&data, 2);
                    break;
                case 0x81:
                    memcpy((uint8_t*)&temp[1], (uint8_t*)&data, 2);
                    break;
                case 0x82:
                    memcpy((uint8_t*)&temp[2], (uint8_t*)&data, 2);
                    break;
                case 0x83:
                    memcpy((uint8_t*)&temp[3], (uint8_t*)&data, 2);
                    break;
                default:
                    return false;
                    break;
            }
            nd_set_index(instance, s_index, (uint8_t *)&temp[0], 8);
        }
        else if(addr>=0xA0)//频率影响量
        {
            ;;//暂时不加
        }
        else
            result = false;
    }
    else*/
    {
        return WriteRn8209Check(addr, buf, len , dev );
    }
    //return result;
}

const struct driver_ops_t rn8209_drv =
{
    Rn8209_initialize,
    Rn8209_control,
    Rn8209_read,
    Rn8209_write,
};




const struct dev_sys_ops_t dev_rn8209_sys = 
{
    NULL, 
    NULL,
    NULL,
    NULL,
    NULL,
};
