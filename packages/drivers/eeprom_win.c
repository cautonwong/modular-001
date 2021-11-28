#include "nidavellir.h"
#ifdef WIN32
#include "fcntl.h"
#include <sys/stat.h>
#include <WinBase.h>
#include <process.h>

int eeprom_initialize(const struct dev_info_t* dev)
{
    int f = 0;
    int sd = 0;
    uint8_t *p;
    struct stat stat_info;
    
    f = open(dev->name, O_CREAT | O_RDWR | O_BINARY, _S_IREAD | _S_IWRITE);
    if (f > 0)
    {
        sd = stat(dev->name, &stat_info);
        if(stat_info.st_size != dev->data_size)
        {
            close(f);
            _unlink(dev->name);
            f = open(dev->name, O_CREAT | O_RDWR | O_BINARY, _S_IREAD | _S_IWRITE);
            if(f > 0)
            {
                p = malloc(dev->data_size);
                memset(p, 0xff, dev->data_size);
                write(f, p, dev->data_size);
                free(p);
            }
        }
        close(f);
    }

    return f;
}


int eeprom_read_chk(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src) 
{
    int f = 0;
    int n = 0;
    
    f = open(dev->name, O_CREAT | O_RDWR | O_BINARY, _S_IREAD | _S_IWRITE);
    if (f > 0)
    {
        n = lseek(f, addr, SEEK_SET);
        assert(n == addr);
        len = read(f, src, len);
        close(f);
        return len; 
    }

    return -1;
}

int eeprom_write_chk(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src)
{
    int f = 0;
    int n = 0;
    
    f = open(dev->name, O_CREAT | O_RDWR | O_BINARY, _S_IREAD | _S_IWRITE);
    if (f > 0)
    {
        n = lseek(f, addr, SEEK_SET);
        assert(n == addr);
        len = write(f, src, len);
        close(f);
        return len; 
    }

    return -1;
}

int eeprom_control(const struct dev_info_t* dev, uint8_t ctrl , void *ctrl_parameter)
{
    assert(0);
    return 0;
}

const struct driver_ops_t  eeprom_driver =
{
    eeprom_initialize,
    eeprom_control,
    eeprom_read_chk,
    eeprom_write_chk,
};

const struct driver_ops_t  flash_driver =
{
    eeprom_initialize,
    eeprom_control,
    eeprom_read_chk,
    eeprom_write_chk,
};
#endif
/** @} */
