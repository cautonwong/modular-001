#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
/**
 * @ingroup    drivers_eepromdev
 * @brief      eeprom 驱动
 *
 * @{
 * @file
 *
 */
#define EEPROM_PASSWORD         0xA0
#define EEPROM_ERR_JUDGE        5
#define EEPROM_OPERATE_CNTS     300
#define EEPROM_HARD_ADDR        0x00              //电路属性直接代码写死，包括原来的resource中配置的IO等属性也直接在总线驱动中写死，总线驱动的索引编号由dev里面带进去。
#define EEPROM_SIZE             (dev->data_size)  //BOM属性从dev里面带进来
#define EEPROM_PAGE_SIZE        0x80

static uint8_t    eeprom_err_cnts = 0;
static uint16_t    eeprom_operate_cnts = 0;
uint8_t   DEVICE_ERR = 0;//BIT0(ESAM),BIT1(EEPROM)


static uint8_t send_device_addr(uint32_t device_addr , const struct bus_ops_t *iic_ops)
{
    uint32_t cnt = 0;
    uint8_t index = iic_ops->bus_index;

    if ((eeprom_operate_cnts++) > EEPROM_OPERATE_CNTS)
    {
        eeprom_operate_cnts = 0;
    }


    while (iic_ops->tx((uint8_t)(device_addr), index) == false)
    {
        cnt++;

        if (cnt >= 80)
        {
            iic_ops->ctrl(STOP, index);
            if (eeprom_err_cnts < EEPROM_ERR_JUDGE)
            {
                eeprom_err_cnts++;
                if (EEPROM_ERR_JUDGE == eeprom_err_cnts)
                {
                    eeprom_err_cnts = 0;
                }
            }
            return false;
        }

        iic_ops->ctrl(STOP, index);
        mcu_delay_us(100);
        iic_ops->ctrl(START, index);
    }
    eeprom_err_cnts = 0;

    return true;
}

/********************************************************************************
* Function Name  : E2Rom_Write_Page
* Description    : Write one page Data To E2Rom
* Arguments      : E2RomAddr - 地址范围 EEP_Size
*                  Len       - 数据长度
*                  DataAddr  - 数据首地址
*                  Index : start from 0,第一块E方为0,第二块E方为1
* Return         : true / false
********************************************************************************/
uint8_t i2c_mem_page_write(uint32_t physics_addr, uint8_t* DataAddr, uint16_t Len, const struct dev_info_t* dev)
{
    const struct bus_ops_t *iic_ops = dev->dev_bus_ops;
    uint8_t index = iic_ops->bus_index;

    iic_ops->ctrl(START, index);

    if (send_device_addr((physics_addr >> 16), iic_ops) == false)
    {
        iic_ops->ctrl(STOP, index);
        return false;
    }

    if (iic_ops->tx((uint8_t)(physics_addr >> 8), index) == false)
    {
        return false;
    }

    if (iic_ops->tx((uint8_t)physics_addr, index) == false)
    {
        return false;
    }

    while (Len-- != 0)
    {
        if (iic_ops->tx(*DataAddr, index) == false)
        {
            iic_ops->ctrl(STOP, index);
            return false;
        }

        DataAddr++;
    }

    iic_ops->ctrl(STOP, index);

    return true;
}
/********************************************************************************
* Function Name  : E2Rom_Write
* Description    : Write Data To E2Rom, 可跨页写
* Arguments      : E2RomAddr - 地址范围 EEP_Size
*                  Len       - 数据长度
*                  DataAddr  - 数据首地址
*                  Index : start from 0,第一块E方为0,第二块E方为1
* Return         : true / false
********************************************************************************/
uint8_t eeprom_write(uint32_t physics_addr, INT8U* DataAddr, uint16_t Len, const struct dev_info_t* dev)
{
    uint16_t length, tmp;
    uint16_t data_addr;


    while (Len > 0)
    {
        data_addr = physics_addr & 0x0000FFFF;

        tmp = data_addr & (EEPROM_PAGE_SIZE - 1);

        if (tmp != 0)
        {
            length = EEPROM_PAGE_SIZE - tmp;
        }
        else
        {
            length = EEPROM_PAGE_SIZE;
        }

        if (Len < length)
        {
            length = Len;
        }

        if (i2c_mem_page_write(physics_addr, DataAddr, length, dev) == false)
        {
            return false;
        }

        physics_addr += length;
        DataAddr += length;
        Len -= length;
    }

    return true;
}
/********************************************************************************
* Function Name  : E2Rom_Read
* Description    : Read Data From E2Rom
* Arguments      : E2RomAddr - 地址范围 EEP_Size
*                  Len       - 数据长度
*                  DataAddr  - 数据存放首地址
*                  Index : start from 0,第一块E方为0,第二块E方为1
* Return         : true / false
********************************************************************************/
uint8_t eeprom_read(uint32_t physics_addr, uint8_t* data_addr, uint16_t Len, const struct dev_info_t* dev)
{
    size_t recv_ack;
    const struct bus_ops_t *iic_ops = dev->dev_bus_ops;
    uint8_t index = iic_ops->bus_index;

    iic_ops->ctrl(START, index);

    if (send_device_addr((physics_addr >> 16), iic_ops) == false)
    {
        iic_ops->ctrl(STOP, index);
        return false;
    }

    if (iic_ops->tx((uint8_t)(physics_addr >> 8), index) == false)
    {
        return false;
    }

    if (iic_ops->tx((uint8_t)physics_addr, index) == false)
    {
        return false;
    }


    iic_ops->ctrl(START, index);

    if (iic_ops->tx((uint8_t)(physics_addr >> 16) | 0x01, index) == false)
    {
        iic_ops->ctrl(STOP, index);
        return false;
    }

    while (Len-- != 0)
    {
        recv_ack = (Len != 0 ? ACK_1 : ACK_0);
        iic_ops->rx(data_addr, index);
		iic_ops->ctrl(recv_ack , index);
		data_addr++;
    }

    iic_ops->ctrl(STOP, index);

    return true;
}


