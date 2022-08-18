#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
#include "ndvh_rn8209.h"
#include "lib_ana.h"

struct Rn8209AdjK_t Rn8209Adj_K;

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
            if (ANA_GetStatus(ANA_STATUS_VDCINDROP))    return false;
            
            timeout--;
        }
        bus_ops->rx_enable(2 , index);
        bus_ops->tx_enable(0 , index);
        for (i = 0; i < (length + 1); i++)
        {
            timeout = 5000;
            while ((false == bus_ops->rx_statue(index)) && (timeout > 1))
            {
                if (ANA_GetStatus(ANA_STATUS_VDCINDROP))    return false;

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
                if (ANA_GetStatus(ANA_STATUS_VDCINDROP))    return false;
                
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
    unsigned char err=0;
    
    mcu_delay_us(5000);
    err = WriteRn8209(0xEA, (unsigned char*)&temp32, 1 , dev);
    mcu_delay_us(5000);
    return err;
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
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Other);//写入脉冲速率
    //nd_get_index(instance, s_index, buf, 4);
    memcpy((unsigned char*)&Pstart, buf, 2);
    memcpy((unsigned char*)&Hfconst, buf+2, 2);
    
    if((Pstart == 0) || (Pstart == 0xFFFF))   Pstart = 0x0049;
    if((Hfconst == 0) || (Hfconst == 0xFFFF))   Hfconst = 0x064a;
    
    CmdRn8209(CMD_WR_EN , dev);

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




unsigned char Rn8209AdjParaSet(struct instance_t* instance, struct Rn8209AdjSet_t* Set , const struct dev_info_t* dev)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t addr = 0, data=0, i;
    uint16_t temp=0,metertype=0;
    uint16_t defaultdata[9]={0x3E40, 0x3F41, 0x4042, 0x4143, 0x4244, 0x4345, 0x4446, 0x4547, 0x4648};
    
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
    
    if (true == ReadRn8209WithChk(EMUCON2, (unsigned char*)&temp, 2 , dev))//相位微调
    {
        if((Set->Phs & 0x0100) == 0x0100)
        {
            temp |= 0x0100;
        }
        else
        {
            temp &= ~0x0100;
        }
        
        if (false == WriteRn8209Check(EMUCON2, (unsigned char*)&temp, 2, dev))
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    
    if((mbase_cb->metertype & 0x0800) == 0x0800)    //开启自热补偿功能
    {
        data = 0;
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Cmp_Value);
        nd_get_index(instance, addr, (uint8_t*)&data, 2);//读取自热补偿值
        
        for(i=0; i<9; i++)
        {
            if(data == defaultdata[i])
                break;
        }
        if(i >= 9)  //防止参数非法
        {
            data = 0x4244;
        }
        CmdRn8209(0x02, dev);
        CmdRn8209(0xBA, dev);
        CmdRn8209(0x7F, dev);
        CmdRn8209(0x5C, dev);
        WriteRn8209( 0x51, (uint8_t*)&data, 2 , dev);
    }

    CmdRn8209(CMD_WR_DIS, dev);

    return true;
}

int Rn8209_initialize(struct instance_t* instance, const struct dev_info_t* dev)
{
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;

    //return bus_ops->init((size_t)dev->cfg);
    return 0;
}

int Rn8209_control(struct instance_t* instance, const struct dev_info_t* dev , uint8_t ctrl , void* ctrl_parameter)
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
            
            Rn8209AdjParaSet(instance, set_test , dev);
            break;
        case AC_POWER_OFF:
            ResetRn8209();
            break;
        default:
            break;

    }
    
    return 0;
}

int Rn8209_read(struct instance_t* instance, const struct dev_info_t* dev, uint32_t addr, uint16_t len, void *src)
{
    uint8_t* buf = src;

    return ReadRn8209WithChk(addr, buf , len ,dev );
}


int Rn8209_write(struct instance_t* instance, const struct dev_info_t* dev, uint32_t addr, uint16_t len, void *src)
{
    uint8_t* buf = src;
    
    return WriteRn8209Check(addr, buf, len , dev );
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
