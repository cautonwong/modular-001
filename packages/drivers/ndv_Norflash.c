/**
 * @ingroup    drivers_flashdev
 * @brief      nor flash 驱动
 *
 * @{
 * @file
 *
 */
#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
#include "pin_function_def.h"


/******************************************************************************
**定义Flash操作命令字  MX25L3206E
******************************************************************************/

/**
- 2.7 to 3.6 volt for read, erase, and program operations
- 1024 Equal Sectors with 4K byte each
- Page base (256 bytes)
- Fast program time: 0.6ms(typ.) and 3ms(max.)/page
- Byte program time: 9us (typical)
- Fast erase time: 40ms(typ.) /sector ; 0.4s(typ.) /block

- Low active read current: 25mA(max.) at 86MHz
- Low active programming current: 15mA (typ.)
- Low active sector erase current: 9mA (typ.)
- Standby current: 15uA (max.)
- Deep power-down mode 2uA (typical)

*/

struct spi_io_t
{
    uint8  command[8];
    uint8  *buffer;
    uint16 length;
    uint16 cmdnum;
};

#define  MX_CMD_WRSR     0x01        /** write status register */
#define  MX_CMD_WRDI     0x04        /** write disable         */
#define  MX_CMD_RDSR     0x05        /** read status register  */
#define  MX_CMD_WREN     0x06        /** write enable          */
#define  MX_CMD_READ     0x03        /** Read data byte        */
#define  MX_CMD_SE       0x20        /** Sector Erase          */
#define  MX_CMD_BE       0xD8        /** Block Erase           */
#define  MX_CMD_CE       0x60        /** Chip Erase            */
#define  MX_CMD_PP       0x02        /** page program          */
#define  MX_CMD_DP       0xB9        /** Deep power down       */
#define  MX_CMD_RDP      0xAB        /** Realese from Deep power down */
#define  MX_CMD_RDID     0x9F        /** Read Identification  */

#ifdef  BIT_BAND_EN
#define  DO_CS_HIGH()      {*(flash_hal->cs->lat_out) = 1;}
#define  DO_CS_LOW()       {*(flash_hal->cs->lat_out) = 0;}
#else
#define  DO_CS_HIGH()      {*(flash_hal->cs->lat_out) |= flash_hal->cs->pin_bit ;}
#define  DO_CS_LOW()       {*(flash_hal->cs->lat_out) &= ~flash_hal->cs->pin_bit ;}
#endif

uint8 flash_read_and_compare(struct spi_io_t *read_data, const struct bus_ops_t *bus_ops)
{
    uint16 ui_i;
    uint8 index = bus_ops->bus_index;
    uint8 rcv_data;

    
    if (read_data->cmdnum > 8)
    {
        return false;
    }
    
    bus_ops->ctrl(START , index);
    
    for (ui_i = 0; ui_i < read_data->cmdnum; ui_i ++)
    {
        /* 发送指令和地址 */
        bus_ops->tx(read_data->command[ui_i] , index);
    }
    
    for (ui_i = 0; ui_i < read_data->length; ui_i ++)
    {
        /* 逐个接收字节 */
        bus_ops->rx( &rcv_data ,index);
        if (read_data->buffer[ui_i] != rcv_data)
        {
            bus_ops->ctrl(STOP , index);
            return false;
        }
    }
    
    bus_ops->ctrl(STOP , index);
    
    return true;
}

uint8 flash_write_data(struct spi_io_t *write_data ,  const struct dev_info_t* dev)
{
    uint16 ui_i;                         /* 循环变量 */
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;
    uint8 index = bus_ops->bus_index;
    
    if (write_data->cmdnum > 8)
    {
        return false;
    }
    
    bus_ops->ctrl(START , index);

    for (ui_i = 0; ui_i < write_data->cmdnum; ui_i ++)
    {
        /* 发送指令和地址 */
        bus_ops->tx(write_data->command[ui_i] , index);
    }
    
    /* 循环逐个接收字节 */
    for (ui_i = 0; ui_i < write_data->length; ui_i ++)
    { 
        bus_ops->tx(write_data->buffer[ui_i] , index);
    }
    
    bus_ops->ctrl(STOP , index);
    
    return true;
}




uint8 flash_read_data(struct spi_io_t *read_data, const struct dev_info_t* dev)
{
    uint16 ui_i;                         /* 循环变量 */
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;
    uint8 index = bus_ops->bus_index;
    
    if (read_data->cmdnum > 8)
    {
        return false;
    }
    
    bus_ops->ctrl(START , index);
    
    for (ui_i = 0; ui_i < read_data->cmdnum; ui_i ++)
    {
        /* 发送指令和地址 */
        bus_ops->tx(read_data->command[ui_i] , index);
    }
    
    mcu_delay_us(50);
    /* 循环逐个接收字节 */
    for (ui_i = 0; ui_i < read_data->length; ui_i ++)
    {
        /* 逐个接收字节 */
        bus_ops->rx( &read_data->buffer[ui_i], index);
    }
    
    bus_ops->ctrl(STOP , index);
    
    return flash_read_and_compare(read_data , bus_ops);     /**< 返回读出比较的结果 */
}



uint8 get_flash_status(const struct dev_info_t* dev)
{
    struct spi_io_t spi_io;
    uint8 flash_statue = 0;
    
    spi_io.command[0] = MX_CMD_RDSR;
    spi_io.buffer = &flash_statue;
    spi_io.length = 1;
    spi_io.cmdnum = 1;
    
    flash_read_data(&spi_io , dev);
    if ((flash_statue & 0x01) == 0)
    {
        spi_io.buffer = &flash_statue;
        flash_read_data(&spi_io , dev);
        
        if ((flash_statue & 0x01) == 0)
        {
            return true;
        }
    }
    
    return false;
}