uint8_t eeprom_data_check(uint32_t physics_addr, volatile uint8_t* pData, uint16_t Len, const struct dev_info_t* dev)
{
    uint8_t RecvACK;
    const struct bus_ops_t *iic_ops = dev->dev_bus_ops;
    uint8_t readData;
    uint8_t index = iic_ops->bus_index;

    iic_ops->ctrl(START , index);

    if (send_device_addr((physics_addr >> 16), iic_ops) == false)
    {
        iic_ops->ctrl(STOP , index);
        return false;
    }

    if (iic_ops->tx((uint8_t)(physics_addr >> 8) , index) == false)
    {
        iic_ops->ctrl(STOP , index);
        return false;
    }

    if (iic_ops->tx((uint8_t)physics_addr, index) == false)
    {
        iic_ops->ctrl(STOP , index);
        return false;
    }

    iic_ops->ctrl(START , index);

    if (iic_ops->tx((uint8_t)(physics_addr >> 16) | 0x01, index) == false)
    {
        iic_ops->ctrl(STOP , index);
        return false;
    }

    mcu_delay_us(20);
    while (Len-- != 0)
    {
        RecvACK = (Len != 0 ? ACK_1 : ACK_0);
        iic_ops->rx(&readData, index ); /* 注意,此处如果不是最后一个字节,要求设备响应 */
        iic_ops->ctrl(RecvACK , index);
        if (readData != *pData)
        {
            if (Len != 0)
            {
                iic_ops->rx(&readData, index);
                iic_ops->ctrl(ACK_0 , index);
            }

            iic_ops->ctrl(STOP , index);
            return false;
        }

        pData++;
    }

    iic_ops->ctrl(STOP , index);

    return true;
}

uint32_t get_physics_addr(uint32_t addr, uint8_t* buf, uint16_t len , const struct dev_info_t* dev)
{

    addr &= 0x0000FFFF;

    if (addr + len > EEPROM_SIZE || buf == NULL)
    {
        return 0;
    }

    addr |= (uint32_t)(EEPROM_PASSWORD | (EEPROM_HARD_ADDR & 0x0E)) << 16;
    return addr;
}




int eeprom_initialize(struct instance_t* instance, const struct dev_info_t* dev)
{
    const struct bus_ops_t *iic_ops = dev->dev_bus_ops;

    return iic_ops->init(iic_ops->bus_index);
}


int eeprom_read_chk(struct instance_t* instance, const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src) 
{
    uint32_t physics_addr = 0;
    uint8_t  errcnt;

    physics_addr = get_physics_addr(addr, src, len , dev);
    if (physics_addr == 0)
    {
        return false;
    }

    DEVICE_ERR &= ~BIT1;
    for (errcnt = 0; errcnt < 8; errcnt++)
    {
        if (eeprom_read(physics_addr, src, len, dev) == false)
        {
            continue;
        }
        else
        {
            break;
        }
    }

    if (errcnt >= 8)
    {
        DEVICE_ERR |= BIT1;
        return false;
    }
    return true;

}
int eeprom_write_chk(struct instance_t* instance, const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src)
{
    uint8_t  errcnt;
    uint32_t physics_addr = 0;

    physics_addr = get_physics_addr(addr, src, len , dev);

    if (physics_addr == 0)
    {
        return false;
    }
    DEVICE_ERR &= ~BIT1;
    for (errcnt = 0; errcnt < 8; errcnt++)
    {
        if (errcnt != 0)
        {
            if (eeprom_write(physics_addr, src, len, dev) == false)
            {
                mcu_delay_us(3000);//待调试
                continue;
            }
            mcu_delay_us(3000);//待调试
        }
        if (eeprom_data_check(physics_addr, src, len, dev) == true)
        {
            break;
        }
    }

    if (errcnt == 8)
    {
        DEVICE_ERR |= BIT1;
        return false;
    }

    return true;



}


int eeprom_control(struct instance_t* instance, const struct dev_info_t* dev, uint8_t ctrl , void *ctrl_parameter)
{
    uint32_t addr = 0;
    uint32_t len = 1;
    uint8_t test_wr[300] , test_rd[300];
    uint16_t i;
    uint8_t Err = 0xAA;
    
    
    while(addr < 0xFFFF)
    {
        for(i=0;i<len;i++)
        {
           test_wr[i] = i;
        }
        
        mcu_clear_watchdog();
        if(true == eeprom_write_chk(instance, dev, addr, len, test_wr))
        {
            for(i=0;i<300;i++)
            {
               test_rd[i] = 0;
            }            
            eeprom_read_chk(instance, dev , addr, len , test_rd); 
            if(mem_compare(test_wr , test_rd , len)!=0)
            {
                Err = 0x55;
            }
        }
        else
        {
            Err = 0x55;
        }
        addr += len;
        
        len++;
        if(len > 300)
        {
            len = 300;
        }
        
        if((addr+len) > 0xFFFF)
        {
            len = 0xFFFF - addr;
        }
    }
    
    return Err;
}

const struct driver_ops_t  CODE eeprom_driver =
{
    eeprom_initialize,
    eeprom_control,
    eeprom_read_chk,
    eeprom_write_chk,
};
/** @} */
