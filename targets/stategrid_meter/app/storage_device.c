#include "targets/stategrid_meter/app/storage_device.h"

#include "targets/stategrid_meter/app/resource_ids.h"
#include "targets/stategrid_meter/board/board_eeprom.h"
#include "targets/stategrid_meter/board/board_flash.h"

int nd_storage_read(uint8_t device_name, uint32_t addr, uint8_t *buf,
                    uint16_t len)
{
    if (device_name == RES_DEV_EEPROM) {
        return nd_eeprom_read(board_eeprom(), addr, buf, len);
    }
    if (device_name == RES_DEV_FLASH) {
        return nd_mx25l3206e_read(board_flash(), addr, buf, len);
    }
    return -1;
}

int nd_storage_write(uint8_t device_name, uint32_t addr, const uint8_t *buf,
                     uint16_t len)
{
    if (device_name == RES_DEV_EEPROM) {
        return nd_eeprom_write(board_eeprom(), addr, buf, len);
    }
    if (device_name == RES_DEV_FLASH) {
        return nd_mx25l3206e_write(board_flash(), addr, buf, len);
    }
    return -1;
}

int nd_storage_erase(uint8_t device_name, uint32_t addr)
{
    if (device_name == RES_DEV_FLASH) {
        return nd_mx25l3206e_erase(board_flash(), addr,
                                   ND_MX25L3206E_SECTOR_SIZE);
    }
    return 0;
}

uint32_t nd_storage_size(uint8_t device_name)
{
    if (device_name == RES_DEV_EEPROM) {
        return BOARD_EEPROM_SIZE;
    }
    if (device_name == RES_DEV_FLASH) {
        return BOARD_FLASH_SIZE;
    }
    return 0;
}
