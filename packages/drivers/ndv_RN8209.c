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
    uint8 index = bus_ops->bus_index;
      
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
    uint8 index = dev_bus_ops->bus_index;

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
    unsigned short Hfconst = 0x096F;
    unsigned short Pstart = 0x0049;
    unsigned char regIE = 0x01;


    CmdRn8209(CMD_WR_EN , dev);
    mcu_delay_us(5000);
    mcu_delay_us(5000);
    mcu_delay_us(5000);
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

    CmdRn8209(CMD_WR_DIS , dev);
    return true;
}


void ResetRn8209(void)
{
    PIN_RN8209_RX_OUT_MODE();
    PIN_RN8209_RX_OUT_HIGH();
    mcu_delay_us(30000);
    PIN_RN8209_RX_OUT_LOW();
    mcu_delay_us(30000);
    PIN_RN8209_RX_OUT_HIGH();
    mcu_delay_us(10000);
    PIN_RN8209_RX_IN_MODE();
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

int Rn8209_control(const struct dev_info_t* dev , uint8 ctrl , void* ctrl_parameter)
{
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;
    
    struct Rn8209AdjSet_t set_test=
    {
        0x0000,
        0x0000,
        0x0000,
        0x00,
    };
    
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
            
            Rn8209AdjParaSet(&set_test , dev);
            break;
        case AC_POWER_OFF:
            ResetRn8209();
            break;
        default:
            break;

    }
    
    return 0;
}

int Rn8209_read(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src )
{

    return ReadRn8209WithChk(addr, src , len ,dev );

}


int Rn8209_write(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src)
{
    
    return WriteRn8209Check(addr, src, len , dev );

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