void flash_write_enable(const struct dev_info_t* dev)
{
    struct spi_io_t spi_io;
    
    spi_io.command[0] = MX_CMD_WREN;
    spi_io.buffer = NULL;
    spi_io.length = 0;
    spi_io.cmdnum = 1;
    
    flash_write_data(&spi_io , dev);
    
}

uint8 flash_erase(uint32 address, const struct dev_info_t* dev, uint8 erase_mode)
{
    struct spi_io_t spi_io;
    uint8 err;
    uint16 cnt = 2000;

 
    if (address > dev->data_size)
    {
        return false;
    }
    
    address &= ~0xFFFU;
    flash_write_enable(dev);
    mcu_delay_us(10);
    if (erase_mode == CTRL_FLS_SECTOR_ERASE)
    {
        spi_io.command[0] = MX_CMD_SE;
    }
    else
    {
        spi_io.command[0] = MX_CMD_BE;
    }
    
    spi_io.command[1] = (uint8)(address >> 16);
    spi_io.command[2] = (uint8)(address >> 8);
    spi_io.command[3] = (uint8)address;
    spi_io.buffer = NULL;
    spi_io.cmdnum = 4;
    spi_io.length = 0;
    
    err = flash_write_data(&spi_io , dev);
    
    while (false == get_flash_status(dev))
    {
        cnt--;
        if(cnt == 0)
        {
            break;
        }
    }
    
    return err;
}





int flash_read(struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src)
{    
    struct spi_io_t spi_io;
    
    if ((addr + len) > dev->data_size)
    {
        return false;
    }
    
    spi_io.command[0] = MX_CMD_READ;
    spi_io.command[1] = (uint8)(addr >> 16);
    spi_io.command[2] = (uint8)(addr >> 8);
    spi_io.command[3] = (uint8)addr;
    spi_io.buffer = src;
    spi_io.length = len;
    spi_io.cmdnum = 4;
    
    
    if(false == flash_read_data(&spi_io , dev))
    {
        return false;
    }
    
    return true;
}

int flash_write(struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src)
{
    struct spi_io_t spi_io;
    uint8 ret;
    uint8 tmp;
    uint16 cnt;
    uint16 timeout = 2000;
    uint8* buf = (uint8*)src;
    
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;
    
    if ((addr + len) > dev->data_size)
    {
        return false;
    }
    
    ret = true;
    
    while (len)
    {
        flash_write_enable(dev);
        tmp = addr & (256 - 1);  /* 查找一页开始的地方 */
        
        if (tmp != 0)
        {
            cnt = 256 - tmp;       /* 非一页开始的地方最多一次能写的字节数 */
        }
        else
        { 
            cnt = 256;             /* 页开始的地方最多一次能写 EEP_PAGE_SIZE */
        }
        
        if (len < cnt)
        {
            cnt = len;
        }
        
        /* page program */
        spi_io.command[0] = MX_CMD_PP;
        /* 地址高字节 */
        spi_io.command[1] = (uint8)(addr >> 16);
        /* 地址中字节 */
        spi_io.command[2] = (uint8)(addr >> 8);
        spi_io.command[3] = (uint8)addr;          /* 地址低字节 */
        
        spi_io.buffer = src;           /* 缓存数据 */
        spi_io.cmdnum = 4;             /* 命令长度 */
        spi_io.length = cnt;           /* 数据长度 */
        mcu_delay_us(10);
        ret = flash_write_data(&spi_io , dev);       /* 执行命令 */
        mcu_delay_us(10);
        while (false == get_flash_status(dev))   /* 等待它的空闲 */
        {
            timeout--;
            if(timeout == 0)
            {
                break;
            }
        }
        
        spi_io.command[0] = MX_CMD_READ;
        
        spi_io.cmdnum = 4;
        
        ret = flash_read_and_compare(&spi_io, bus_ops);
        
        if (ret == false)
        {
            break;
        }
        
        addr += cnt;
        buf += cnt;
        len -= cnt;
    }
    
    return ret;
}



uint8 test_flash(struct dev_info_t* dev)
{
    uint8 i;
    
    uint8 test_data[128];
    
    
    for(i=0;i<128;i++)
    {
       test_data[i] = i;
    }

    flash_write(dev, 0x00000000, 128, test_data);
    
    for(i=0;i<128;i++)
    {
       test_data[i] = 0;
    }
    
    flash_read(dev, 0x00000000, 128, test_data);
    
    for(i=0;i<128;i++)
    {
       if(test_data[i] != i)
       return false;
    }
    return true;
}




int flash_ioctrl(struct dev_info_t* dev , uint8 ctrl , void* ctrl_parameter)
{
    uint32 *addr = (uint32 *)ctrl_parameter;

    if (*addr >= dev->data_size)
    {
        return false;
    }

    switch(ctrl)
    {
        case CTRL_FLS_SECTOR_ERASE:
        case CTRL_FLS_BLOCK_ERASE:
              if(false == flash_erase(*addr, dev , ctrl))
              {
                  return false;
              }
              break ;
        case CTRL_FLS_TEST:
              if(true == flash_erase(*addr, dev , ctrl))
              {
                  test_flash(dev);
              }
              break;
        default:
            return false;
    }
    return true;
}



int flash_init(struct dev_info_t *dev)
{
    const struct bus_ops_t *bus_ops = dev->dev_bus_ops;
    
    FLS_VDD_OPEN();
    
    bus_ops->init(0);
    
    return true;
}


const struct driver_ops_t nor_flash_driver = 
{
    flash_init,
    flash_ioctrl,
    flash_read,
    flash_write,
};

/** @} */